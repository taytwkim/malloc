#define _DEFAULT_SOURCE
#include "malloc.h"

#include <pthread.h>    // pthread_mutex_lock, pthread_mutex_unlock
#include <stddef.h>     // size_t, NULL
#include <stdint.h>     // uint8_t
#include <sys/mman.h>   // mmap, munmap, mapping flags

#include "arena.h"
#include "chunk.h"
#include "env.h"
#include "freelist.h"
#include "heap.h"
#include "tcache.h"
#include "util.h"

void *malloc(size_t requested_size) {
    ensure_global_init();
    
    safe_log_msg("[malloc]: entered malloc\n");

    if (requested_size == 0) {
        safe_log_msg("[malloc]: requested size is 0, return NULL\n");
        return NULL;
    }

    arena_t *a = arena_from_thread();

    if (!a) {
        safe_log_msg("[malloc]: failed to find arena; return NULL\n");
        return NULL;
    }
    
    size_t chunk_size = align_16(sizeof(inuse_chunk_prefix_t) + requested_size);

    // A smaller result means the size calculation exceeded SIZE_MAX and wrapped around.
    if (chunk_size < requested_size) {
        return NULL;
    }

    size_t min_chunk_size = align_16(sizeof(free_chunk_prefix_t) + sizeof(size_t));

    if (chunk_size < min_chunk_size) {
        chunk_size = min_chunk_size;
    }
    
    size_t max_heap_capacity = ARENA_DEFAULT_MAPPING_SIZE - align_16(sizeof(heap_t));

    if (chunk_size > max_heap_capacity) {
        safe_log_msg("[malloc]: large request alloc path\n");

        size_t mapping_size = align_pagesize(chunk_size);

        if (mapping_size < chunk_size) {
            return NULL;
        }

        void *mem = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) return NULL;

        inuse_chunk_prefix_t *hdr = mem;
        chunk_write_size_to_hdr(hdr, mapping_size);
        chunk_set_M(hdr, 1);
        chunk_set_heap(hdr, NULL);

        return chunk_hdr_to_payload(hdr);
    }

    int bin = (int)(chunk_size / 16) - 2;   // Chunk-size bins: 32->0, 48->1, 64->2, ...; sizes include the prefix.
    if (bin < 0 || bin >= TCACHE_MAX_BINS) bin = -1;

    void *hdr = NULL;

    // 1) Try tcache first
    if (!g_config.disable_tcache && bin >= 0) {
        safe_log_msg("[malloc]: searching tcache\n");

        tcache_bin_t *b = &g_tcache[bin];

        if (b->head != NULL) {
            free_chunk_prefix_t *fc = b->head;
            b->head = fc->prev;
            b->count--;
            hdr = (void*)fc;
        }
    }

    // 2) If tcache miss, fall back to arena freelist / bump
    if (!hdr) {
        safe_log_msg("[malloc]: searching freelist\n");

        pthread_mutex_lock(&a->lock);

        hdr = free_list_try(a, chunk_size);

        if (!hdr) {
            safe_log_msg("[malloc]: freelist miss, carve from top\n");
            hdr = heap_carve_from_bump(a->active_heap, chunk_size);     // if free list miss, carve from top

            if (!hdr) {
                safe_log_msg("[malloc]: malloc failed, return NULL\n");
                pthread_mutex_unlock(&a->lock);
                return NULL;
            }
        }

        pthread_mutex_unlock(&a->lock);
    }

    void *ret = chunk_hdr_to_payload(hdr);
    safe_log_ptr("[malloc]: allocated: ", ret);

    return ret;
}

void free(void *ptr) {
    safe_log_msg("[free]: entered free\n");

    if (!ptr) {
        safe_log_msg("[free]: received nullptr\n");
        return;
    }

    ensure_global_init();

    uint8_t *hdr = (uint8_t*)chunk_payload_to_hdr(ptr);
    size_t chunk_size = chunk_get_size(hdr);

    if (chunk_get_M(hdr)) {
        munmap(hdr, chunk_get_size(hdr));
        return;
    }

    heap_t *h = chunk_get_heap(hdr);   // Route to the owning heap/arena (cross-thread correct)
    
    if (!h) {
        safe_log_msg("[free]: failed to find the right heap\n");
        return;
    }

    arena_t *a = h->arena;

    if (!a) {
        safe_log_msg("[free]: failed to find the right arena\n");
        return;
    }
    
    int bin = (int)(chunk_size / 16) - 2;

    if (bin < 0 || bin >= TCACHE_MAX_BINS) {
        bin = -1;
    }

    // 1) Try to put small chunks into per-thread tcache
    // NOTE: this caches into the current thread's tcache even for cross-thread frees.
    // That’s okay for correctness as long as ownership metadata remains in the chunk.

    if (!g_config.disable_tcache && bin >= 0) {
        safe_log_msg("[free]: free to tcache\n");
        
        tcache_bin_t *b = &g_tcache[bin];

        if (b->count < TCACHE_MAX_COUNT) {
            free_chunk_prefix_t *fc = (free_chunk_prefix_t*)hdr;

            // IMPORTANT: do NOT mark as free, do NOT set footer, do NOT coalesce.
            // Chunk stays "in-use" from the global allocator's point of view.
            
            fc->prev = b->head;
            b->head = fc;
            b->count++;
            return;
        }
    }

    // 2) Fall back to global free path: mark free, coalesce in the owning heap, push to arena freelist.
    safe_log_msg("[free]: free to freelist\n");
    pthread_mutex_lock(&a->lock);

    chunk_write_size_to_hdr(hdr, chunk_size);
    chunk_write_ftr(hdr, chunk_size);

    safe_log_msg("[free]: merge free chunk\n");
    free_chunk_prefix_t *merged = heap_coalesce_free_chunk(h, hdr);

    size_t merged_chunk_size = chunk_get_size(merged);

    uint8_t *merged_end = (uint8_t*)merged + merged_chunk_size;

    heap_set_next_chunk_P(h, merged, 0);

    // if the freed chunk touches the top of THIS heap, shrink bump
    if (merged_end == h->bump) {
        safe_log_msg("[free]: shrink bump\n");
        h->bump = (uint8_t*)merged;

        // unmap heap if it is completely free
        if (h->bump == h->base && !(a->heaps == h && h->next == NULL)) {
            safe_log_msg("[free]: heap unused, unmap heap\n");
            arena_munmap_heap(a, h);
        }

        pthread_mutex_unlock(&a->lock);
        return;
    }

    ((free_chunk_prefix_t*)merged)->prev = NULL;
    ((free_chunk_prefix_t*)merged)->next = NULL;

    safe_log_msg("[free]: push free chunk to freelist\n");
    free_list_push_front(a, (free_chunk_prefix_t*)merged);

    pthread_mutex_unlock(&a->lock);
}
