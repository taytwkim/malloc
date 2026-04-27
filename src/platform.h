#ifndef TAYMALLOC_PLATFORM_H
#define TAYMALLOC_PLATFORM_H

#include <stddef.h>
#include <pthread.h>

typedef pthread_mutex_t platform_mutex_t;
typedef pthread_once_t platform_once_t;

#define PLATFORM_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define PLATFORM_ONCE_INIT PTHREAD_ONCE_INIT

int platform_mutex_init(platform_mutex_t *mutex);
int platform_mutex_lock(platform_mutex_t *mutex);
int platform_mutex_unlock(platform_mutex_t *mutex);
int platform_call_once(platform_once_t *once_control, void (*init_routine)(void));

size_t platform_page_size(void);
int platform_cpu_count(void);

void *platform_map_memory(size_t size);
int platform_unmap_memory(void *addr, size_t size);

#endif
