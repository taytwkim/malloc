#ifndef MYALLOC_FREELIST_H
#define MYALLOC_FREELIST_H

#include "arena.h"
#include "heap.h"

void free_list_remove(arena_t *a, free_chunk_t *fc);

void free_list_push_front(arena_t *a, free_chunk_t *fc);

void* free_list_try(arena_t *a, size_t chunk_size);

#endif