CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Source files
SRCS = filecompressor.c compression.c huffman.c rle.c lz77.c encryption.c parallel.c lz77_parallel.c large_file_utils.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = test_large_file.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BINS = $(TEST_SRCS:.c=.exe)

# Main executable
TARGET = filecompressor

all: $(TARGET) tests

# Main target rule
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Test binaries
tests: $(TEST_BINS)

%.exe: %.o $(filter-out filecompressor.o, $(OBJS))
	$(CC) $(LDFLAGS) -o $@ $^

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(TARGET) $(OBJS) $(TEST_BINS) $(TEST_OBJS)

# Debug build with symbols
debug: CFLAGS += -g -O0
debug: clean all

# Optimized build
optimize: CFLAGS += -O3 -march=native
optimize: clean all

# Help rule
help:
	@echo "Available targets:"
	@echo "  all       - Build main program and tests (default)"
	@echo "  clean     - Remove compiled files"
	@echo "  debug     - Build with debug symbols"
	@echo "  optimize  - Build with maximum optimization"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                  - Build everything with normal optimization"
	@echo "  make debug            - Build with debug symbols"
	@echo "  make optimize         - Build with maximum optimization"

.PHONY: all clean debug optimize help tests 