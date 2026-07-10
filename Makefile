CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -pthread
LDFLAGS = -shared -pthread

UNAME_S := $(shell uname -s)

SRCS = src/arena.c src/freelist.c src/heap.c src/malloc.c src/config.c src/platform.c

# substitute src/%.c to build/%.o
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

ifeq ($(UNAME_S), Darwin)
LIB_NAME = libtaymalloc.dylib
else
LIB_NAME = libtaymalloc.so
endif

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
