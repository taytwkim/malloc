# Memory Allocator

`taymalloc` is a memory allocator library that can be dynamically linked into existing codebases.

## Usage

Tested on Linux. Interposition support on macOS is still in progress.

1. Build `libtaymalloc.so`.

```shell
make
```

2. Compile the target code, then use `LD_PRELOAD` to replace the system's default `malloc` with `taymalloc`.

```shell
# Compile the target code
gcc tests/hello.c -o build/hello

# Inject taymalloc
LD_PRELOAD=./build/libtaymalloc.so ./build/hello
```

## Testing

For quick tests on non-Linux platforms, use `docker_run.sh` to spin up a Linux container and run the tests.

```shell
./scripts/docker_run.sh tests/hello.c

# Pass in environment variables
./scripts/docker_run.sh tests/hello.c TAYMALLOC_VERBOSE=1
```
