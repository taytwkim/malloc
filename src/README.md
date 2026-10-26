# Memory Allocator

Here’s a brief breakdown of the allocator’s components.

- `malloc.h` contains the main allocator API (`malloc`, `free`, `calloc`, and `realloc`).
- `arena.h` contains arena-related logic, including mapping and unmapping heaps.
- `heap.h` contains logic for carving new chunks from the heap bump as well as merging and splitting free chunks. While merging and splitting are free list operations, `heap.h` handles operations involving multiple chunks from the same heap.
- `chunk.h` contains chunk-related logic, including helper functions to read and write chunk metadata. It also contains functions used in the large-chunk allocation path.
- `freelist.h` implements `free_list_try`, `free_list_remove`, and `free_list_push_front`, which calls the heap operations described above.
- `tcache.h` implements `tcache_try_push` and `tcache_try_pop`.