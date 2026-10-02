#ifndef TAYMALLOC_TCACHE_H
#define TAYMALLOC_TCACHE_H

#include "chunk.h"  // free_chunk_prefix_t

#define TCACHE_MAX_BINS 64
#define TCACHE_MAX_COUNT 32

typedef struct tcache_bin {
    free_chunk_prefix_t *head;   // head of the linked list
    int count;
} tcache_bin_t;

static _Thread_local tcache_bin_t g_tcache[TCACHE_MAX_BINS];  // per-thread tcache

#endif