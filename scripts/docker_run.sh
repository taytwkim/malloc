#!/bin/bash

# script fails fast instead of silently continuing
# -e            exit immediately if a command fails.
# -u            treat unset variables as errors.
# -o pipefail   in a pipeline, fail if any command fails, not just the last one.
set -euo pipefail

IMAGE_NAME="taymalloc-linux-dev"
CONTAINER_WORKDIR="/workspace"

usage() {
    echo "Usage: $0 <test-source.c>"
    echo "Example: $0 tests/hello.c TAYMALLOC_VERBOSE=1 ..."
}

if [[ $# -lt 1 ]]; then
    usage
    exit 1
fi

TEST_SOURCE="$1"
shift
EXTRA_ENVS=("$@")

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is not installed or not available in PATH" >&2
    exit 1
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ ! -f "$REPO_ROOT/$TEST_SOURCE" ]]; then
    echo "test source not found: $TEST_SOURCE" >&2
    exit 1
fi

TEST_BASENAME="$(basename "$TEST_SOURCE")"
TEST_NAME="${TEST_BASENAME%.c}"
EXTRA_ENV_STRING="${EXTRA_ENVS[*]:-}"

docker build -t "$IMAGE_NAME" "$REPO_ROOT"

docker run --rm \
    -v "$REPO_ROOT:$CONTAINER_WORKDIR" \
    -w "$CONTAINER_WORKDIR" \
    "$IMAGE_NAME" \
    bash -lc "
        set -euo pipefail
        TMP_WORKDIR=/tmp/taymalloc
        rm -rf \"\$TMP_WORKDIR\"
        mkdir -p \"\$TMP_WORKDIR\"
        cp -R . \"\$TMP_WORKDIR\"
        cd \"\$TMP_WORKDIR\"
        make clean
        make
        mkdir -p build
        gcc -std=c11 -Wall -Wextra -O2 -Isrc -D_GNU_SOURCE \"$TEST_SOURCE\" -o \"build/$TEST_NAME\" -lpthread -fopenmp
        echo
        echo \"Running build/$TEST_NAME with LD_PRELOAD=./build/libtaymalloc.so\"
        TAYMALLOC_INJECTED=1 ${EXTRA_ENV_STRING} LD_PRELOAD=./build/libtaymalloc.so \"./build/$TEST_NAME\"
    "
