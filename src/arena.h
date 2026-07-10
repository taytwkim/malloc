#ifndef MYALLOC_ARENA_H
#define MYALLOC_ARENA_H

#include "chunk.h"
#include "heap.h"
#include "platform.h"

#define MAX_NUM_ARENAS 64
#define ARENA_DEFAULT_HEAP_SIZE (size_t) 16 * 1024 * 1024

typedef struct arena {
    int id;
    heap_t *heaps;
    heap_t *active_heap;    // for now, let's assume that the active_heap is always the heap that was most recently added
    free_chunk_t *free_list;
    platform_mutex_t lock;
} arena_t;

int arena_map_new_heap(arena_t *a, size_t need_total);

// find heap and remove from the linked list
int arena_unmap_heap(arena_t *a, heap_t *h);

// for malloc, we want to allocate from the thread-specific arena
arena_t *arena_from_thread(void);

void ensure_global_init(void);

#endif
