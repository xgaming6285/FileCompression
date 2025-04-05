/**
 * Large File Utilities
 * Functions and structures for handling files larger than available RAM
 */
#ifndef LARGE_FILE_UTILS_H
#define LARGE_FILE_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Default chunk size (1MB)
#define DEFAULT_CHUNK_SIZE (1024 * 1024)

// Structure for large file processing
typedef struct {
    FILE* file;
    char* filename;
    uint64_t file_size;
    uint64_t current_position;
    size_t chunk_size;
    uint8_t* buffer;
    int eof_reached;
} LargeFileReader;

// Structure for output file
typedef struct {
    FILE* file;
    char* filename;
    uint64_t bytes_written;
    size_t chunk_size;
    uint8_t* buffer;
    size_t buffer_pos;
} LargeFileWriter;

// Initialize a large file reader
LargeFileReader* large_file_reader_init(const char* filename, size_t chunk_size);

// Free resources used by a large file reader
void large_file_reader_free(LargeFileReader* reader);

// Read the next chunk from a file
// Returns: Pointer to buffer containing data, or NULL on error
// Sets bytes_read to the number of bytes read
uint8_t* large_file_reader_next_chunk(LargeFileReader* reader, size_t* bytes_read);

// Reset reader to the beginning of the file
int large_file_reader_reset(LargeFileReader* reader);

// Initialize a large file writer
LargeFileWriter* large_file_writer_init(const char* filename, size_t chunk_size);

// Free resources used by a large file writer
void large_file_writer_free(LargeFileWriter* writer);

// Write data to the output file
int large_file_writer_write(LargeFileWriter* writer, const uint8_t* data, size_t size);

// Flush any remaining data in the buffer to the file
int large_file_writer_flush(LargeFileWriter* writer);

#endif /* LARGE_FILE_UTILS_H */ 