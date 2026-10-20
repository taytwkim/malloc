#ifndef TAYMALLOC_ENV_H
#define TAYMALLOC_ENV_H

typedef struct {
    int hello;
    int verbose;
    int disable_tcache;
    int disable_arenas;
    int freelist_num_bins;
} taymalloc_config_t;

extern taymalloc_config_t g_env;

void config_init(void);

#endif
