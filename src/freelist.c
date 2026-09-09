#include "freelist.h"
#include "debug.h"

void free_list_remove(arena_t *a, free_chunk_t *fc) {
    safe_log_msg("[freelist_remove]: entered\n");
    safe_log_ptr("[freelist_remove]: fc = ", fc);
    safe_log_ptr("[freelist_remove]: a->free_list = ", a->free_list);
    safe_log_ptr("[freelist_remove]: fc->prev = ", fc->prev);
    safe_log_ptr("[freelist_remove]: fc->next = ", fc->next);

    free_chunk_t *fd = fc->prev, *bk = fc->next;

    safe_log_ptr("[freelist_remove]: fd = ", fd);
    safe_log_ptr("[freelist_remove]: bk = ", bk);

    if (bk) bk->prev = fd;
    if (fd) fd->next = bk;
    if (a->free_list == fc) a->free_list = fd;
    fc->prev = fc->next = NULL;
}

void free_list_push_front(arena_t *a, free_chunk_t *fc) {
    fc->next = NULL;
    fc->prev = a->free_list;
    if (a->free_list) a->free_list->next = fc;
    a->free_list = fc;
}

void* free_list_try(arena_t *a, size_t chunk_size) {
    for (free_chunk_t *p = a->free_list; p; p = p->prev) {
        if (!chunk_is_free(p)) continue;
        if (chunk_get_size(p) >= chunk_size) {
            heap_t *h = chunk_get_heap(p);
            return heap_split_free_chunk(h, p, chunk_size);
        }
    }
    return NULL;
}
