#ifndef MY_ALLOC_H
#define MY_ALLOC_H

#include <stddef.h> // for size_t

void *malloc(size_t requested_size);

void free(void *ptr);

#endif