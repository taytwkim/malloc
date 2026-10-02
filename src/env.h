#ifndef TAYMALLOC_ENV_H
#define TAYMALLOC_ENV_H

typedef struct {
    int hello;
    int verbose;
    int disable_tcache;
    int disable_arenas;
} taymalloc_config_t;

extern taymalloc_config_t g_config;

void config_init(void);

#endif
