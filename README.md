# Memory Allocator

`taymalloc` is a memory allocator library that can be dynamically linked into existing Linux codebases.

## Usage

1. Build `libtaymalloc.so`.

```shell
make
```

2. Compile target code, then `LD_PRELOAD` to replace system's default `malloc` with `taymalloc`.

```shell
# Compile target code
gcc tests/hello.c -o build/hello

# Inject taymalloc
LD_PRELOAD=./build/libtaymalloc.so ./build/hello
```

For quick tests on non-Linux platforms, use `docker_run.sh` to spin up a Linux container and run the tests.

```shell
./scripts/docker_run.sh tests/hello.c

# Pass in environment variables
./scripts/docker_run.sh tests/hello.c TAYMALLOC_VERBOSE=1
```
