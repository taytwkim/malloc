#define _DEFAULT_SOURCE
#include "chunk.h"

#include <sys/mman.h>   // mmap, munmap, mapping flags

#include "util.h"

void *mmap_large_chunk(size_t chunk_size) {
    if (chunk_size > SIZE_MAX - MMAPED_CHUNK_PADDING) {
        // Catches overflow when adding padding.
        return NULL;
    }

    size_t mapping_size = align_pagesize(chunk_size + MMAPED_CHUNK_PADDING);

    if (mapping_size < chunk_size + MMAPED_CHUNK_PADDING) {
        // Catches overflow when rounding up to a page size.
        return NULL;
    }

    void *mem = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return NULL;

    inuse_chunk_prefix_t *hdr = (void *)((uint8_t *)mem + MMAPED_CHUNK_PADDING);
    chunk_write_size_to_hdr(hdr, mapping_size);
    chunk_set_M(hdr, 1);

    return hdr;
}

void munmap_large_chunk(void *hdr) {
    (void)munmap((uint8_t *)hdr - MMAPED_CHUNK_PADDING, chunk_get_size(hdr));
}
