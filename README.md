# Memory Allocator

Implement a memory allocator from scratch in C that can be dynamically interposed in Linux codebases.

## Getting Started

1. Build `libtaymalloc.so`.

```bash
make
```

2. Compile target code, then use `LD_PRELOAD` to replace the system's default `malloc` with `taymalloc`.

```bash
# Compile target code
gcc tests/hello.c -o build/hello

# Load taymalloc
LD_PRELOAD=./build/libtaymalloc.so ./build/hello
```

For quick tests on non-Linux platforms, use `docker_run.sh` to spin up a Linux container and run tests.

```bash
./scripts/docker_run.sh tests/hello.c

# Pass in environment variables
./scripts/docker_run.sh tests/hello.c TAYMALLOC_VERBOSE=1
```
