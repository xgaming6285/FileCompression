/**
 * LZ77 Compression Algorithm Implementation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lz77.h"

// For accessing the optimization goal
#include "filecompressor.h"  // For get_optimization_goal()

// Initialize global parameters with default values
size_t WINDOW_SIZE = DEFAULT_WINDOW_SIZE;
size_t LOOKAHEAD_SIZE = DEFAULT_LOOKAHEAD_SIZE;
size_t MIN_MATCH = DEFAULT_MIN_MATCH;

// Set LZ77 parameters based on optimization goal
void set_lz77_optimization(int optimization_goal) {
    switch(optimization_goal) {
        case OPT_SPEED:
            printf("Optimizing LZ77 for speed\n");
            WINDOW_SIZE = SPEED_WINDOW_SIZE;
            LOOKAHEAD_SIZE = SPEED_LOOKAHEAD_SIZE;
            MIN_MATCH = SPEED_MIN_MATCH;
            break;
        case OPT_SIZE:
            printf("Optimizing LZ77 for size\n");
            WINDOW_SIZE = SIZE_WINDOW_SIZE;
            LOOKAHEAD_SIZE = SIZE_LOOKAHEAD_SIZE;
            MIN_MATCH = SIZE_MIN_MATCH;
            break;
        default:
            // Use defaults
            WINDOW_SIZE = DEFAULT_WINDOW_SIZE;
            LOOKAHEAD_SIZE = DEFAULT_LOOKAHEAD_SIZE;
            MIN_MATCH = DEFAULT_MIN_MATCH;
            break;
    }
}

// Helper function to find the longest match in the window
static void find_longest_match(const uint8_t *data, size_t data_size, size_t current_pos,
                               uint16_t *match_offset, uint8_t *match_length) {
    size_t window_start = (current_pos <= WINDOW_SIZE) ? 0 : current_pos - WINDOW_SIZE;
    size_t lookahead_end = current_pos + LOOKAHEAD_SIZE;
    if (lookahead_end > data_size) {
        lookahead_end = data_size;
    }
    
    *match_offset = 0;
    *match_length = 0;
    
    // If the remaining data is too small, don't try to find a match
    if (current_pos + MIN_MATCH > data_size) {
        return;
    }
    
    // Optimization: For speed, we can implement a hash table for faster matching
    // But for now, we'll optimize the search loop
    
    // For each position in the window, try to find a match
    for (size_t i = window_start; i < current_pos; i++) {
        // Quick check: if the first byte doesn't match, continue
        if (data[i] != data[current_pos]) {
            continue;
        }
        
        size_t j;
        size_t current_match_length = 0;
        
        // Compare bytes in the window with bytes in the lookahead buffer
        for (j = 0; j < lookahead_end - current_pos; j++) {
            if (data[i + j] != data[current_pos + j]) {
                break;
            }
            current_match_length++;
        }
        
        // If this match is longer than the previous best match
        if (current_match_length >= MIN_MATCH && current_match_length > *match_length) {
            *match_offset = current_pos - i;
            
            // Cap match length to maximum uint8_t value
            if (current_match_length >= UINT8_MAX) {
                *match_length = UINT8_MAX;
                break;
            } else {
                *match_length = (uint8_t)current_match_length;
            }
        }
    }
}

// Compress data using LZ77 algorithm
int compress_lz77_buffer(const uint8_t *input, size_t input_size, 
                        uint8_t *output, size_t *output_size) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    
    // Check for invalid parameters
    if (!input || !output || !output_size || input_size == 0) {
        return 1;
    }
    
    // Process the input data
    while (in_pos < input_size) {
        uint16_t match_offset = 0;
        uint8_t match_length = 0;
        
        // Find the longest match
        find_longest_match(input, input_size, in_pos, &match_offset, &match_length);
        
        // Write the token to the output
        if (out_pos + 4 > *output_size) {
            return 1; // Output buffer too small
        }
        
        // If we found a match, encode it as (offset, length, next_char)
        output[out_pos++] = (match_length >= MIN_MATCH) ? 1 : 0; // Flag byte: 1 for match, 0 for literal
        
        if (match_length >= MIN_MATCH) {
            // Write a match token
            output[out_pos++] = (match_offset >> 8) & 0xFF; // High byte of offset
            output[out_pos++] = match_offset & 0xFF;        // Low byte of offset
            output[out_pos++] = match_length;
            
            // Move input position
            in_pos += match_length;
        } else {
            // Write a literal
            output[out_pos++] = input[in_pos++];
        }
    }
    
    *output_size = out_pos;
    return 0;
}

// Decompress data using LZ77 algorithm
int decompress_lz77_buffer(const uint8_t *input, size_t input_size,
                           uint8_t *output, size_t *output_size) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    
    // Check for invalid parameters
    if (!input || !output || !output_size || input_size == 0) {
        return 1;
    }
    
    // Process the input data
    while (in_pos < input_size && out_pos < *output_size) {
        if (in_pos >= input_size) {
            break;
        }
        
        uint8_t flag = input[in_pos++];
        
        if (flag == 1) {
            // This is a match token
            if (in_pos + 3 > input_size) {
                printf("Error: Unexpected end of compressed data\n");
                return 1; // Malformed input
            }
            
            uint16_t offset = ((uint16_t)input[in_pos] << 8) | input[in_pos + 1];
            uint8_t length = input[in_pos + 2];
            in_pos += 3;
            
            // Sanity check
            if (offset == 0 || offset > out_pos) {
                printf("Error: Invalid match offset %d at position %zu\n", offset, in_pos - 3);
                return 1;
            }
            
            if (out_pos + length > *output_size) {
                printf("Error: Output buffer too small for match\n");
                return 1; // Output buffer too small
            }
            
            // Copy the match from the output buffer
            // Note: The match may overlap with itself, so we can't use memcpy
            for (int i = 0; i < length; i++) {
                output[out_pos] = output[out_pos - offset];
                out_pos++;
            }
        } else {
            // This is a literal
            if (out_pos >= *output_size) {
                printf("Error: Output buffer too small for literal\n");
                return 1; // Output buffer too small
            }
            
            if (in_pos >= input_size) {
                printf("Error: Unexpected end of compressed data\n");
                return 1; // Malformed input
            }
            
            output[out_pos++] = input[in_pos++];
        }
    }
    
    *output_size = out_pos;
    return 0;
}

// Compress a file using LZ77
int compress_lz77(const char *input_file, const char *output_file) {
    FILE *infile, *outfile;
    uint8_t *input_buffer = NULL;
    uint8_t *output_buffer = NULL;
    size_t input_size = 0;
    size_t output_size = 0;
    int result = 1; // Default to error
    size_t file_buffer_size = get_buffer_size(); // Get the configured buffer size
    
    // Apply optimization settings
    set_lz77_optimization(get_optimization_goal());
    
    // Open input file
    infile = fopen(input_file, "rb");
    if (!infile) {
        printf("Error: Cannot open input file %s\n", input_file);
        return 1;
    }
    
    // Get file size
    fseek(infile, 0, SEEK_END);
    input_size = ftell(infile);
    fseek(infile, 0, SEEK_SET);
    
    if (input_size == 0) {
        printf("Error: Input file is empty\n");
        fclose(infile);
        return 1;
    }
    
    // Allocate buffers
    input_buffer = (uint8_t *)malloc(input_size);
    output_buffer = (uint8_t *)malloc(input_size * 2); // Worst case: expansion
    
    if (!input_buffer || !output_buffer) {
        printf("Error: Memory allocation failed\n");
        if (input_buffer) free(input_buffer);
        if (output_buffer) free(output_buffer);
        fclose(infile);
        return 1;
    }
    
    // Read the input file using buffered I/O for better performance
    size_t bytes_read = 0;
    size_t total_read = 0;
    
    while ((bytes_read = fread(input_buffer + total_read, 1, 
           (input_size - total_read < file_buffer_size) ? input_size - total_read : file_buffer_size, 
           infile)) > 0) {
        total_read += bytes_read;
    }
    
    if (total_read != input_size) {
        printf("Error: Failed to read input file, expected %zu bytes, got %zu\n", 
               input_size, total_read);
        free(input_buffer);
        free(output_buffer);
        fclose(infile);
        return 1;
    }
    
    fclose(infile);
    
    // Compress the data
    output_size = input_size * 2; // Start with worst-case size
    if (compress_lz77_buffer(input_buffer, input_size, output_buffer, &output_size) != 0) {
        printf("Error: Compression failed\n");
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    // Open output file
    outfile = fopen(output_file, "wb");
    if (!outfile) {
        printf("Error: Cannot create output file %s\n", output_file);
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    // Write the original file size to the output
    if (fwrite(&input_size, sizeof(size_t), 1, outfile) != 1) {
        printf("Error: Failed to write output file header\n");
        free(input_buffer);
        free(output_buffer);
        fclose(outfile);
        return 1;
    }
    
    // Write the compressed data using buffered I/O
    size_t bytes_written = 0;
    size_t total_written = 0;
    
    while (total_written < output_size) {
        size_t to_write = (output_size - total_written < file_buffer_size) ? 
                          output_size - total_written : file_buffer_size;
        
        bytes_written = fwrite(output_buffer + total_written, 1, to_write, outfile);
        if (bytes_written != to_write) {
            printf("Error: Failed to write compressed data\n");
            free(input_buffer);
            free(output_buffer);
            fclose(outfile);
            return 1;
        }
        
        total_written += bytes_written;
    }
    
    // Success
    result = 0;
    printf("Compressed %zu bytes to %zu bytes (%.2f%%)\n",
           input_size, output_size, (float)output_size * 100 / input_size);
    
    // Clean up
    free(input_buffer);
    free(output_buffer);
    fclose(outfile);
    
    return result;
}

// Decompress a file using LZ77
int decompress_lz77(const char *input_file, const char *output_file) {
    FILE *infile, *outfile;
    uint8_t *input_buffer = NULL;
    uint8_t *output_buffer = NULL;
    size_t input_size = 0;
    size_t output_size = 0;
    size_t original_size = 0;
    int result = 1; // Default to error
    size_t file_buffer_size = get_buffer_size(); // Get the configured buffer size
    
    // Apply optimization settings - needed for proper decompression parameters
    set_lz77_optimization(get_optimization_goal());
    
    // Open input file
    infile = fopen(input_file, "rb");
    if (!infile) {
        printf("Error: Cannot open input file %s\n", input_file);
        return 1;
    }
    
    // Get file size
    fseek(infile, 0, SEEK_END);
    input_size = ftell(infile);
    fseek(infile, 0, SEEK_SET);
    
    if (input_size <= sizeof(size_t)) {
        printf("Error: Input file is too small or corrupted\n");
        fclose(infile);
        return 1;
    }
    
    // Read the original file size
    if (fread(&original_size, sizeof(size_t), 1, infile) != 1) {
        printf("Error: Failed to read file header\n");
        fclose(infile);
        return 1;
    }
    
    // Update input size (exclude header)
    input_size -= sizeof(size_t);
    
    // Allocate buffers
    input_buffer = (uint8_t *)malloc(input_size);
    output_buffer = (uint8_t *)malloc(original_size);
    
    if (!input_buffer || !output_buffer) {
        printf("Error: Memory allocation failed\n");
        if (input_buffer) free(input_buffer);
        if (output_buffer) free(output_buffer);
        fclose(infile);
        return 1;
    }
    
    // Read compressed data using buffered I/O
    size_t bytes_read = 0;
    size_t total_read = 0;
    
    while (total_read < input_size) {
        size_t to_read = (input_size - total_read < file_buffer_size) ? 
                         input_size - total_read : file_buffer_size;
        
        bytes_read = fread(input_buffer + total_read, 1, to_read, infile);
        if (bytes_read != to_read) {
            printf("Error: Failed to read compressed data\n");
            free(input_buffer);
            free(output_buffer);
            fclose(infile);
            return 1;
        }
        
        total_read += bytes_read;
    }
    
    // Close input file
    fclose(infile);
    
    // Decompress the data
    output_size = original_size;
    if (decompress_lz77_buffer(input_buffer, input_size, output_buffer, &output_size) != 0) {
        printf("Error: Decompression failed\n");
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    // Check if the decompressed size matches the expected size
    if (output_size != original_size) {
        printf("Warning: Decompressed size (%zu) does not match expected size (%zu)\n",
               output_size, original_size);
    }
    
    // Open output file
    outfile = fopen(output_file, "wb");
    if (!outfile) {
        printf("Error: Cannot create output file %s\n", output_file);
        free(input_buffer);
        free(output_buffer);
        return 1;
    }
    
    // Write the decompressed data using buffered I/O
    size_t bytes_written = 0;
    size_t total_written = 0;
    
    while (total_written < output_size) {
        size_t to_write = (output_size - total_written < file_buffer_size) ? 
                          output_size - total_written : file_buffer_size;
        
        bytes_written = fwrite(output_buffer + total_written, 1, to_write, outfile);
        if (bytes_written != to_write) {
            printf("Error: Failed to write decompressed data\n");
            free(input_buffer);
            free(output_buffer);
            fclose(outfile);
            return 1;
        }
        
        total_written += bytes_written;
    }
    
    // Success
    result = 0;
    printf("Decompressed %zu bytes to %zu bytes\n", input_size, output_size);
    
    // Clean up
    free(input_buffer);
    free(output_buffer);
    fclose(outfile);
    
    return result;
} 