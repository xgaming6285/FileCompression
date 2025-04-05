/**
 * Parallel implementation of LZ77 Compression Algorithm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lz77.h"
#include "parallel.h"

// Function to compress a file using parallel LZ77 algorithm
int compress_lz77_parallel(const char *input_file, const char *output_file) {
    return compress_file_parallel(input_file, output_file, 
        get_algorithm_by_type(LZ77), get_thread_count());
}

// Function to decompress a file using parallel LZ77 algorithm
int decompress_lz77_parallel(const char *input_file, const char *output_file) {
    return decompress_file_parallel(input_file, output_file, 
        get_algorithm_by_type(LZ77), get_thread_count());
} 