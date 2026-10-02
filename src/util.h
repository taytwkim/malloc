#ifndef TAYMALLOC_UTIL_H
#define TAYMALLOC_UTIL_H

#include <stddef.h>     // size_t
#include <stdint.h>     // uintptr_t
#include <unistd.h>     // sysconf, _SC_PAGESIZE, write, ssize_t, STDOUT_FILENO

#include "env.h"

// requested size is rounded up to a multiple of 16
static inline size_t align_16(size_t n) {
    size_t rem = n % 16;
    return rem == 0 ? n : (n + (16 - rem));
}

// returns whether the pointer's address is aligned to 16
static inline int is_aligned_16(const void *p) {
    return ((uintptr_t) p % 16) == 0;
}

static inline size_t align_pagesize(size_t n) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t ps = page_size < 1 ? (size_t)4096 : (size_t)page_size;
    size_t rem = n % ps;
    return rem ? (n + (ps - rem)) : n;
}

static inline size_t safe_strlen(const char *s) {
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

// compiler may generate warning if the return value of write is unused.
// ssize_t is a signed integer type used for sizes when the function may also need to return -1 for an error.
static inline void ignore_write_result(ssize_t result) {
    (void)result;   // casts value to void i.e., we are intentionally not using this value.
}

static inline void safe_log_msg(const char *msg) {
    if (g_config.verbose && msg) {
        ignore_write_result(write(1, msg, safe_strlen(msg)));
    }
}

static inline void safe_log_ptr(const char *msg, void *ptr) {
    if (!g_config.verbose) return;

    // Print the message first
    safe_log_msg(msg);

    // Convert pointer to hex manually
    unsigned long n = (unsigned long)ptr;
    char buf[19]; 
    buf[0] = '0'; buf[1] = 'x';
    buf[18] = '\n'; // Add newline at the end

    for (int i = 17; i >= 2; i--) {
        int digit = n % 16;
        buf[i] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        n /= 16;
    }
    
    ignore_write_result(write(STDOUT_FILENO, buf, 19));
}

#endif
