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
    uint8_t *bump;         // Start of unexplored region; initially base
    uint8_t *end;          // One past the aligned heap mapping
} heap_t;

/* HEAP LAYOUT
 *
 *      [ PADDING               ]       heap_t + any bytes needed for alignment
 *      [ header (size | flags) ]       First chunk header
 *      [ payload ...           ]       First chunk payload <- 16B aligned
 *
 * heap_get_padding_size() returns the total bytes before the first chunk header,
 * including heap_t. This places the first payload at a 16-byte-aligned address.
 */

static inline size_t heap_get_padding_size(void) {
    return align_16(sizeof(heap_t) + sizeof(inuse_chunk_prefix_t)) - sizeof(inuse_chunk_prefix_t);
}

static inline heap_t *get_chunk_heap(void *hdr) {
    return (heap_t *)((uintptr_t)hdr & ~((uintptr_t)HEAP_MMAP_SIZE - 1));
}

int heap_is_first_chunk(heap_t *h, void *hdr);

int heap_is_last_chunk(heap_t *h, void *hdr);

void heap_set_next_chunk_P(heap_t *h, void *hdr, int P);

// If the freelist does not have a suitable chunk, carve from bump
void* heap_carve_from_bump(heap_t *h, size_t chunk_size);

// Merge chunk with adjacent free chunks (adjacent in memory, not in the linked list)
void* heap_coalesce_free_chunk(heap_t *h, void *hdr);

// If the free chunk is large enough (i.e., remainder is greater than MIN_CHUNK_SIZE), split the chunk.
// Return header of chunk to allocate.
void* heap_split_free_chunk(heap_t *h, free_chunk_prefix_t *prefix, size_t new_chunk_size);

void *heap_split_inuse_chunk(heap_t *h, inuse_chunk_prefix_t *prefix, size_t new_chunk_size);

#endif
