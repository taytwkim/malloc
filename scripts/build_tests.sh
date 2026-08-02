#!/bin/bash

set -e

mkdir -p build

CC=cc
CFLAGS="-std=c11 -Wall -Wextra -O2 -pthread"
OPENMP_FLAGS="-fopenmp"
LIB_NAME="libtaymalloc.so"
PRELOAD_VAR="LD_PRELOAD"

echo "Building test binaries..."

$CC $CFLAGS tests/hello.c -o build/hello

echo "  [Done] build/hello"

$CC $CFLAGS tests/sequential.c -o build/sequential

echo "  [Done] build/sequential"

if $CC $CFLAGS tests/parallel.c -o build/parallel $OPENMP_FLAGS; then
    echo "  [Done] build/parallel (OpenMP enabled)"
else
    echo "  [Skipped] build/parallel (OpenMP unavailable with $CC)"
fi

echo ""

echo "Compilation complete. To run with TAYMALLOC, use:"
echo "$PRELOAD_VAR=./build/$LIB_NAME ./build/hello"

echo ""

echo "To check that the symbol has been injected, set TAYMALLOC_INJECTED:"
echo "TAYMALLOC_INJECTED=1 $PRELOAD_VAR=./build/$LIB_NAME ./build/hello"

echo ""

echo "To enable logging, set TAYMALLOC_VERBOSE:"
echo "TAYMALLOC_VERBOSE=1 $PRELOAD_VAR=./build/$LIB_NAME ./build/hello"

echo ""
