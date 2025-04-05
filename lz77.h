/**
 * LZ77 Compression Algorithm Implementation
 * Header file defining data structures and function prototypes
 */
#ifndef LZ77_H
#define LZ77_H

#include <stdint.h>
#include <stdlib.h>

// Default parameters
#define DEFAULT_WINDOW_SIZE   4096  // 4KB sliding window
#define DEFAULT_LOOKAHEAD_SIZE 16    // 16-byte lookahead buffer
#define DEFAULT_MIN_MATCH     3     // Minimum match length

// Optimized parameters for speed
#define SPEED_WINDOW_SIZE     1024  // Smaller window for faster search
#define SPEED_LOOKAHEAD_SIZE  8     // Smaller lookahead buffer
#define SPEED_MIN_MATCH       4     // Require slightly longer matches to reduce search time

// Optimized parameters for size
#define SIZE_WINDOW_SIZE      8192  // Larger window for better compression
#define SIZE_LOOKAHEAD_SIZE   32    // Larger lookahead buffer
#define SIZE_MIN_MATCH        2     // Smaller minimum match for better compression

// Current parameters (will be set at runtime)
extern size_t WINDOW_SIZE;
extern size_t LOOKAHEAD_SIZE;
extern size_t MIN_MATCH;

// Token structure to represent LZ77 output
typedef struct {
    uint16_t offset;    // Offset (distance) to the start of the match in window
    uint8_t length;     // Length of the match
    uint8_t next_char;  // Next character after the match
} LZ77Token;

// Function declarations
int compress_lz77(const char *input_file, const char *output_file);
int decompress_lz77(const char *input_file, const char *output_file);

// Buffer operations
int compress_lz77_buffer(const uint8_t *input, size_t input_size, 
                        uint8_t *output, size_t *output_size);
int decompress_lz77_buffer(const uint8_t *input, size_t input_size,
                          uint8_t *output, size_t *output_size);

// Set optimization parameters based on goal
void set_lz77_optimization(int optimization_goal);

#endif // LZ77_H 