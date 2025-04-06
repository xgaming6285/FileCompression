CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -pthread
LIBS = -lm -lssl -lcrypto

# For multithreading support
CFLAGS += -pthread

# Source files
SOURCES = filecompressor.c compression.c huffman.c rle.c lz77.c encryption.c \
          parallel.c lz77_parallel.c large_file_utils.c progressive.c split_archive.c deduplication.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Executable name
EXECUTABLE = filecompressor

# Test sources
TEST_SOURCES = test_large_file.c
TEST_OBJECTS = $(TEST_SOURCES:.c=.o) large_file_utils.o
TEST_EXECUTABLE = test_large_file

# Default target
all: $(EXECUTABLE) $(TEST_EXECUTABLE)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: all

# Release build with more optimizations
release: CFLAGS += -O3 -march=native -DNDEBUG
release: all

# Link the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test executable
$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o $@ $(LDFLAGS) $(LIBS)

# Clean up
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(EXECUTABLE) $(TEST_EXECUTABLE)

# Dependencies
filecompressor.o: filecompressor.c filecompressor.h compression.h huffman.h rle.h parallel.h encryption.h large_file_utils.h progressive.h split_archive.h
compression.o: compression.c compression.h huffman.h rle.h parallel.h lz77.h lz77_parallel.h encryption.h progressive.h
huffman.o: huffman.c huffman.h
rle.o: rle.c rle.h
lz77.o: lz77.c lz77.h
lz77_parallel.o: lz77_parallel.c lz77_parallel.h lz77.h parallel.h
parallel.o: parallel.c parallel.h compression.h
encryption.o: encryption.c encryption.h
large_file_utils.o: large_file_utils.c large_file_utils.h
progressive.o: progressive.c progressive.h compression.h huffman.h lz77.h rle.h
split_archive.o: split_archive.c split_archive.h large_file_utils.h compression.h
test_large_file.o: test_large_file.c large_file_utils.h
deduplication.o: deduplication.c deduplication.h

.PHONY: all debug release clean 