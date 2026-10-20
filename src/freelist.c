#include "freelist.h"

#include <stddef.h>     // size_t, NULL

#include "arena.h"
#include "chunk.h"
#include "env.h"
#include "heap.h"
#include "util.h"

int get_free_list_bin(size_t chunk_size) {
    size_t units = chunk_size / 16;
    size_t min_units = MIN_CHUNK_SIZE / 16;

    if (units < min_units) {
        return -1;
    }

    if (units - min_units >= (size_t)(g_env.freelist_num_bins - 1)) {
        return g_env.freelist_num_bins - 1;
    }

    return units - min_units;
}

void* free_list_try(arena_t *a, size_t chunk_size) {
    int bin = get_free_list_bin(chunk_size);

    if (bin < 0) {
        return NULL;
    }

    while (bin < g_env.freelist_num_bins - 1 && !a->free_list[bin]) {
        ++bin;
    }

    // found a free chunk in exact size bin
    if (bin < g_env.freelist_num_bins - 1) {
        heap_t *h = get_chunk_heap(a->free_list[bin]);
        return heap_split_free_chunk(h, a->free_list[bin], chunk_size);
    }

    // search oversize bin
    for (free_chunk_prefix_t *p = a->free_list[bin]; p; p = p->next) {
        if (chunk_get_size(p) >= chunk_size) {
            heap_t *h = get_chunk_heap(p);
            return heap_split_free_chunk(h, p, chunk_size);
        }
    }

    return NULL;
}

void free_list_remove(arena_t *a, free_chunk_prefix_t *fc) {
    free_chunk_prefix_t *fd = fc->prev, *bk = fc->next;

    if (bk) {
        bk->prev = fd;
    }

    if (fd) {
        fd->next = bk;
    }

    int bin = get_free_list_bin(chunk_get_size(fc));

    if (a->free_list[bin] == fc) {
        a->free_list[bin] = bk;
    }
    
    fc->prev = fc->next = NULL;
}

void free_list_push_front(arena_t *a, free_chunk_prefix_t *fc) {
    fc->prev = NULL;
    int bin = get_free_list_bin(chunk_get_size(fc));
    fc->next = a->free_list[bin];
    
    if (a->free_list[bin]) {
        a->free_list[bin]->prev = fc;   
    }

    a->free_list[bin] = fc;
}
