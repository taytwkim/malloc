#include "heap.h"
#include "arena.h"
#include "freelist.h"
#include "debug.h"

void heap_set_next_chunk_P(heap_t *h, void *hdr, int P) {
    void *nxt = get_next_chunk_hdr(hdr);
    if ((uint8_t*)nxt < h->bump) chunk_set_P(nxt, P);
}

// if the freelist does not have a suitable chunk, carve from bump
void* heap_carve_from_bump(heap_t *h, size_t need_total) {
    uintptr_t start = (uintptr_t) h->bump;

    // Ensure payload is 16-byte aligned; header is chunk_prefix_t bytes before payload.
    uintptr_t payload = (start + sizeof(chunk_prefix_t) + 15u) & ~((uintptr_t)15u);
    uint8_t *hdr = (uint8_t*)(payload - sizeof(chunk_prefix_t));

    if ((size_t)(h->end - hdr) < need_total) {
        int status = arena_map_new_heap(h->arena, ARENA_DEFAULT_HEAP_SIZE);

        if (status == 0) {
            return heap_carve_from_bump(h->arena->active_heap, need_total);
        }
        else {
            return NULL;
        }
    }

    chunk_write_size_to_hdr(hdr, need_total);

    // Note that the chunk right before the bump is in-use.
    // If we are at the base, its safe to say that the previous chunk is "in-use",
    // so we don't try to merge with left neighbor later when the chunk is freed.

    chunk_set_P(hdr, 1);
    chunk_set_heap(hdr, h);

    h->bump = hdr + need_total;
    return hdr;
}

// last chunk here means chunk right before the bump
static int heap_is_last_chunk(heap_t *h, void *hdr) {
    void *nxt = get_next_chunk_hdr(hdr);
    return (uint8_t*)nxt == h->bump;
}

static uint8_t *heap_first_chunk_hdr(heap_t *h) {
    uintptr_t start = (uintptr_t)h->base;
    uintptr_t payload = (start + sizeof(chunk_prefix_t) + 15u) & ~((uintptr_t)15u);
    return (uint8_t *)(payload - sizeof(chunk_prefix_t));
}

static int heap_is_first_chunk(heap_t *h, void *hdr) {
    return (uint8_t *)hdr == heap_first_chunk_hdr(h);
}

// merge chunk with adjacent free chunks (adjacent in memory, not in the linked list)
void* heap_coalesce_free_chunk(heap_t *h, void *hdr) {
    size_t csz = chunk_get_size(hdr);
    void *nxt = get_next_chunk_hdr(hdr);

    // merge with right chunk
    if (!heap_is_last_chunk(h, hdr)) {
        // remember that the way we check whether a chunk is free is by inspecing the P flag of the NEXT chunk hdr
        // so if the next header is the last chunk in the heap, it is not safe to call chunk_is_free, we are reading
        // from the unexplored region.
        if (!heap_is_last_chunk(h, nxt) && chunk_is_free(nxt)) {
            size_t nxt_sz = chunk_get_size(nxt);
            free_list_remove(h->arena, (free_chunk_t*)nxt);
            csz += nxt_sz;
            chunk_write_size_to_hdr(hdr, csz);
            chunk_write_ftr(hdr, csz);
        }
    }

    // merge with left chunk
    if (!heap_is_first_chunk(h, hdr) && prev_chunk_is_free(hdr)) {
        uint8_t *p = (uint8_t*) hdr;
        void *prev_footer = (p - sizeof(size_t));
        size_t prev_sz = chunk_get_size(prev_footer);
        void *prv = p - prev_sz;
        free_list_remove(h->arena, (free_chunk_t*) prv);
        csz += prev_sz;
        chunk_write_size_to_hdr(prv, csz);
        chunk_write_ftr(prv, csz);
        hdr = prv;
    }

    return hdr;
}

// if the free chunk is large enough, split the chunk
void* heap_split_free_chunk(heap_t *h, free_chunk_t *fc, size_t need) {
    size_t csz = chunk_get_size(fc);
    const size_t MIN_FREE = get_free_chunk_min_size();

    if (csz >= need + MIN_FREE) {
        // split chunk
        free_list_remove(h->arena, fc);

        uint8_t *base = (uint8_t*)fc;

        // allocated chunk header
        chunk_write_size_to_hdr(base, need);
        chunk_set_heap(base, h);
        heap_set_next_chunk_P(h, base, 1);

        // remainder chunk
        uint8_t *rem = base + need;
        size_t rem_sz = csz - need;

        chunk_write_size_to_hdr(rem, rem_sz);
        chunk_write_ftr(rem, rem_sz);
        chunk_set_heap(rem, h);

        ((free_chunk_t*)rem)->prev = NULL;
        ((free_chunk_t*)rem)->next = NULL;
        free_list_push_front(h->arena, (free_chunk_t*)rem);

        return base;
    }

    // can't split - allocate whole chunk
    free_list_remove(h->arena, fc);
    chunk_set_heap(fc, h);
    heap_set_next_chunk_P(h, fc, 1);

    return fc;
}
