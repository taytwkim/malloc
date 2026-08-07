#!/bin/bash

set -euo pipefail

NUM_THREADS="${1:-4}"
NUM_ITERS="${2:-1000000}"
NUM_RUNS="${3:-5}"
LIB_PATH="./build/libtaymalloc.so"
PRELOAD_VAR="LD_PRELOAD"
BENCH_BIN="./build/parallel"

if [[ ! -x "$BENCH_BIN" ]]; then
    echo "missing benchmark binary: $BENCH_BIN" >&2
    exit 1
fi

if [[ ! -f "$LIB_PATH" ]]; then
    echo "missing allocator library: $LIB_PATH" >&2
    exit 1
fi

run_case() {
    local label="$1"
    shift

    echo
    echo "=== $label ==="

    for ((run = 1; run <= NUM_RUNS; run++)); do
        echo "[run $run/$NUM_RUNS]"
        time -p env "$PRELOAD_VAR=$LIB_PATH" "$@" "$BENCH_BIN" "$NUM_THREADS" "$NUM_ITERS"
        echo
    done
}

echo "benchmark config:"
echo "  threads = $NUM_THREADS"
echo "  iterations per thread = $NUM_ITERS"
echo "  runs per case = $NUM_RUNS"

run_case "single_heap baseline" TAYMALLOC_DISABLE_ARENAS=1 TAYMALLOC_DISABLE_TCACHE=1
run_case "multi_heap" TAYMALLOC_DISABLE_TCACHE=1
run_case "multi_heap + tcache"
