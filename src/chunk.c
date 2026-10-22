#define _DEFAULT_SOURCE
#include "chunk.h"

#include <string.h>
#include <sys/mman.h>   // mmap, munmap, mapping flags

#include "util.h"

size_t get_mmapped_chunk_mmap_size(size_t chunk_size) {
    if (chunk_size > SIZE_MAX - MMAPED_CHUNK_PADDING) {
        // Catches overflow when adding padding.
        return 0;
    }

    size_t mapping_size = align_pagesize(chunk_size + MMAPED_CHUNK_PADDING);

    if (mapping_size < chunk_size + MMAPED_CHUNK_PADDING) {
        // Catches overflow when rounding up to a page size.
        return 0;
    }

    return mapping_size;
}

void *mmap_large_chunk(size_t chunk_size) {
    size_t mapping_size = get_mmapped_chunk_mmap_size(chunk_size);
    if (mapping_size == 0) return NULL;

    void *mem = mmap(NULL, mapping_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return NULL;

    inuse_chunk_prefix_t *hdr = (void *)((uint8_t *)mem + MMAPED_CHUNK_PADDING);
    chunk_write_size_to_hdr(hdr, chunk_size);
    chunk_set_M(hdr, 1);

    return hdr;
}

void munmap_large_chunk(void *hdr) {
    size_t mapping_size = get_mmapped_chunk_mmap_size(chunk_get_size(hdr));
    (void)munmap((uint8_t *)hdr - MMAPED_CHUNK_PADDING, mapping_size);
}

void chunk_copy_data(void *hdr1, size_t size1, void* hdr2, size_t size2) {
    size_t copy_size = size1 < size2 ? size1 : size2;
    copy_size -= sizeof(inuse_chunk_prefix_t);
    memcpy(chunk_hdr_to_payload(hdr2), chunk_hdr_to_payload(hdr1), copy_size);   
}