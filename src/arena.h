#ifndef TAYMALLOC_ARENA_H
#define TAYMALLOC_ARENA_H

#include <pthread.h>  // pthread_mutex_t
#include <stddef.h>   // size_t
#include <stdint.h>   // SIZE_MAX

#include "chunk.h"

#define MAX_NUM_ARENAS 64

// HEAP_MMAP_SIZE should be a multiple of page size.
// HEAP_MMAP_SIZE includes sizeof(heap_t) and initial padding to 
// make the first chunk 16-aligned.
// Actual capacity (space for chunks) is smaller.
#define HEAP_MMAP_SIZE ((size_t)16 * 1024 * 1024)

// Number of size-class bins for the free list.
#define DEFAULT_FREE_LIST_MAX_BINS 64

typedef struct arena {
    int id;
    heap_t *heaps;

    // Active heap is the heap we carve from when there is no suitable chunk in the free list.
    // For now, let's assume that the active heap is always the heap that was most recently mmaped.
    heap_t *active_heap;
    
    free_chunk_prefix_t* free_list[DEFAULT_FREE_LIST_MAX_BINS];
    pthread_mutex_t lock;
} arena_t;

// Checks at compile time
_Static_assert(HEAP_MMAP_SIZE != 0 &&
               (HEAP_MMAP_SIZE & (HEAP_MMAP_SIZE - 1)) == 0,
               "Heap mmap size must be a power of two.");

// We first map twice HEAP_MMAP_SIZE so we can keep a HEAP_MMAP_SIZE-aligned region
// of that size, then unmap the unused prefix and suffix.
_Static_assert(HEAP_MMAP_SIZE <= SIZE_MAX / 2,
               "Heap mmap size must allow doubling.");

// Create one aligned heap using HEAP_MMAP_SIZE.
int arena_mmap_new_heap(arena_t *a);

// Find heap and remove from the linked list
int arena_munmap_heap(arena_t *a, heap_t *h);

// For malloc, we want to allocate from the thread's designated arena
arena_t *arena_from_thread(void);

void global_init_once(void);

#endif
