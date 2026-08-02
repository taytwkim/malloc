CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -pthread
LDFLAGS = -shared -pthread

SRCS = src/arena.c src/config.c src/freelist.c src/heap.c src/malloc.c src/platform.c
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS)) # substitute src/%.c to build/%.o
LIB_NAME = libtaymalloc.so
LIB_PATH = build/$(LIB_NAME)

.PHONY: all clean

all: build $(LIB_PATH)

build:
	mkdir -p build

$(LIB_PATH): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
