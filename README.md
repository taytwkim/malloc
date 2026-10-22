# Memory Allocator

Implement a memory allocator in C that can be dynamically interposed in Linux codebases.

## Usage

1. Build `libtaymalloc.so`.

```bash
make
```

2. Compile target code, then use `LD_PRELOAD` to replace the system's default `malloc` with `taymalloc`.

```bash
# Compile target code
gcc hello.c -o hello
LD_PRELOAD=./build/libtaymalloc.so ./hello
```

## Testing

All tests under `tests/` link directly with the allocator's object files, and do not require `LD_PRELOAD`.
Test builds require OpenMP support for the multithreaded tests.

```bash
make build_test         # Build tests
make run_test           # Build and run all tests
```