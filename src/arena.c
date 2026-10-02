#define _DEFAULT_SOURCE
#include "arena.h"

#include <pthread.h>    // mutexes and one-time initialization
#include <stddef.h>     // size_t, NULL
#include <stdint.h>     // uint8_t
#include <sys/mman.h>   // mmap, munmap, mapping flags
#include <unistd.h>     // sysconf, _SC_NPROCESSORS_ONLN

#include "chunk.h"
#include "env.h"
#include "heap.h"
#include "util.h"

static pthread_once_t g_once = PTHREAD_ONCE_INIT;
static arena_t g_arenas[MAX_NUM_ARENAS];
static int g_num_arenas = 0;
static int g_next_arena = 0;
static pthread_mutex_t g_arena_assign_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local arena_t *t_arena = NULL;

int arena_mmap_new_heap(arena_t *a, size_t mapping_size) {
    mapping_size = align_pagesize(mapping_size);

    void *mem = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return -1;

    heap_t *h = (heap_t *)mem;
    h->arena = a;
    h->next = NULL;
    
    size_t first_chunk_offset = align_16(sizeof(*h) + sizeof(inuse_chunk_prefix_t)) - sizeof(inuse_chunk_prefix_t);
    h->base = (uint8_t *)mem + first_chunk_offset;
    h->bump = h->base;
    h->end = (uint8_t *)mem + mapping_size;

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

int arena_munmap_heap(arena_t *a, heap_t *h) {
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

            size_t mapping_size = (size_t)((uint8_t *)h->end - (uint8_t *)h);
            (void)munmap((void *)h, mapping_size);

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
        size_t mapping_size = (size_t)((uint8_t *)h->end - (uint8_t *)h);
        (void)munmap((void *)h, mapping_size);
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
    pthread_mutex_init(&a->lock, NULL);

    int add_heap_succeeded = arena_mmap_new_heap(a, ARENA_DEFAULT_MAPPING_SIZE);
    
    if (add_heap_succeeded < 0) return -1;

    return 0;
}

// for malloc, we want to allocate from the thread-specific arena
arena_t *arena_from_thread(void) {
    if (t_arena) return t_arena;

    if (g_config.disable_arenas) {
        t_arena = &g_arenas[0];
        return t_arena;
    }

    pthread_mutex_lock(&g_arena_assign_lock);

    int idx = g_next_arena % g_num_arenas;
    g_next_arena++;

    t_arena = &g_arenas[idx];

    pthread_mutex_unlock(&g_arena_assign_lock);

    return t_arena;
}

static void global_init(void) {
    config_init();  // read environment variables once during startup

    if (g_config.disable_arenas) {
        g_num_arenas = 1;
    }
    else {
        long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
        g_num_arenas = cpu_count < 1 ? 1
                     : cpu_count > MAX_NUM_ARENAS ? MAX_NUM_ARENAS
                     : (int)cpu_count;
    }

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
    pthread_once(&g_once, global_init);
}
