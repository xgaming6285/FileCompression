/**
 * Compression Utility
 * Implementation file for various compression algorithms
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compression.h"
#include "huffman.h"
#include "rle.h"
#include "parallel.h"
#include "lz77.h"          // Add LZ77 header
#include "lz77_parallel.h" // Add LZ77 parallel header
#include "encryption.h"    // Add encryption header
#include "progressive.h"   // Add progressive header
#include "filecompressor.h" // For optimization settings

// Function declarations for external functions
extern const char* get_encryption_key();

// Array of available compression algorithms
static CompressionAlgorithm algorithms[10];
static int algorithm_count = 0;
static int thread_count = DEFAULT_THREADS;

// Wrapper functions for parallel compression
int compress_huffman_parallel(const char *input_file, const char *output_file) {
    CompressionAlgorithm *huffman = get_algorithm_by_type(HUFFMAN);
    return compress_file_parallel(input_file, output_file, huffman, thread_count);
}

int decompress_huffman_parallel(const char *input_file, const char *output_file) {
    CompressionAlgorithm *huffman = get_algorithm_by_type(HUFFMAN);
    return decompress_file_parallel(input_file, output_file, huffman, thread_count);
}

int compress_rle_parallel(const char *input_file, const char *output_file) {
    CompressionAlgorithm *rle = get_algorithm_by_type(RLE);
    return compress_file_parallel(input_file, output_file, rle, thread_count);
}

int decompress_rle_parallel(const char *input_file, const char *output_file) {
    CompressionAlgorithm *rle = get_algorithm_by_type(RLE);
    return decompress_file_parallel(input_file, output_file, rle, thread_count);
}

// Wrapper functions for encrypted compression
int compress_encrypted_lz77(const char *input_file, const char *output_file) {
    // Get the encryption key from the user input
    const char *key = get_encryption_key();
    return compress_and_encrypt(input_file, output_file, key);
}

int decompress_encrypted_lz77(const char *input_file, const char *output_file) {
    // Get the encryption key from the user input
    const char *key = get_encryption_key();
    return decrypt_and_decompress(input_file, output_file, key);
}

// Wrapper functions for progressive format
int compress_progressive(const char *input_file, const char *output_file) {
    // Default to Huffman algorithm for progressive compression and no checksum
    return progressive_compress_file(input_file, output_file, CHECKSUM_NONE);
}

int decompress_progressive(const char *input_file, const char *output_file) {
    return progressive_decompress_file(input_file, output_file);
}

// Initialize available compression algorithms
void init_compression_algorithms() {
    algorithm_count = 0;
    
    // Add Huffman coding algorithm
    algorithms[algorithm_count].name = "Huffman";
    algorithms[algorithm_count].description = "Huffman coding (good compression ratio)";
    algorithms[algorithm_count].extension = ".huf";
    algorithms[algorithm_count].compress = compress_file;
    algorithms[algorithm_count].decompress = decompress_file;
    algorithm_count++;
    
    // Add RLE algorithm
    algorithms[algorithm_count].name = "RLE";
    algorithms[algorithm_count].description = "Run-Length Encoding (fast, good for repetitive data)";
    algorithms[algorithm_count].extension = ".rle";
    algorithms[algorithm_count].compress = compress_rle;
    algorithms[algorithm_count].decompress = decompress_rle;
    algorithm_count++;
    
    // Add parallel Huffman coding algorithm
    algorithms[algorithm_count].name = "Huffman-Parallel";
    algorithms[algorithm_count].description = "Parallel Huffman coding (uses multiple threads)";
    algorithms[algorithm_count].extension = ".hufp";
    algorithms[algorithm_count].compress = compress_huffman_parallel;
    algorithms[algorithm_count].decompress = decompress_huffman_parallel;
    algorithm_count++;
    
    // Add parallel RLE algorithm
    algorithms[algorithm_count].name = "RLE-Parallel";
    algorithms[algorithm_count].description = "Parallel Run-Length Encoding (uses multiple threads)";
    algorithms[algorithm_count].extension = ".rlep";
    algorithms[algorithm_count].compress = compress_rle_parallel;
    algorithms[algorithm_count].decompress = decompress_rle_parallel;
    algorithm_count++;
    
    // Add LZ77 algorithm
    algorithms[algorithm_count].name = "LZ77";
    algorithms[algorithm_count].description = "Lempel-Ziv 77 (excellent compression ratio)";
    algorithms[algorithm_count].extension = ".lz77";
    algorithms[algorithm_count].compress = compress_lz77;
    algorithms[algorithm_count].decompress = decompress_lz77;
    algorithm_count++;
    
    // Add parallel LZ77 algorithm
    algorithms[algorithm_count].name = "LZ77-Parallel";
    algorithms[algorithm_count].description = "Parallel Lempel-Ziv 77 (excellent compression ratio with multiple threads)";
    algorithms[algorithm_count].extension = ".lz77p";
    algorithms[algorithm_count].compress = compress_lz77_parallel;
    algorithms[algorithm_count].decompress = decompress_lz77_parallel;
    algorithm_count++;
    
    // Add encrypted LZ77 algorithm
    algorithms[algorithm_count].name = "LZ77-Encrypted";
    algorithms[algorithm_count].description = "Encrypted LZ77 (compression with encryption for security)";
    algorithms[algorithm_count].extension = ".lz77e";
    algorithms[algorithm_count].compress = compress_encrypted_lz77;
    algorithms[algorithm_count].decompress = decompress_encrypted_lz77;
    algorithm_count++;
    
    // Add Progressive format algorithm
    algorithms[algorithm_count].name = "Progressive";
    algorithms[algorithm_count].description = "Progressive format (supports partial decompression and streaming)";
    algorithms[algorithm_count].extension = ".prog";
    algorithms[algorithm_count].compress = compress_progressive;
    algorithms[algorithm_count].decompress = decompress_progressive;
    algorithm_count++;
    
    // Initialize the parallel subsystem
    init_parallel_compression(thread_count);
}

// Get number of available algorithms
int get_algorithm_count() {
    return algorithm_count;
}

// Get algorithm by index
CompressionAlgorithm* get_algorithm(int index) {
    if (index >= 0 && index < algorithm_count) {
        return &algorithms[index];
    }
    return NULL;
}

// Get algorithm by type
CompressionAlgorithm* get_algorithm_by_type(CompressionType type) {
    if ((int)type >= 0 && (int)type < algorithm_count) {
        return &algorithms[type];
    }
    return NULL;
}

// Print available algorithms
void print_available_algorithms() {
    printf("Available compression algorithms:\n");
    for (int i = 0; i < algorithm_count; i++) {
        printf("  %d: %s - %s\n", i, algorithms[i].name, algorithms[i].description);
    }
}

// Get the number of threads to use
int get_thread_count() {
    return thread_count;
}

// Set the number of threads to use
void set_thread_count(int count) {
    if (count > 0 && count <= MAX_THREADS) {
        thread_count = count;
    } else if (count <= 0) {
        // Auto-detect optimal thread count
        thread_count = get_optimal_threads();
    } else {
        thread_count = MAX_THREADS;
    }
}

// Profiling functions implementation
void start_profiling(ProfileData* profile, const char* operation) {
    if (!profile) return;
    
    profile->operation_name = operation;
    profile->start_time = clock();
    profile->end_time = 0;
    profile->elapsed_time = 0.0;
    
    printf("Starting profiling for: %s\n", operation);
}

void end_profiling(ProfileData* profile) {
    if (!profile || profile->start_time == 0) return;
    
    profile->end_time = clock();
    profile->elapsed_time = ((double)(profile->end_time - profile->start_time)) / CLOCKS_PER_SEC;
}

void print_profiling_results(ProfileData* profile) {
    if (!profile || profile->end_time == 0) return;
    
    printf("Profiling results for: %s\n", profile->operation_name);
    printf("  Elapsed time: %.6f seconds\n", profile->elapsed_time);
}

// Get algorithm extension by index
const char* get_algorithm_extension(int algorithm_index) {
    CompressionAlgorithm* algorithm = get_algorithm(algorithm_index);
    if (algorithm) {
        return algorithm->extension;
    }
    return ".dat"; // Default extension
}

// Get algorithm name by index
const char* get_algorithm_name(int algorithm_index) {
    CompressionAlgorithm* algorithm = get_algorithm(algorithm_index);
    if (algorithm) {
        return algorithm->name;
    }
    return "Unknown"; // Default name
}

// Simple algorithm detection from file extension
int detect_algorithm_from_file(const char* filename) {
    if (!filename) return -1;
    
    // Find the last dot in the filename
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return -1;
    
    // Compare with known extensions
    for (int i = 0; i < algorithm_count; i++) {
        if (strcmp(dot, algorithms[i].extension) == 0) {
            return i;
        }
    }
    
    // Special case for progressive format
    if (strcmp(dot, ".prog") == 0) {
        return PROGRESSIVE;
    }
    
    return -1; // Unknown format
}

// Buffer-based compression
int compress_buffer(int algorithm_index, const uint8_t* input, size_t input_size, 
                   uint8_t* output, size_t* output_size) {
    // This is a simplified implementation - in a real implementation,
    // we would have buffer-based versions of each compression algorithm
    if (algorithm_index == HUFFMAN) {
        // Call Huffman buffer-based compression
        // For now, we'll just copy data as a placeholder
        if (*output_size < input_size) {
            fprintf(stderr, "Output buffer too small\n");
            return 0;
        }
        memcpy(output, input, input_size);
        *output_size = input_size;
        return 1;
    }
    else if (algorithm_index == RLE) {
        // Call RLE buffer-based compression
        // For now, we'll just copy data as a placeholder
        if (*output_size < input_size) {
            fprintf(stderr, "Output buffer too small\n");
            return 0;
        }
        memcpy(output, input, input_size);
        *output_size = input_size;
        return 1;
    }
    // Add similar cases for other algorithms
    
    fprintf(stderr, "Unsupported algorithm for buffer compression\n");
    return 0;
}

// Buffer-based decompression
int decompress_buffer(int algorithm_index, const uint8_t* input, size_t input_size, 
                     uint8_t* output, size_t* output_size) {
    // This is a simplified implementation - in a real implementation,
    // we would have buffer-based versions of each decompression algorithm
    if (algorithm_index == HUFFMAN) {
        // Call Huffman buffer-based decompression
        // For now, we'll just copy data as a placeholder
        if (*output_size < input_size) {
            fprintf(stderr, "Output buffer too small\n");
            return 0;
        }
        memcpy(output, input, input_size);
        *output_size = input_size;
        return 1;
    }
    else if (algorithm_index == RLE) {
        // Call RLE buffer-based decompression
        // For now, we'll just copy data as a placeholder
        if (*output_size < input_size) {
            fprintf(stderr, "Output buffer too small\n");
            return 0;
        }
        memcpy(output, input, input_size);
        *output_size = input_size;
        return 1;
    }
    // Add similar cases for other algorithms
    
    fprintf(stderr, "Unsupported algorithm for buffer decompression\n");
    return 0;
}

// High-level file compression function
int compress_file_with_algorithm(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type) {
    (void)checksum_type; // Mark as unused for now
    
    if (algorithm_index < 0 || algorithm_index >= algorithm_count) {
        fprintf(stderr, "Invalid algorithm index: %d\n", algorithm_index);
        return 0;
    }
    
    CompressionAlgorithm* algorithm = get_algorithm(algorithm_index);
    if (!algorithm) {
        fprintf(stderr, "Failed to get algorithm with index %d\n", algorithm_index);
        return 0;
    }
    
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Could not open input file %s\n", input_file);
        return 0;
    }
    
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Could not open output file %s\n", output_file);
        fclose(input);
        return 0;
    }
    
    // Use the algorithm's compression function directly
    int result = 0;
    
    // Close files before calling the algorithm function, as it will open them again
    fclose(input);
    fclose(output);
    
    // Call the algorithm's compression function
    result = algorithm->compress(input_file, output_file);
    
    return result;
}

// High-level file decompression function
int decompress_file_with_algorithm(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type) {
    (void)checksum_type; // Mark as unused for now
    
    if (algorithm_index < 0 || algorithm_index >= algorithm_count) {
        fprintf(stderr, "Invalid algorithm index: %d\n", algorithm_index);
        return 0;
    }
    
    CompressionAlgorithm* algorithm = get_algorithm(algorithm_index);
    if (!algorithm) {
        fprintf(stderr, "Failed to get algorithm with index %d\n", algorithm_index);
        return 0;
    }
    
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Could not open input file %s\n", input_file);
        return 0;
    }
    
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Could not open output file %s\n", output_file);
        fclose(input);
        return 0;
    }
    
    // Use the algorithm's decompression function directly
    int result = 0;
    
    // Close files before calling the algorithm function, as it will open them again
    fclose(input);
    fclose(output);
    
    // Call the algorithm's decompression function
    result = algorithm->decompress(input_file, output_file);
    
    return result;
} 