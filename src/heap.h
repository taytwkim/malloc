#ifndef TAYMALLOC_HEAP_H
#define TAYMALLOC_HEAP_H

#include <stddef.h>     // size_t
#include <stdint.h>     // uint8_t

#include "chunk.h"
#include "arena.h"
#include "util.h"

typedef struct arena arena_t;

typedef struct heap {
    arena_t *arena;
    struct heap *next;
    uint8_t *base;         // First chunk header, after heap_t and alignment padding
    uint8_t *bump;         // Frontier of carved chunks; initially base
    uint8_t *end;          // One past the aligned heap mapping
} heap_t;

static inline size_t heap_first_chunk_hdr_offset(void) {
    return align_16(sizeof(heap_t) + sizeof(inuse_chunk_prefix_t)) - sizeof(inuse_chunk_prefix_t);
}

static inline heap_t *get_chunk_heap(void *hdr) {
    return (heap_t *)((uintptr_t)hdr & ~((uintptr_t)HEAP_MMAP_SIZE - 1));
}

void heap_set_next_chunk_P(heap_t *h, void *hdr, int P);

// If the freelist does not have a suitable chunk, carve from bump
void* heap_carve_from_bump(heap_t *h, size_t chunk_size);

// Merge chunk with adjacent free chunks (adjacent in memory, not in the linked list)
void* heap_coalesce_free_chunk(heap_t *h, void *hdr);

// If the free chunk is large enough (remainder is greater than MIN_CHUNK_SIZE), split the chunk.
// Return header of chunk to allocate.
void* heap_split_free_chunk(heap_t *h, free_chunk_prefix_t *fc, size_t chunk_size);

#endif
