#ifndef TAYMALLOC_H
#define TAYMALLOC_H

#include <stddef.h>     // size_t

void *malloc(size_t requested_size);
void free(void *ptr);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t requested_size);

#endif