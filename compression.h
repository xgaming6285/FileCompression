/**
 * Compression Utility
 * Header file defining the interface for compression algorithms
 */
#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "huffman.h"  // Include Huffman header file
#include "large_file_utils.h" // Include large file utilities

// Maximum number of threads
#define MAX_THREADS 64

// Default number of threads (0 = auto-detect)
#define DEFAULT_THREADS 0

// Compression algorithm types
typedef enum {
    HUFFMAN = 0,
    RLE = 1,
    HUFFMAN_PARALLEL = 2,
    RLE_PARALLEL = 3,
    LZ77 = 4,
    LZ77_PARALLEL = 5,
    LZ77_ENCRYPTED = 6,
    PROGRESSIVE = 7  // New progressive format
} CompressionType;

// Function pointer types for compression and decompression
typedef int (*CompressFunc)(const char*, const char*);
typedef int (*DecompressFunc)(const char*, const char*);

// Compression algorithm structure
typedef struct {
    const char* name;         // Algorithm name
    const char* description;  // Algorithm description
    const char* extension;    // File extension
    CompressFunc compress;    // Compression function
    DecompressFunc decompress; // Decompression function
} CompressionAlgorithm;

// Profiling data structure
typedef struct {
    const char* operation_name;
    clock_t start_time;
    clock_t end_time;
    double elapsed_time;
} ProfileData;

// Function prototypes
void init_compression_algorithms();
int get_algorithm_count();
CompressionAlgorithm* get_algorithm(int index);
#define get_algorithm_by_index get_algorithm
CompressionAlgorithm* get_algorithm_by_type(CompressionType type);
void print_available_algorithms();
int get_thread_count();
void set_thread_count(int count);
int get_optimal_threads();

// Get file extension for an algorithm
const char* get_algorithm_extension(int algorithm_index);
// Get algorithm name
const char* get_algorithm_name(int algorithm_index);

// Profiling functions
void start_profiling(ProfileData* profile, const char* operation);
void end_profiling(ProfileData* profile);
void print_profiling_results(ProfileData* profile);

// Large file processing support
int compress_large_file(const char* input_file, const char* output_file, size_t chunk_size);
int decompress_large_file(const char* input_file, const char* output_file, size_t chunk_size);

// Buffer-based compression/decompression interface
int compress_buffer(int algorithm_index, const uint8_t* input, size_t input_size, 
                   uint8_t* output, size_t* output_size);
int decompress_buffer(int algorithm_index, const uint8_t* input, size_t input_size, 
                     uint8_t* output, size_t* output_size);

// High level file compression/decompression functions
int compress_file_with_algorithm(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type);
int decompress_file_with_algorithm(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type);

// Detect algorithm from file extension
int detect_algorithm_from_file(const char* filename);

#endif // COMPRESSION_H 