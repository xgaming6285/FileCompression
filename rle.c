/**
 * Run-Length Encoding Implementation
 * Implementation file with compression and decompression functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "rle.h"

// Max run length (we use 255 since it needs to fit in a byte)
#define MAX_RUN 255

// Function to compress a file using RLE
int compress_rle(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Get file size
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        fclose(in);
        return 1;
    }
    
    // Write original file size
    fwrite(&file_size, sizeof(long), 1, out);
    
    // Perform RLE compression
    if (file_size > 0) {
        uint8_t current_byte = fgetc(in);
        uint8_t count = 1;
        
        for (long i = 1; i < file_size; i++) {
            uint8_t next_byte = fgetc(in);
            
            // If same byte and not at max run length
            if (next_byte == current_byte && count < MAX_RUN) {
                count++;
            } else {
                // Write the run
                fputc(count, out);
                fputc(current_byte, out);
                
                // Start new run
                current_byte = next_byte;
                count = 1;
            }
        }
        
        // Write the last run
        fputc(count, out);
        fputc(current_byte, out);
    }
    
    fclose(in);
    fclose(out);
    return 0;
}

// Function to decompress a file using RLE
int decompress_rle(const char *input_file, const char *output_file) {
    FILE *in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Read original file size
    long file_size;
    fread(&file_size, sizeof(long), 1, in);
    
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        fclose(in);
        return 1;
    }
    
    // Perform RLE decompression
    long bytes_written = 0;
    
    while (bytes_written < file_size) {
        uint8_t count = fgetc(in);
        uint8_t byte = fgetc(in);
        
        // Write 'count' copies of 'byte'
        for (int i = 0; i < count && bytes_written < file_size; i++) {
            fputc(byte, out);
            bytes_written++;
        }
    }
    
    fclose(in);
    fclose(out);
    return 0;
} 