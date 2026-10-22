#ifndef TAYMALLOC_CHUNK_H
#define TAYMALLOC_CHUNK_H

#include <stddef.h>     // size_t
#include <stdint.h>     // uint8_t

#include "util.h"

/* 
 * ARENA CHUNK LAYOUT (in a 64 bit machine)
 *
 * in-use:    [ header (size | flags) ]       8 bytes
 *            [ payload ...           ]       <- 16B aligned
 * 
 * free:      [ header (size | flags) ]       8 bytes
 *            [ fd                    ]       8 bytes, forward pointer to the next free chunk
 *            [ bk                    ]       8 bytes, backward pointer to the prev free chunk
 * 
 *            ...
 * 
 *            [ footer (size)         ]       8 bytes, same as the header, but flag bits are zeros
 * 
 * FLAGS
 *  
 *    - bit 0: PREV_IN_USE_BIT (P)
 *          We check this flag when merging two chunks.
 *          When a chunk is freed, we look at its left neighbor and try to merge.
 *          But we want to first make sure that the left neighbor is actually free. 
 *          If we naively read from the left neighbor's footer without checking, 
 *          we might be reading from the payload of an in-use chunk.
 *          This is not really a problem with the right neighbor, 
 *          because both free and in-use chunks have a header.
 * 
 *    - bit 1: MMAPED (M)
 *          If the request size is large enough, malloc falls back to the large-allocation 
 *          path and mmaps a separate region.
 *          These regions are not part of any arena or heap
 *          and can be unmapped directly with munmap().
 * 
 * Note: the reason why we can store the chunk size and the flags in a single header 
 * is because the chunk size is 16-aligned.
 * This means that the low four bits of the chunk size will always be zero,
 * so we can use these bits to store metadata.
 *
 * 
 * MMAPED CHUNK LAYOUT
 * 
 * If the chunk size exceeds the default heap size, we MMAP a region just for that chunk.
 * The base of the MMAPED region is page-aligned (and thus 16B aligned). To ensure that the
 * payload is 16B aligned, we insert a 8B padding.
 * 
 *      [ PADDING               ]   8 bytes <- MMAP base (16B aligned)
 *      [ header (size | flags) ]   8 bytes
 *      [ payload ...           ]   <- 16B aligned
 * 
 * Note that the chunk size stored in the header is the actual aligned chunk size. The
 * size of the entire mmaped region can be retrieved by rounding chunk size + PADDING 
 * up to the page size.
 */

typedef struct heap heap_t;

typedef struct inuse_chunk_prefix {
    size_t hdr;
} inuse_chunk_prefix_t;

typedef struct free_chunk_prefix {
    size_t hdr;
    struct free_chunk_prefix *prev;
    struct free_chunk_prefix *next;
} free_chunk_prefix_t;

#define MIN_CHUNK_SIZE align_16(sizeof(free_chunk_prefix_t) + sizeof(size_t))

// Header & CHUNK_HDR_SIZE_MASK clears the flag bits and gives us the chunk size.
// Notice that we flip the bits (~).
#define CHUNK_HDR_SIZE_MASK (~(size_t)0xF)

// Set: header |= MASK
// Clear: header &= ~Mask
#define CHUNK_HDR_P_MASK ((size_t)1)

#define CHUNK_HDR_M_MASK ((size_t)2)

// MMAPED chunks don't have heap_t and has different alignment.
// Padding ensures that the chunk payload is 16-aligned.
#define MMAPED_CHUNK_PADDING (16 - sizeof(inuse_chunk_prefix_t))

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

// Returns the page-rounded mapping size, or 0 on overflow.
size_t get_mmapped_chunk_mmap_size(size_t chunk_size);

void *mmap_large_chunk(size_t chunk_size);

void munmap_large_chunk(void *hdr);

// Copy data from chunk1 to chunk2.
// Both chunks should be in-use.
// Only min(size1, size2) bytes will be copied.
void chunk_copy_data(void *hdr1, size_t size1, void* hdr2, size_t size2);

#endif
