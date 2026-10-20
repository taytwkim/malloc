#include "tcache.h"

#include "chunk.h"
#include "util.h"

typedef struct tcache_bin {
    int count;
    free_chunk_prefix_t *head;
} tcache_bin_t;

static _Thread_local tcache_bin_t g_tcache[TCACHE_MAX_BINS];

static int get_tcache_bin_index(size_t chunk_size) {
    size_t units = chunk_size / 16;
    size_t min_units = MIN_CHUNK_SIZE / 16;

    if (units < min_units || units - min_units >= TCACHE_MAX_BINS) {
        return -1;
    }

    return (int)(units - min_units);
}

void *tcache_try_pop(size_t chunk_size) {
    int bin = get_tcache_bin_index(chunk_size);

    if (bin < 0) {
        return NULL;   
    }

    safe_log_msg("[malloc]: searching tcache\n");

    tcache_bin_t *b = &g_tcache[bin];

    if (b->head == NULL) {
        return NULL;
    }

    free_chunk_prefix_t *fc = b->head;
    b->head = fc->next;
    b->count--;

    return fc;
}

int tcache_try_push(void *hdr) {
    int bin = get_tcache_bin_index(chunk_get_size(hdr));

    if (bin < 0) {
        return 0;   
    }

    safe_log_msg("[free]: free to tcache\n");

    tcache_bin_t *b = &g_tcache[bin];

    if (b->count >= TCACHE_MAX_COUNT) {
        return 0;
    }

    // Keep the chunk in-use from the arena's perspective so neighbors cannot
    // coalesce with it. Its address still identifies the owning heap/arena.
    free_chunk_prefix_t *fc = hdr;
    fc->next = b->head;
    b->head = fc;
    b->count++;

    return 1;
}
