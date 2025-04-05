/**
 * File Compression Utility
 * Main header file
 */
#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H

#include <stddef.h>

// Optimization goals
typedef enum {
    OPT_NONE,
    OPT_SPEED,
    OPT_SIZE
} OptimizationGoal;

// Global variables for configuration
extern OptimizationGoal opt_goal;

// Get optimization goal
OptimizationGoal get_optimization_goal();

// Get buffer size
size_t get_buffer_size();

// Get encryption key
const char* get_encryption_key();

// Set encryption key
void set_encryption_key(const char* key);

// Include declarations for large file support
// Huffman large file support
extern int compress_large_file(const char* input_file, const char* output_file, size_t chunk_size);
extern int decompress_large_file(const char* input_file, const char* output_file, size_t chunk_size);

#endif // FILECOMPRESSOR_H 