// tests/parallel.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "../src/malloc.h"

/* Tests for multi-threaded mallocs and frees */

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [num_threads] [num_iters]\n", prog);
    fprintf(stderr, "Defaults: num_threads=4, num_iters=10000\n");
}

int main(int argc, char **argv) {
    int num_threads = 4;
    size_t num_iters = 10000;   // iterations per thread
    enum { BATCH_SIZE = 64 };

    if (argc > 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (argc >= 2) {
        num_threads = atoi(argv[1]);
        if (num_threads <= 0) {
            fprintf(stderr, "Invalid num_threads: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (argc == 3) {
        num_iters = strtoull(argv[2], NULL, 10);
        if (num_iters == 0) {
            fprintf(stderr, "Invalid num_iters: %s\n", argv[2]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("test2: multithreaded alloc/free test\n");
    printf("  threads = %d, iters per thread = %zu\n", num_threads, num_iters);

    int errors = 0;

    #pragma omp parallel num_threads(num_threads) reduction(+:errors)
    {
        int tid = omp_get_thread_num();

        for (size_t i = 0; i < num_iters; i++) {
            size_t sz = 16 + ((i + tid) % 256);   // mixed sizes
            unsigned char *p = (unsigned char*)malloc(sz);
            
            if (!p) {
                printf("Thread %d: my_malloc returned NULL at iter %zu\n", tid, i);
                errors++;
                break;
            }

            // Fill with a thread-specific pattern
            unsigned char pattern = (unsigned char)(tid + 1);
            memset(p, pattern, sz);

            // Verify the pattern (check corruption)
            for (size_t j = 0; j < sz; j++) {
                if (p[j] != pattern) {
                    printf("Thread %d: data corrupted at iter %zu, offset %zu\n", tid, i, j);
                    errors++;
                    break;
                }
            }
            free(p);

            // If this thread saw an error, stop
            if (errors > 0) {
                break;
            }
        }
    }
    if (errors > 0) {
        printf("test2: FAILED (errors = %d)\n", errors);
        return 1;
    }
    
    printf("test2: PASSED ✅\n");
    return 0;
}
