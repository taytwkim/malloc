#ifndef TAYMALLOC_TCACHE_H
#define TAYMALLOC_TCACHE_H

#include <stddef.h>     // size_t

#define TCACHE_MAX_BINS 64
#define TCACHE_MAX_COUNT 32

void *tcache_try_pop(size_t chunk_size);
int tcache_try_push(void *hdr);

#endif
