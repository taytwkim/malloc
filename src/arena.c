#include "arena.h"
#include "util.h"
#include "config.h"

static platform_once_t g_once = PLATFORM_ONCE_INIT;
static arena_t g_arenas[MAX_NUM_ARENAS];
static int g_num_arenas = 0;
static int g_next_arena = 0;
static platform_mutex_t g_arena_assign_lock = PLATFORM_MUTEX_INITIALIZER;

// If compiled with a specific C standard, the compiler defines __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
    static _Thread_local arena_t *t_arena = NULL;
#else
    static __thread arena_t *t_arena = NULL;
#endif

int arena_map_new_heap(arena_t *a, size_t need_total) {
    size_t req = align_pagesize(need_total);

    void *mem = platform_map_memory(req);
    if (!mem) return -1;

    heap_t *h = (heap_t *)mem;
    h->arena = a;
    h->next = NULL;
    
    uint8_t *payload = (uint8_t *)mem + sizeof(*h);

    h->base = payload;
    h->bump = h->base;
    h->end = (uint8_t *)mem + req;

    if (a->heaps == NULL) {
        a->heaps = h;
    } 
    else {
        heap_t *curr = a->heaps;
        while (curr->next != NULL) curr = curr->next;
        curr->next = h;
    }

    a->active_heap = h;

    return 0;
}

int arena_unmap_heap(arena_t *a, heap_t *h) {
    heap_t *curr = a->heaps;
    heap_t* prev = NULL;

    while (curr) {
        if (curr == h) {
            // remove from linked list
            if (prev == NULL) {
                // if h is the head of the list
                a->heaps = curr->next;
            }
            else {
                prev->next = curr->next;
            }
            
            if (h == a->active_heap) {
                // for now, we always set the last heap to be the active heap
                a->active_heap = prev;
            }

            size_t map_size = (size_t)((uint8_t *)h->end - (uint8_t *)h);
            (void)platform_unmap_memory((void *)h, map_size);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

static void arena_unmap_all_heaps(arena_t *a) {
    heap_t *h = a->heaps;

    while (h) {
        heap_t *next = h->next;
        size_t map_size = (size_t)((uint8_t *)h->end - (uint8_t *)h);
        (void)platform_unmap_memory((void *)h, map_size);
        h = next;
    }
    
    a->heaps = NULL;
    a->active_heap = NULL;
}

static int arena_init(arena_t *a, int id) {
    a->id = id;
    a->heaps = NULL;
    a->active_heap = NULL;
    a->free_list = NULL;
    platform_mutex_init(&a->lock);

    int add_heap_succeeded = arena_map_new_heap(a, ARENA_DEFAULT_HEAP_SIZE);
    
    if (add_heap_succeeded < 0) return -1;

    return 0;
}

// for malloc, we want to allocate from the thread-specific arena
arena_t *arena_from_thread(void) {
    if (t_arena) return t_arena;

    if (g_cfg.disable_arenas) {
        t_arena = &g_arenas[0];
        return t_arena;
    }

    platform_mutex_lock(&g_arena_assign_lock);

    int idx = g_next_arena % g_num_arenas;
    g_next_arena++;

    t_arena = &g_arenas[idx];

    platform_mutex_unlock(&g_arena_assign_lock);

    return t_arena;
}

static void global_init(void) {
    config_init();  // read environment variables once during startup

    if (g_cfg.disable_arenas) {
        g_num_arenas = 1;
    }
    else {
        g_num_arenas = platform_cpu_count();
    }

    if (g_num_arenas < 1) g_num_arenas = 1;
    
    if (g_num_arenas > MAX_NUM_ARENAS) g_num_arenas = MAX_NUM_ARENAS;

    for (int i = 0; i < g_num_arenas; ++i) {
        if (arena_init(&g_arenas[i], i) < 0) {
            // clean up if arena_init fails
            for (int j = 0; j < i; ++j) {
                arena_unmap_all_heaps(&g_arenas[j]);
            }
            return;
        }
    }
}

void ensure_global_init(void) {
    platform_call_once(&g_once, global_init);
}
