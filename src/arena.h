#ifndef TAYMALLOC_ARENA_H
#define TAYMALLOC_ARENA_H

#include <pthread.h>  // pthread_mutex_t
#include <stddef.h>   // size_t
#include <stdint.h>   // SIZE_MAX

#include "chunk.h"

#define MAX_NUM_ARENAS 64

// Heap mapping size including heap_t and initial padding.
// Actual heap_capacity (space for chunks) is smaller than this value.
#define HEAP_MAPPING_SIZE ((size_t)16 * 1024 * 1024)

typedef struct arena {
    int id;
    heap_t *heaps;
    heap_t *active_heap;    // For now, let's assume that the active_heap is always the heap that was most recently added
    free_chunk_prefix_t *free_list;
    pthread_mutex_t lock;
} arena_t;

// Checks at compile time
_Static_assert(HEAP_MAPPING_SIZE != 0 &&
               (HEAP_MAPPING_SIZE & (HEAP_MAPPING_SIZE - 1)) == 0,
               "heap mapping size must be a power of two");

_Static_assert(HEAP_MAPPING_SIZE <= SIZE_MAX / 2,
               "heap mapping size must allow doubling");

// Create one aligned heap using HEAP_MAPPING_SIZE.
int arena_mmap_new_heap(arena_t *a);

// Find heap and remove from the linked list
int arena_munmap_heap(arena_t *a, heap_t *h);

// For malloc, we want to allocate from the thread's designated arena
arena_t *arena_from_thread(void);

void global_init_once(void);

#endif
