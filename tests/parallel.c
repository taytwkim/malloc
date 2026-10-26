#define _POSIX_C_SOURCE 200809L // rand_r

#include <pthread.h> // pthread_create, pthread_join
#include <stdio.h>   // printf, fprintf, stderr
#include <stdlib.h>  // malloc, free, atoi, strtoull, rand_r, size_t, NULL

// Default: 4 threads each allocate and free 10,000 times
int NUM_THREADS = 4;
size_t NUM_ITERS = 10000;

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [num_threads] [num_iters]\n", prog);
    fprintf(stderr, "Default: num_threads=4, num_iters=10000\n");
}

typedef struct {
    int tid;
    size_t num_iters;
    int errors;
} worker_args;

static void *worker(void *arg) {
    worker_args *args = arg;
    unsigned int seed = 42u + (unsigned int)args->tid;
    const int MAX_SIZE = 4096;

    for (size_t i = 0; i < args->num_iters; ++i) {
        size_t sz = 1 + rand_r(&seed) % MAX_SIZE;
        void *p = malloc(sz);

        if (!p) {
            printf("Thread %d: malloc failed at iter %zu\n", args->tid, i);
            args->errors++;
            break;
        }
        
        free(p);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (argc >= 2) {
        NUM_THREADS = atoi(argv[1]);

        if (NUM_THREADS <= 0) {
            fprintf(stderr, "Invalid num_threads: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (argc == 3) {
        NUM_ITERS = strtoull(argv[2], NULL, 10);
        
        if (NUM_ITERS == 0) {
            fprintf(stderr, "Invalid num_iters: %s\n", argv[2]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("Multithreaded test\n");
    printf("    # threads = %d, # iters per thread = %zu\n", NUM_THREADS, NUM_ITERS);

    pthread_t *threads = calloc(NUM_THREADS, sizeof(pthread_t));
    worker_args *args = calloc(NUM_THREADS, sizeof(worker_args));
    
    if (!threads || !args) {
        fprintf(stderr, "Failed to allocate memory for thread data\n");
        free(threads);
        free(args);
        return 1;
    }

    int errors = 0;
    int created = 0;

    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].tid = i;
        args[i].num_iters = NUM_ITERS;

        int status = pthread_create(&threads[i], NULL, worker, &args[i]);
        
        if (status != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", status);
            errors++;
            break;
        }
        
        created++;
    }

    // Wait for the threads to finish
    for (int i = 0; i < created; ++i) {
        int status = pthread_join(threads[i], NULL);
        
        if (status != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        
        errors += args[i].errors;
    }

    free(threads);
    free(args);

    if (errors > 0) {
        printf("[parallel]: FAILED (errors = %d)\n", errors);
        return 1;
    }
    
    printf("[parallel]: PASSED ✅\n");
    return 0;
}
