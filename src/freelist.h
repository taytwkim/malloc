#ifndef TAYMALLOC_FREELIST_H
#define TAYMALLOC_FREELIST_H

#include <stddef.h>     // size_t

#include "arena.h"
#include "chunk.h"

void free_list_remove(arena_t *a, free_chunk_prefix_t *fc);

void free_list_push_front(arena_t *a, free_chunk_prefix_t *fc);

void* free_list_try(arena_t *a, size_t chunk_size);

#endif