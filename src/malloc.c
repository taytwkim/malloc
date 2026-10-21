#include "malloc.h"

#include <pthread.h>    // pthread_mutex_lock, pthread_mutex_unlock
#include <stddef.h>     // size_t, NULL
#include <stdint.h>     // uint8_t
#include <sys/mman.h>   // munmap

#include "arena.h"
#include "chunk.h"
#include "env.h"
#include "freelist.h"
#include "heap.h"
#include "tcache.h"
#include "util.h"

void *malloc(size_t requested_size) {
    global_init_once();
    
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

    if (chunk_size < MIN_CHUNK_SIZE) {
        chunk_size = MIN_CHUNK_SIZE;
    }
    
    size_t max_heap_capacity = HEAP_MMAP_SIZE - heap_get_padding_size();

    if (chunk_size > max_heap_capacity) {
        safe_log_msg("[malloc]: large request alloc path\n");

        void *hdr = mmap_large_chunk(chunk_size);

        if (hdr == NULL) {
            return NULL;
        }

        return chunk_hdr_to_payload(hdr);
    }

    // 1) Try tcache first
    void *hdr = NULL;

    if (!g_env.disable_tcache) {
        hdr = tcache_try_pop(chunk_size);
    }

    // 2) If tcache miss, fall back to arena freelist or active heap bump
    if (!hdr) {
        safe_log_msg("[malloc]: searching freelist\n");

        pthread_mutex_lock(&a->lock);

        hdr = free_list_try(a, chunk_size);

        if (!hdr) {
            safe_log_msg("[malloc]: freelist miss, carve from top\n");
            hdr = heap_carve_from_bump(a->active_heap, chunk_size);

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

    global_init_once();

    uint8_t *hdr = (uint8_t*)chunk_payload_to_hdr(ptr);
    size_t chunk_size = chunk_get_size(hdr);

    if (chunk_get_M(hdr)) {
        munmap_large_chunk(hdr);
        return;
    }

    heap_t *h = get_chunk_heap(hdr);   // Route to the owning heap/arena
    
    if (!h) {
        safe_log_msg("[free]: failed to find the right heap\n");
        return;
    }

    arena_t *a = h->arena;

    if (!a) {
        safe_log_msg("[free]: failed to find the right arena\n");
        return;
    }
    
    // 1) Try to put small chunks into per-thread tcache
    if (!g_env.disable_tcache && tcache_try_push(hdr)) {
        return;
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

    // If the freed chunk touches the top of the heap, shrink bump
    if (merged_end == h->bump) {
        safe_log_msg("[free]: shrink bump\n");
        h->bump = (uint8_t*)merged;

        // Unmap heap if it is completely free
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

void *realloc(void *ptr, size_t requested_size) {
    if (requested_size == 0) {
        return NULL;
    }

    if (ptr == NULL) {
        return malloc(requested_size);
    }

    uint8_t *hdr = (uint8_t*)chunk_payload_to_hdr(ptr);
    size_t curr_chunk_size = chunk_get_size(hdr);
    size_t new_chunk_size = align_16(sizeof(inuse_chunk_prefix_t) + requested_size);

    // A smaller result means the size calculation exceeded SIZE_MAX and wrapped around.
    if (new_chunk_size < requested_size) {
        return NULL;
    }

    if (new_chunk_size < MIN_CHUNK_SIZE) {
        new_chunk_size = MIN_CHUNK_SIZE;
    }

    if (new_chunk_size == curr_chunk_size) {
        return chunk_hdr_to_payload(hdr);
    }

    // If the chunk has been mmaped directly.
    if (chunk_get_M(hdr)) {
        size_t curr_mapping_size = get_mmapped_chunk_mmap_size(curr_chunk_size);
        size_t new_mapping_size = get_mmapped_chunk_mmap_size(new_chunk_size);

        if (new_mapping_size == 0) {
            return NULL;
        }

        // The current mapping can satisfy the new size
        if (new_mapping_size <= curr_mapping_size) {

            // Same number of pages, only update the chunk size.
            if (new_mapping_size == curr_mapping_size) {
                chunk_write_size_to_hdr(hdr, new_chunk_size);
                return ptr;
            }

            // Release pages beyond the new mapping boundary.
            uint8_t *mapping_base = hdr - MMAPED_CHUNK_PADDING;
            uint8_t *tail_start = mapping_base + new_mapping_size;
            size_t tail_size = curr_mapping_size - new_mapping_size;

            // Keep the original size if unmapping fails.
            if (munmap(tail_start, tail_size) != 0) {
                return ptr;
            }

            chunk_write_size_to_hdr(hdr, new_chunk_size);
            return ptr;
        }
        
        // The new size needs a larger mapping.
        else {
            // mmap a new region, copy data, and munmap the old region
            void *new_hdr = mmap_large_chunk(new_chunk_size);

            if (new_hdr == NULL) {
                return NULL;
            }

            chunk_copy_data(hdr, curr_chunk_size, new_hdr, new_chunk_size);
            munmap_large_chunk(hdr);
            return chunk_hdr_to_payload(new_hdr);
        }
    }

    // if (new_chunk_size <= curr_chunk_size) {
    //     // Try splitting
    //     if (curr_chunk_size - new_chunk_size >= MIN_CHUNK_SIZE) {

    //     }

    //     // Do nothing if the chunk is not large enought to be split
    //     return ptr;
    // }

    // // If the requested size is larger than the current chunk size
    // else {
    //     // Check if the next chunk is free and is large enough to be merged
    //     // If not, allocate a new chunk, copy, and free the old chunk
    // }

    return hdr;
}
