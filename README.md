# Memory Allocator

Memory allocator that can be dynamically interposed in Linux codebases.

The allocator follows [glibc's design](https://sourceware.org/glibc/wiki/MallocInternals) closely. Its main features are:

- Multiple arenas to improve allocation throughput in multithreaded programs.
- Free list divided into size-classes for faster lookup.
- Thread-local caches for freed chunks.
- Implementations of `malloc`, `free`, `calloc`, and `realloc`.

See `src/` for implementation. Here is a more detailed [documentation](https://taytwkim.vercel.app/blog/projects/003-malloc/).

## Usage

- Build `libtaymalloc.so`.

```bash
make
```

- Compile target code, then use `LD_PRELOAD` to replace the system's default `malloc`.

```bash
gcc hello.c -o hello
LD_PRELOAD=./build/libtaymalloc.so ./hello
```

## Testing

```bash
make build_test         # Build tests
make run_test           # Build and run all tests
```

All tests under `tests/` link directly with the allocator's object files instead of using `LD_PRELOAD`.