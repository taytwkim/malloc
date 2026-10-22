#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, free

int main(void) {
    void* p = malloc(16);
    
    // If we don't touch p, compiler optimization might remove malloc!
    if (p) {
        printf("Allocated 16 bytes at address: %p\n", p);
    } 
    else {
        printf("Malloc failed.\n");
        return 1;
    }

    free(p);
    
    printf("Hello World! ✅\n");
    
    return 0;
}
