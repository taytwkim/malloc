#include "platform.h"

#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

int platform_mutex_init(platform_mutex_t *mutex) {
    return pthread_mutex_init(mutex, NULL);
}

int platform_mutex_lock(platform_mutex_t *mutex) {
    return pthread_mutex_lock(mutex);
}

int platform_mutex_unlock(platform_mutex_t *mutex) {
    return pthread_mutex_unlock(mutex);
}

int platform_call_once(platform_once_t *once_control, void (*init_routine)(void)) {
    return pthread_once(once_control, init_routine);
}

size_t platform_page_size(void) {
#ifdef _SC_PAGESIZE
    long page_size = sysconf(_SC_PAGESIZE);
#else
    long page_size = sysconf(_SC_PAGE_SIZE);
#endif

    if (page_size < 1) {
        return (size_t)4096;
    }

    return (size_t)page_size;
}

int platform_cpu_count(void) {
#ifdef _SC_NPROCESSORS_ONLN
    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
#else
    long cpu_count = sysconf(_SC_NPROCESSORS_CONF);
#endif

    if (cpu_count < 1) {
        return 1;
    }

    if (cpu_count > (long)INT_MAX) {
        return INT_MAX;
    }

    return (int)cpu_count;
}

void *platform_map_memory(size_t size) {
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return mem == MAP_FAILED ? NULL : mem;
}

int platform_unmap_memory(void *addr, size_t size) {
    return munmap(addr, size);
}
