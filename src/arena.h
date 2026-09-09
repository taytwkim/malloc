#ifndef MYALLOC_ARENA_H
#define MYALLOC_ARENA_H

#include "chunk.h"
#include "heap.h"
#include "platform.h"

#define MAX_NUM_ARENAS 64

// Total mapping bytes, including heap_t and initial alignment padding.
// Actual heap_capacity (space for chunks) is smaller than this value.
#define ARENA_DEFAULT_MAPPING_SIZE ((size_t)16 * 1024 * 1024)

typedef struct arena {
    int id;
    heap_t *heaps;
    heap_t *active_heap;    // for now, let's assume that the active_heap is always the heap that was most recently added
    free_chunk_t *free_list;
    platform_mutex_t lock;
} arena_t;

// Round mapping_size up to a page and place heap_t inside that mapping.
int arena_mmap_new_heap(arena_t *a, size_t mapping_size);

// find heap and remove from the linked list
int arena_munmap_heap(arena_t *a, heap_t *h);

// for malloc, we want to allocate from the thread-specific arena
arena_t *arena_from_thread(void);

void ensure_global_init(void);

#endif
