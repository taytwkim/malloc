<h1 align="center">Design a Memory Allocator</h1>

`taymalloc` is a memory allocator for Linux and macOS that can be built as a shared library and linked into existing codebases.

While not heavily optimized for throughput or RSS, it provides safe concurrent memory allocation through lock protection. 
Per-thread arenas and thread-local caches are used to reduce contention.

## Platform Support

- Linux: supported as a preloadable allocator via `LD_PRELOAD`
- macOS: supported as a shared library allocator, with best-effort interposition for selected programs via `DYLD_INSERT_LIBRARIES`

The allocator core is written against a small POSIX platform wrapper, so Linux and macOS share the same implementation. The preload and interposition workflow still differs by OS.

## Getting Started

### Linux

1. Build `libtaymalloc.so`.

```shell
make
```

2. Compile target code, then use `LD_PRELOAD` to replace system's default `malloc` with `taymalloc`.

```shell
# Compile target code
gcc tests/hello.c -o build/hello

# Inject taymalloc
LD_PRELOAD=./build/libtaymalloc.so ./build/hello
```

### macOS

1. Build `libtaymalloc.dylib`.

```shell
make
```

2. For selected binaries, try interposing with `DYLD_INSERT_LIBRARIES`.

```shell
DYLD_INSERT_LIBRARIES=./build/libtaymalloc.dylib ./build/hello
```

macOS does not guarantee allocator interposition for every target process. The intended support level here is "works for selected programs and controlled tests," not "universal drop-in replacement for all binaries."

### CMake

You can also build with CMake on both Linux and macOS.

```shell
cmake -S . -B build-cmake
cmake --build build-cmake
```

The optional `parallel` test depends on OpenMP. On macOS with the default Apple Clang toolchain, that target may be skipped unless OpenMP support is installed separately.
