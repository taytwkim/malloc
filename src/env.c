#include "env.h"  // taymalloc_config_t, g_cfg, config_init

#include <stdlib.h>  // getenv
#include <string.h>  // strcmp
#include <unistd.h>  // write

#include "debug.h"  // ignore_write_result, safe_strlen

taymalloc_config_t g_cfg = {0};  // zero-initializes runtime settings

static int env_is_enabled(const char* name) {
    const char* value = getenv(name);
    return value && strcmp(value, "1") == 0;
}

void config_init(void) {
    if (env_is_enabled("TAYMALLOC_HELLO")) {
        char* msg = "WARNING! You are using taymalloc.\n";
        ignore_write_result(write(1, msg, safe_strlen(msg)));
        g_cfg.hello = 1;
    }

    if (env_is_enabled("TAYMALLOC_VERBOSE")) {
        char* msg = "Logs enabled.\n";
        ignore_write_result(write(1, msg, safe_strlen(msg)));
        g_cfg.verbose = 1;
    }
    
    if (env_is_enabled("TAYMALLOC_DISABLE_ARENAS")) {
        if (g_cfg.verbose) {
            char* msg = "Per-thread arenas disabled.\n";
            ignore_write_result(write(1, msg, safe_strlen(msg)));
        }
        g_cfg.disable_arenas = 1;
    }

    if (env_is_enabled("TAYMALLOC_DISABLE_TCACHE")) {
        if (g_cfg.verbose) {
            char* msg = "Tcache disabled.\n";
            ignore_write_result(write(1, msg, safe_strlen(msg)));
        }
        g_cfg.disable_tcache = 1;
    }
}
