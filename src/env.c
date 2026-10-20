#include "env.h"

#include <stdlib.h>     // getenv
#include <string.h>     // strcmp
#include <unistd.h>     // write

#include "util.h"
#include "arena.h"

taymalloc_config_t g_env = {0};  // Zero-initialize

static int env_is_enabled(const char* name) {
    const char* value = getenv(name);
    return value && strcmp(value, "1") == 0;
}

void config_init(void) {
    if (env_is_enabled("TAYMALLOC_HELLO")) {
        char* msg = "WARNING! You are using taymalloc.\n";
        ignore_write_result(write(1, msg, safe_strlen(msg)));
        g_env.hello = 1;
    }

    if (env_is_enabled("TAYMALLOC_VERBOSE")) {
        char* msg = "Logs enabled.\n";
        ignore_write_result(write(1, msg, safe_strlen(msg)));
        g_env.verbose = 1;
    }
    
    if (env_is_enabled("TAYMALLOC_DISABLE_ARENAS")) {
        if (g_env.verbose) {
            char* msg = "Per-thread arenas disabled.\n";
            ignore_write_result(write(1, msg, safe_strlen(msg)));
        }
        g_env.disable_arenas = 1;
    }

    g_env.freelist_num_bins = DEFAULT_FREE_LIST_MAX_BINS;

    if (env_is_enabled("TAYMALLOC_DISABLE_FREELIST_SIZE_BINS")) {
        g_env.freelist_num_bins = 1;
        safe_log_msg("Freelist size bins disabled.\n");
    }

    if (env_is_enabled("TAYMALLOC_DISABLE_TCACHE")) {
        if (g_env.verbose) {
            char* msg = "Tcache disabled.\n";
            ignore_write_result(write(1, msg, safe_strlen(msg)));
        }
        g_env.disable_tcache = 1;
    }
}
