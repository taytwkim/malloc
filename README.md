<h1 align="center">Design a Memory Allocator</h1>

`taymalloc` is a Linux and macOS memory allocator library that can be dynamically linked into existing codebases.

## Usage

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

2. Interpose using `DYLD_INSERT_LIBRARIES`.

```shell
DYLD_INSERT_LIBRARIES=./build/libtaymalloc.dylib ./build/hello
```

**Note:** On macOS, allocator interposition is not guaranteed for all target processes. The macOS build is intended for selected programs and controlled test environments, rather than as a universal drop-in replacement that works in every case.

### CMake

You can also build with CMake on both Linux and macOS.

```shell
cmake -S . -B build-cmake
cmake --build build-cmake
```

The optional `parallel` test depends on OpenMP. On macOS with the default Apple Clang toolchain, the target may be skipped unless OpenMP is supported.
