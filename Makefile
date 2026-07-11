CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -pthread
LDFLAGS = -shared -pthread

# In a Makefile, "=" and ":=" differ in when the value gets evaluated.
# "=" uses lazy evaluation, so the value is expanded each time the variable is referenced.
# If the value contains $(shell ...), that command may run multiple times.
# ":=" uses eager evaluation, so the value is expanded once when make reads this line.
UNAME_S := $(shell uname -s)

SRCS = src/arena.c src/config.c src/freelist.c src/heap.c src/malloc.c src/platform.c

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