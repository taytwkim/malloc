CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -D_GNU_SOURCE
LDLIBS = -lpthread

UNAME_S := $(shell uname -s)

SRCS = src/arena.c src/freelist.c src/heap.c src/malloc.c src/config.c src/platform.c
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

ifeq ($(UNAME_S),Darwin)
LIB_NAME = libtaymalloc.dylib
else
LIB_NAME = libtaymalloc.so
endif

LIB_PATH = build/$(LIB_NAME)

all: build $(LIB_PATH)

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_PATH): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDLIBS)

clean:
	rm -rf build

.PHONY: all clean
