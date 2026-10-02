#ifndef TAYMALLOC_UTIL_H
#define TAYMALLOC_UTIL_H

#include <stddef.h>  // size_t
#include <stdint.h>  // uintptr_t
#include <unistd.h>  // sysconf, _SC_PAGESIZE

// requested size is rounded up to a multiple of 16
static inline size_t align_16(size_t n) {
    size_t rem = n % 16;
    return rem == 0 ? n : (n + (16 - rem));
}

// returns whether the pointer's address is aligned to 16
static inline int is_aligned_16(const void *p) {
    return ((uintptr_t) p % 16) == 0;
}

static inline size_t align_pagesize(size_t n) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t ps = page_size < 1 ? (size_t)4096 : (size_t)page_size;
    size_t rem = n % ps;
    return rem ? (n + (ps - rem)) : n;
}

#endif
