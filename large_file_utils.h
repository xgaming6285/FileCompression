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

// Checksum type enumeration
typedef enum {
    CHECKSUM_NONE = 0,   // No checksum
    CHECKSUM_CRC32 = 1,  // CRC32 checksum
    CHECKSUM_MD5 = 2,    // MD5 hash
    CHECKSUM_SHA256 = 3  // SHA256 hash
} ChecksumType;

// Checksum data structure
typedef struct {
    ChecksumType type;    // Type of checksum
    uint32_t crc32;       // CRC32 value (if used)
    uint8_t md5[16];      // MD5 hash (if used)
    uint8_t sha256[32];   // SHA256 hash (if used)
} ChecksumData;

// Structure for large file processing
typedef struct {
    FILE* file;
    char* filename;
    uint64_t file_size;
    uint64_t current_position;
    size_t chunk_size;
    uint8_t* buffer;
    int eof_reached;
    ChecksumType checksum_type; // Type of checksum to use/verify
} LargeFileReader;

// Structure for output file
typedef struct {
    FILE* file;
    char* filename;
    uint64_t bytes_written;
    size_t chunk_size;
    uint8_t* buffer;
    size_t buffer_pos;
    ChecksumType checksum_type; // Type of checksum to use
} LargeFileWriter;

// Initialize a large file reader
LargeFileReader* large_file_reader_init(const char* filename, size_t chunk_size);

// Initialize a large file reader with checksum verification
LargeFileReader* large_file_reader_init_with_checksum(const char* filename, size_t chunk_size, ChecksumType checksum_type);

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

// Initialize a large file writer with checksum generation
LargeFileWriter* large_file_writer_init_with_checksum(const char* filename, size_t chunk_size, ChecksumType checksum_type);

// Free resources used by a large file writer
void large_file_writer_free(LargeFileWriter* writer);

// Write data to the output file
int large_file_writer_write(LargeFileWriter* writer, const uint8_t* data, size_t size);

// Flush any remaining data in the buffer to the file
int large_file_writer_flush(LargeFileWriter* writer);

// Calculate checksums for a buffer
void calculate_checksum(const uint8_t* data, size_t size, ChecksumData* checksum, ChecksumType type);

// Verify data against a checksum
int verify_checksum(const uint8_t* data, size_t size, const ChecksumData* checksum);

// Get the size of the checksum data for the specified type
size_t get_checksum_size(ChecksumType type);

// Get a string representation of the checksum
char* checksum_to_string(const ChecksumData* checksum, char* buffer, size_t buffer_size);

#endif /* LARGE_FILE_UTILS_H */ 