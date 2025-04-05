/**
 * Parallel implementation of LZ77 Compression Algorithm
 * Header file defining function prototypes
 */
#ifndef LZ77_PARALLEL_H
#define LZ77_PARALLEL_H

// Function to compress a file using parallel LZ77 algorithm
int compress_lz77_parallel(const char *input_file, const char *output_file);

// Function to decompress a file using parallel LZ77 algorithm
int decompress_lz77_parallel(const char *input_file, const char *output_file);

#endif // LZ77_PARALLEL_H 