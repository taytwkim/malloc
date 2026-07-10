#ifndef MYALLOC_HEAP_H
#define MYALLOC_HEAP_H

#include <stdint.h>
#include "chunk.h"

typedef struct arena arena_t;

typedef struct heap {
    arena_t *arena;
    struct heap *next;
    uint8_t *base;
    uint8_t *bump;
    uint8_t *end;
} heap_t;

void heap_set_next_chunk_P(heap_t *h, void *hdr, int P);

// if the freelist does not have a suitable chunk, carve from bump
void* heap_carve_from_bump(heap_t *h, size_t need_total);

// merge chunk with adjacent free chunks (adjacent in memory, not in the linked list)
void* heap_coalesce_free_chunk(heap_t *h, void *hdr);

// if the free chunk is large enough, split the chunk
void* heap_split_free_chunk(heap_t *h, free_chunk_t *fc, size_t need);

#endif