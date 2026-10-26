#include <assert.h>  // assert
#include <stdint.h>  // uintptr_t
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, free, rand, srand

static int aligned16(void *p) {
    return ((uintptr_t)p & 15u) == 0; 
}

static void test_alignment(void) {
    void* a = malloc(1);
    void* b = malloc(17);
    void* c = malloc(4096);

    assert(a && b && c);
    assert(aligned16(a));
    assert(aligned16(b));
    assert(aligned16(c));

    free(a);
    free(b);
    free(c);
}

static void test_null_and_zero(void) {
    free(NULL);             // free(NULL) should be a no-op
    void* p = malloc(0);
    assert(p == NULL);
}

static void test_big_chunk_alloc(void) {
    void* p = malloc(16777217);
    assert(p);
    printf("    allocated 16777217 bytes at address: %p\n", p);
    free(p);
}

static void test_churn(void) {
    const int N = 1000;
    const int rounds = 10;
    const int MAX_SIZE = 4096;
    void* arr[N];
    void* more[N];

    srand(42);

    for (int r = 0; r < rounds; ++r) {
        // Allocate random sizes between 1 and MAX_SIZE bytes.
        for (int i = 0; i < N; ++i) {
            size_t sz = 1 + rand() % MAX_SIZE;
            arr[i] = malloc(sz);
            assert(arr[i]);
            assert(aligned16(arr[i]));
        }

        // Toss a coin for each block to decide whether to free it.
        for (int i = 0; i < N; ++i) {
            if (rand() % 2 == 0) {
                free(arr[i]);
                arr[i] = NULL;
            }
        }

        // Keep a second batch alive alongside the surviving blocks.
        for (int i = 0; i < N; ++i) {
            size_t sz = 1 + rand() % MAX_SIZE;
            more[i] = malloc(sz);
            assert(more[i]);
            assert(aligned16(more[i]));
        }

        // Free the survivors and the entire second batch.
        for (int i = 0; i < N; ++i) {
            free(arr[i]);
            free(more[i]);
        }
    }
}

int main(void){
    printf("[*] test_alignment...\n");
    test_alignment();
    
    printf("[*] test_null_and_zero...\n");
    test_null_and_zero();
    
    printf("[*] test_big_chunk_alloc...\n");
    test_big_chunk_alloc();

    printf("[*] test_churn...\n");
    test_churn();

    printf("[sequential] ALL TESTS PASSED ✅\n");
    
    return 0;
}
