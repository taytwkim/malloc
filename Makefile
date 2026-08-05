CC = cc

# We don't link against libraries when compiling individual source files, 
# but -pthread is still often added at compile time because it may enable 
# thread-related compiler behavior.

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

# $@ = the target
# $^ = all normal prerequisites
# $< = the first prerequisite

$(LIB_PATH): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
