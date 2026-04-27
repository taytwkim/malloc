#include "arena.h"
#include "config.h"
#include "debug.h"
#include "freelist.h"
#include "heap.h"
#include "tcache.h"
#include "util.h"

void *malloc(size_t size) {
    ensure_global_init();
    
    safe_log_msg("[malloc]: entered malloc\n");

    if (size == 0) {
        safe_log_msg("[malloc]: requested size is 0, return NULL\n");
        return NULL;
    }

    arena_t *a = arena_from_thread();

    if (!a) {
        safe_log_msg("[malloc]: failed to find arena; return NULL\n");
        return NULL;
    }

    size_t payload = align_16(size);
    size_t need_total = align_16(sizeof(chunk_prefix_t) + payload);     // prefix + header
    size_t min_chunk = get_free_chunk_min_size();

    if (need_total < min_chunk) {
        need_total = min_chunk;
    }

    if (need_total > ARENA_DEFAULT_HEAP_SIZE) {
        safe_log_msg("[malloc]: large request alloc path\n");
        arena_map_new_heap(a, need_total);
        void *hdr = heap_carve_from_bump(a->active_heap, need_total);
        void *ret = chunk_hdr_to_payload(hdr);
        return ret;
    }

    int bin = (int)(need_total / 16) - 2;   // 32->0, 48->1, 64->2 ... smallest is 32 (8 hdr + 16 payload -> 24 -> align -> 32)

    if (bin < 0 || bin >= TCACHE_MAX_BINS) bin = -1;

    // 1) Try tcache first
    void *hdr = NULL;

    if (!g_cfg.disable_tcache && bin >= 0) {
        safe_log_msg("[malloc]: searching tcache\n");

        tcache_bin_t *b = &g_tcache[bin];

        if (b->head != NULL) {
            free_chunk_t *fc = b->head;
            b->head = fc->prev;
            b->count--;
            hdr = (void*)fc;
        }
    }

    // 2) If tcache miss, fall back to arena freelist / bump
    if (!hdr) {
        safe_log_msg("[malloc]: searching freelist\n");

        platform_mutex_lock(&a->lock);

        hdr = free_list_try(a, need_total);

        if (!hdr) {
            safe_log_msg("[malloc]: freelist miss, carve from top\n");
            hdr = heap_carve_from_bump(a->active_heap, need_total);     // if free list miss, carve from top

            if (!hdr) {
                safe_log_msg("[malloc]: malloc failed, return NULL\n");
                platform_mutex_unlock(&a->lock);
                return NULL;
            }
        }
        platform_mutex_unlock(&a->lock);
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
    size_t csz = chunk_get_size(hdr);
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
    
    int bin = (int)(csz / 16) - 2;

    if (bin < 0 || bin >= TCACHE_MAX_BINS) {
        bin = -1;
    }

    // 1) Try to put small chunks into per-thread tcache
    // NOTE: this caches into the current thread's tcache even for cross-thread frees.
    // That’s okay for correctness as long as ownership metadata remains in the chunk.

    if (!g_cfg.disable_tcache && bin >= 0) {
        safe_log_msg("[free]: free to tcache\n");
        
        tcache_bin_t *b = &g_tcache[bin];

        if (b->count < TCACHE_MAX_COUNT) {
            free_chunk_t *fc = (free_chunk_t*)hdr;

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
    platform_mutex_lock(&a->lock);

    chunk_write_size_to_hdr(hdr, csz);
    chunk_write_ftr(hdr, csz);

    safe_log_msg("[free]: merge free chunk\n");
    free_chunk_t *merged = heap_coalesce_free_chunk(h, hdr);

    size_t msz = chunk_get_size(merged);

    uint8_t *merged_end = (uint8_t*)merged + msz;

    heap_set_next_chunk_P(h, merged, 0);

    // if the freed chunk touches the top of THIS heap, shrink bump
    if (merged_end == h->bump) {
        safe_log_msg("[free]: shrink bump\n");
        h->bump = (uint8_t*)merged;

        // unmap heap if it is completely free
        if (h->bump == h->base && !(a->heaps == h && h->next == NULL)) {
            safe_log_msg("[free]: heap unused, unmap heap\n");
            arena_unmap_heap(a, h);
        }

        platform_mutex_unlock(&a->lock);
        return;
    }

    ((free_chunk_t*)merged)->prev = NULL;
    ((free_chunk_t*)merged)->next = NULL;

    safe_log_msg("[free]: push free chunk to freelist\n");
    free_list_push_front(a, (free_chunk_t*)merged);

    platform_mutex_unlock(&a->lock);
}
