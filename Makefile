CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fPIC -pthread -fno-builtin
LDFLAGS = -pthread

LIB_SOURCES = $(wildcard src/*.c)
LIB_OBJECTS = $(patsubst src/%.c, build/%.o, $(LIB_SOURCES))
LIB_PATH = build/libtaymalloc.so

TEST_SOURCES = $(wildcard tests/*.c)
TEST_EXECUTABLES = $(patsubst tests/%.c, build/%, $(TEST_SOURCES))

.PHONY: all build_test run_test clean

all: $(LIB_PATH)

$(LIB_PATH): $(LIB_OBJECTS)
	$(CC) -shared $(LDFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build_test: $(TEST_EXECUTABLES)

run_test: build_test
	@set -e; \
	for test in $(TEST_EXECUTABLES); do \
		echo "Running $$test"; \
		./$$test; \
		echo; \
	done

# Tests link directly with the allocator objects.
$(TEST_EXECUTABLES): build/%: tests/%.c $(LIB_OBJECTS) | build
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Build directory and cleanup
build:
	mkdir -p $@

clean:
	rm -rf build
