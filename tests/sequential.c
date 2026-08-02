#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int aligned16(void *p) {
    return ((uintptr_t)p & 15u) == 0; 
}

static void test_alignment(void) {
    void *a = malloc(1);
    void *b = malloc(17);
    void *c = malloc(4096);

    assert(a && b && c);
    assert(aligned16(a));
    assert(aligned16(b));
    assert(aligned16(c));

    free(a);
    free(b);
    free(c);
}

static void test_null_and_zero(void) {
    free(NULL);            // free(NULL) should be a no-op
    void *p = malloc(0);   // should be NULL
    assert(p == NULL);
}

static void test_big_chunk_alloc(void) {
    void *p = malloc(16777217);
    
    // If we don't touch p, compiler optimization might remove malloc!
    if (p) {
        printf("        allocated 16777217 bytes at address: %p\n", p);
    } 
    else {
        printf("        malloc failed\n");
    }

    free(p);
}

static void test_unmap_heap(void){
    void *p = malloc(16);       // map first heap
    void *q = malloc(16777217); // map second heap by allocating big chunk
    free(p);                    // should unmap the first heap
    free(q);
}

static void test_churn(void) {
    enum { N = 4 };
    void *arr[N] = {0};

    // Allocate a mix of sizes
    for (int i = 0; i < N; ++i) {
        size_t sz = 1 + (i % 64);    // 1..64 bytes
        arr[i] = malloc(sz);
        assert(arr[i]);
        assert(aligned16(arr[i]));
    }

    // Free every third block to fragment the freelist
    for (int i = 0; i < N; i += 3) {
        free(arr[i]);
        arr[i] = NULL;
    }

    // Reuse: allocate & free a bunch of 64B payloads
    for (int i = 0; i < N; ++i) {
        void *p = malloc(64);
        assert(p && aligned16(p));
        free(p);
    }

    // Free the rest
    for (int i = 0; i < N; ++i) {
        if (arr[i]) free(arr[i]);
    }
}

int main(void){
    printf("[*] test_alignment...\n");
    test_alignment();
    
    printf("[*] test_null_and_zero...\n");
    test_null_and_zero();
    
    printf("[*] test_big_chunk_alloc...\n");
    test_big_chunk_alloc();

    printf("[*] test_unmap_heap...\n");
    test_unmap_heap();

    printf("[*] test_churn...\n");
    test_churn();

    printf("OK: all tests passed ✅\n");
    
    return 0;
}
