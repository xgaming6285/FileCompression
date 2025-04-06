/**
 * Progressive Compression/Decompression
 * Provides functionality for partial decompression and streaming of compressed files
 */
#ifndef PROGRESSIVE_H
#define PROGRESSIVE_H

#include <stdio.h>
#include <stdint.h>
#include "large_file_utils.h"
#include "compression.h"

// Default block size for progressive compression (1MB)
#define DEFAULT_BLOCK_SIZE (1024 * 1024)
// Maximum block size (16MB)
#define MAX_BLOCK_SIZE (16 * 1024 * 1024)

// Magic number for progressive files
#define MAGIC_NUMBER "PROG"
// Current version of the progressive format
#define CURRENT_VERSION 1

// Flags for progressive format
#define FLAG_HAS_CHECKSUM       0x01
#define FLAG_STREAMING_OPTIMIZED 0x02
#define FLAG_ENCRYPTED          0x04

// Temporary file for block decompression
#define TEMP_OUTPUT_FILE "temp_output.dat"

// Progressive compression file header
typedef struct {
    char magic[4];              // Magic number "PROG"
    uint8_t version;            // Version number
    uint8_t algorithm;          // Compression algorithm used
    uint8_t flags;              // Various flags (encrypted, checksum, etc.)
    uint32_t block_size;        // Size of each compressed block
    uint32_t total_blocks;      // Total number of blocks
    uint64_t original_size;     // Original file size
    ChecksumData checksum;      // File checksum (if used)
} ProgressiveHeader;

// Progressive file block header
typedef struct {
    uint32_t block_id;          // Block identifier (sequence number)
    uint32_t compressed_size;   // Size of compressed data
    uint32_t original_size;     // Original size before compression 
    ChecksumData block_checksum; // Checksum for this block (if used)
} BlockHeader;

// Progressive decompression context
typedef struct {
    FILE* file;                 // Input file handle
    char* filename;             // Input filename
    ProgressiveHeader header;   // File header
    uint64_t current_pos;       // Current file position
    uint8_t* block_buffer;      // Buffer for reading blocks
    uint8_t* output_buffer;     // Buffer for decompressed data
    void* huffman_tree;         // Huffman tree (if Huffman compression used)
    void* algorithm_context;    // Algorithm-specific context
    int last_block_id;          // Last block ID processed
    int initialized;            // Whether initialization completed
} ProgressiveContext;

// Create a new progressive context
ProgressiveContext* progressive_init(const char* filename);

// Free a progressive context
void progressive_free(ProgressiveContext* context);

// Decompress a specific block by ID
// Returns decompressed data size or -1 on error
int64_t progressive_decompress_block(ProgressiveContext* context, uint32_t block_id, uint8_t* output, size_t output_size);

// Get header information without decompressing
int progressive_get_header(const char* filename, ProgressiveHeader* header);

// Get number of blocks in a file
uint32_t progressive_get_block_count(ProgressiveContext* context);

// Get original file size
uint64_t progressive_get_original_size(ProgressiveContext* context);

// Compress a file using progressive format
int progressive_compress_file(const char* input_file, const char* output_file, ChecksumType checksum_type);

// Decompress a progressive file completely
int progressive_decompress_file(const char* input_file, const char* output_file);

// Decompress a range of blocks
int progressive_decompress_range(const char* input_file, const char* output_file, uint32_t start_block, uint32_t end_block);

// Stream processing function type (for callback-based processing)
typedef int (*StreamCallback)(const uint8_t* data, size_t size, void* user_data);

// Process a progressive file through streaming
int progressive_stream_process(const char* input_file, StreamCallback callback, void* user_data);

#endif /* PROGRESSIVE_H */ 