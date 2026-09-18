#ifndef MYALLOC_CHUNK_H
#define MYALLOC_CHUNK_H

#include <stddef.h>     // size_t
#include <stdint.h>     // uint8_t
#include "util.h"       // align_16

/* 
 * chunk layout

 * in-use:    [ header (size | flags) ]       8 bytes (in a 64 bit machine)
 *            [ owning heap ptr       ]       8 bytes
 *            [ payload ...           ]
 * 
 * free:      [ header (size | flags) ]       8 bytes
 *            [ owning heap ptr       ]       8 bytes
 *            [ fd                    ]       8 bytes, forward pointer to the next free chunk
 *            [ bk                    ]       8 bytes, backward pointer to the prev free chunk
 *            ... 
 *            [ footer (size )        ]       8 bytes, same as the header, but flag bits are zeros
 * 
 * flags
 *  
 *    - bit 0: PREV_IN_USE_BIT (P)
 *          We need this flag when merging two chunks.
 *          When a chunk is freed, we look at its left neighbor and try to merge.
 *          But we want to first make sure that the left chunk is actually free. 
 *          If we naively read from the left chunk's footer without checking, 
 *          we might be reading from the payload of an in-use chunk.
 *          This is not really a problem when merging with the right chunk, 
 *          because both free and in-use chunks have the header.
 * 
 *    - bit 1: MMAPED (M)
 *          If the request size is large enough, malloc uses the large-allocation path
 *          and mmaps a separate region. These regions are not part of any arena or heap
 *          and can be released directly with munmap().
 * 
 * Note: the reason why we can store the chunk size and the flags in a single header 
 * is because the chunk size is 16 aligned in a 64-bit machine.
 * This means that the low four bits of the chunk size will always be zero,
 * so we can use these bits to store metadata.
 */

typedef struct heap heap_t;

typedef struct inuse_chunk_prefix {
    size_t hdr;
    heap_t *heap;
} inuse_chunk_prefix_t;

typedef struct free_chunk_prefix {
    size_t hdr;
    heap_t *heap;
    struct free_chunk_prefix *prev;
    struct free_chunk_prefix *next;
} free_chunk_prefix_t;

// header & CHUNK_HDR_SIZE_MASK clears the flag bits and gives us the chunk size.
// Notice that we flip the bits (~).
#define CHUNK_HDR_SIZE_MASK (~(size_t)0xF)

// Set: header |= MASK
// Clear: header &= ~Mask
#define CHUNK_HDR_P_MASK ((size_t)1)

#define CHUNK_HDR_M_MASK ((size_t)2)

static inline size_t chunk_get_size(void *hdr) { 
    return (*(size_t*)hdr) & CHUNK_HDR_SIZE_MASK; 
}

static inline int chunk_get_P(void *hdr) {
    return (*(size_t*)hdr & CHUNK_HDR_P_MASK) != 0;
}

static inline int chunk_get_M(void *hdr) {
    return (*(size_t*)hdr & CHUNK_HDR_M_MASK) != 0;
}

static inline void chunk_set_P(void *hdr, int on) {
    if (on) *(size_t*)hdr |= CHUNK_HDR_P_MASK;
    else    *(size_t*)hdr &= ~CHUNK_HDR_P_MASK;
}

static inline void chunk_set_M(void *hdr, int on) {
    if (on) *(size_t*)hdr |= CHUNK_HDR_M_MASK;
    else    *(size_t*)hdr &= ~CHUNK_HDR_M_MASK;
}

static inline void chunk_write_size_to_hdr(void *hdr, size_t size_aligned) {
    size_t flags = *(size_t*)hdr & (CHUNK_HDR_P_MASK | CHUNK_HDR_M_MASK);
    *(size_t*)hdr = (size_aligned & CHUNK_HDR_SIZE_MASK) | flags;
}

static inline void chunk_write_ftr(void *hdr, size_t size_aligned) {
    uint8_t *base = (uint8_t*)hdr;
    *(size_t*)(base + size_aligned - sizeof(size_t)) = (size_aligned & CHUNK_HDR_SIZE_MASK);
}

static inline uint8_t* chunk_hdr_to_payload(void *hdr) { 
    return (uint8_t*)hdr + sizeof(inuse_chunk_prefix_t);
}

static inline void* chunk_payload_to_hdr(void *ptr) { 
    return (uint8_t*)ptr - sizeof(inuse_chunk_prefix_t);
}

static inline int prev_chunk_is_free(void *hdr) { 
    return !chunk_get_P(hdr);
}

static inline void* get_next_chunk_hdr(void *hdr) { 
    return (uint8_t*)hdr + chunk_get_size(hdr); 
}

static inline int chunk_is_free(void *hdr) {
    void *nxt = get_next_chunk_hdr(hdr);
    return prev_chunk_is_free(nxt);
}

static inline heap_t* chunk_get_heap(void *hdr) { 
    return ((inuse_chunk_prefix_t*)hdr)->heap;
}

static inline void chunk_set_heap(void *hdr, heap_t *h) { 
    ((inuse_chunk_prefix_t*)hdr)->heap = h;
}

static inline size_t get_free_chunk_min_size(void) { 
    return align_16(sizeof(free_chunk_prefix_t) + sizeof(size_t));
}

#endif
