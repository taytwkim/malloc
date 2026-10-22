# Memory Allocator

Memory allocator that can be dynamically interposed in Linux codebases.

The allocator follows glibc's design closely. Its main features are:

- Multiple arenas and heaps to improve multithreaded performance.
- Size-class-based free lists for faster chunk lookup.
- Thread-local caches for recently freed chunks.
- Implementations of `malloc`, `free`, `calloc`, and `realloc`.

See `src/` for implementation. Here is [a more detailed documentation](https://taytwkim.vercel.app/blog/projects/003-malloc/).

## Usage

1. Build `libtaymalloc.so`.

```bash
make
```

2. Compile target code, then use `LD_PRELOAD` to replace the system's default `malloc`.

```bash
gcc hello.c -o hello
LD_PRELOAD=./build/libtaymalloc.so ./hello
```

## Testing

```bash
make build_test         # Build tests
make run_test           # Build and run all tests
```

All tests under `tests/` link directly with the allocator's object files and do not require `LD_PRELOAD`.

Test builds require OpenMP support for the multi-threaded tests.