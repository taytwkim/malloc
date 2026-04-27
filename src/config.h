#ifndef TAYMALLOC_CONFIG_H
#define TAYMALLOC_CONFIG_H

#include <stddef.h>

typedef struct {
    int injected;
    int verbose;
    int disable_tcache;
    int disable_arenas;
} taymalloc_config_t;

extern taymalloc_config_t g_cfg;

void config_init(void);

#endif
