#include "heap.h"

#include <stddef.h>     // size_t, NULL
#include <stdint.h>     // uint8_t, uintptr_t

#include "arena.h"
#include "chunk.h"
#include "freelist.h"

void heap_set_next_chunk_P(heap_t *h, void *hdr, int P) {
    void *nxt = get_next_chunk_hdr(hdr);
    if ((uint8_t*)nxt < h->bump) chunk_set_P(nxt, P);
}

// If the freelist does not have a suitable chunk, carve from bump
void* heap_carve_from_bump(heap_t *h, size_t chunk_size) {
    uintptr_t start = (uintptr_t) h->bump;

    // Ensure payload is 16-byte aligned; header is inuse_chunk_prefix_t bytes before payload
    uintptr_t payload = (start + sizeof(inuse_chunk_prefix_t) + 15u) & ~((uintptr_t)15u);
    uint8_t *hdr = (uint8_t*)(payload - sizeof(inuse_chunk_prefix_t));

    if ((size_t)(h->end - hdr) < chunk_size) {
        int status = arena_mmap_new_heap(h->arena);

        if (status == 0) {
            return heap_carve_from_bump(h->arena->active_heap, chunk_size);
        }
        else {
            return NULL;
        }
    }

    chunk_write_size_to_hdr(hdr, chunk_size);

    // Note that the chunk right before the bump is always in-use.
    // If we are at the base, its safe to say that the previous chunk is "in-use",
    // so we don't try to merge with left neighbor later when the chunk is freed.

    chunk_set_P(hdr, 1);
    chunk_set_M(hdr, 0);

    h->bump = hdr + chunk_size;
    return hdr;
}

static uint8_t *heap_first_chunk_hdr(heap_t *h) {
    uintptr_t start = (uintptr_t)h->base;
    uintptr_t payload = (start + sizeof(inuse_chunk_prefix_t) + 15u) & ~((uintptr_t)15u);
    return (uint8_t *)(payload - sizeof(inuse_chunk_prefix_t));
}

int heap_is_first_chunk(heap_t *h, void *hdr) {
    return (uint8_t *)hdr == heap_first_chunk_hdr(h);
}

// Last chunk here means chunk right before the bump
int heap_is_last_chunk(heap_t *h, void *hdr) {
    void *nxt = get_next_chunk_hdr(hdr);
    return (uint8_t*)nxt == h->bump;
}

// Merge chunk with adjacent free chunks (adjacent in memory, not in the linked list)
void* heap_coalesce_free_chunk(heap_t *h, void *hdr) {
    size_t existing_chunk_size = chunk_get_size(hdr);

    // Merge with right chunk
    if (!heap_is_last_chunk(h, hdr)) {
        void *nxt = get_next_chunk_hdr(hdr);
        
        // Remember that the way we check whether a chunk is free is by inspecting the P flag of the NEXT chunk hdr.
        // So if the next header is the last chunk in the heap, it is not safe to call chunk_is_free, we are reading
        // from the unexplored region.
        
        if (!heap_is_last_chunk(h, nxt) && chunk_is_free(nxt)) {
            size_t next_chunk_size = chunk_get_size(nxt);
            free_list_remove(h->arena, (free_chunk_prefix_t*)nxt);
            existing_chunk_size += next_chunk_size;
            chunk_write_size_to_hdr(hdr, existing_chunk_size);
            chunk_write_ftr(hdr, existing_chunk_size);
        }
    }

    // Merge with left chunk
    if (!heap_is_first_chunk(h, hdr) && prev_chunk_is_free(hdr)) {
        uint8_t *p = (uint8_t*) hdr;
        void *prev_footer = (p - sizeof(size_t));
        size_t previous_chunk_size = chunk_get_size(prev_footer);
        void *prv = p - previous_chunk_size;
        free_list_remove(h->arena, (free_chunk_prefix_t*) prv);
        existing_chunk_size += previous_chunk_size;
        chunk_write_size_to_hdr(prv, existing_chunk_size);
        chunk_write_ftr(prv, existing_chunk_size);
        hdr = prv;
    }

    return hdr;
}

// If the free chunk is large enough, split the chunk
void* heap_split_free_chunk(heap_t *h, free_chunk_prefix_t *prefix, size_t new_chunk_size) {
    size_t *hdr = &prefix->hdr;
    size_t curr_chunk_size = chunk_get_size(hdr);

    if (curr_chunk_size >= new_chunk_size + MIN_CHUNK_SIZE) {
        // Split chunk
        free_list_remove(h->arena, prefix);

        // Header for chunk to allocate
        chunk_write_size_to_hdr(hdr, new_chunk_size);
        heap_set_next_chunk_P(h, hdr, 1);

        // Remainder chunk
        uint8_t *rem = (uint8_t*)hdr + new_chunk_size;
        size_t rem_chunk_size = curr_chunk_size - new_chunk_size;

        chunk_write_size_to_hdr(rem, rem_chunk_size);
        chunk_set_M(rem, 0);
        chunk_write_ftr(rem, rem_chunk_size);

        free_chunk_prefix_t *rem_prefix = (free_chunk_prefix_t*)rem;
        rem_prefix->prev = NULL;
        rem_prefix->next = NULL;

        free_list_push_front(h->arena, rem_prefix);

        return hdr;
    }

    // Can't split - allocate whole chunk
    free_list_remove(h->arena, prefix);
    heap_set_next_chunk_P(h, hdr, 1);

    return hdr;
}

void *heap_split_inuse_chunk(heap_t *h, inuse_chunk_prefix_t *prefix, size_t new_chunk_size) {
    size_t *hdr = &prefix->hdr;
    size_t curr_chunk_size = chunk_get_size(hdr);

    if (curr_chunk_size >= new_chunk_size + MIN_CHUNK_SIZE) {
        chunk_write_size_to_hdr(hdr, new_chunk_size);
        heap_set_next_chunk_P(h, hdr, 1);

        // Remainder chunk
        uint8_t *rem = (uint8_t*)hdr + new_chunk_size;
        size_t rem_chunk_size = curr_chunk_size - new_chunk_size;

        // The original chunk was at the top: return its remainder to bump.
        if (rem + rem_chunk_size == h->bump) {
            h->bump = rem;
            return hdr;
        }

        chunk_write_size_to_hdr(rem, rem_chunk_size);
        chunk_set_M(rem, 0);
        chunk_write_ftr(rem, rem_chunk_size);

        // Coalesce first so insertion uses the final chunk size's bin.
        free_chunk_prefix_t *merged = heap_coalesce_free_chunk(h, rem);
        heap_set_next_chunk_P(h, merged, 0);

        merged->prev = NULL;
        merged->next = NULL;
        free_list_push_front(h->arena, merged);
    }
    
    return hdr;
}
