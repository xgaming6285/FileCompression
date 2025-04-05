/**
 * Run-Length Encoding Implementation
 * Header file defining function prototypes
 */
#ifndef RLE_H
#define RLE_H

// Function to compress a file using RLE
int compress_rle(const char *input_file, const char *output_file);

// Function to decompress a file using RLE
int decompress_rle(const char *input_file, const char *output_file);

#endif // RLE_H 