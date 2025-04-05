/**
 * Parallel Compression Utilities
 * Header file for parallel processing functions
 */
#ifndef PARALLEL_H
#define PARALLEL_H

#include <stddef.h>
#include "compression.h"

// Function prototypes
void init_parallel_compression(int thread_count);
int get_optimal_threads();

// Parallel compression and decompression functions
int compress_file_parallel(const char *input_file, const char *output_file, 
                          CompressionAlgorithm *algorithm, int thread_count);
int decompress_file_parallel(const char *input_file, const char *output_file, 
                            CompressionAlgorithm *algorithm, int thread_count);

#endif // PARALLEL_H 