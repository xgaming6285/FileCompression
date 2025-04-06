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
    
    printf("DEBUG: RLE compressing file of size %ld bytes\n", file_size);
    
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        fclose(in);
        return 1;
    }
    
    // Write original file size
    if (fwrite(&file_size, sizeof(long), 1, out) != 1) {
        printf("Error writing file size to output\n");
        fclose(in);
        fclose(out);
        return 1;
    }
    
    // Perform RLE compression
    if (file_size > 0) {
        int byte = fgetc(in);
        if (byte == EOF) {
            printf("Error reading first byte\n");
            fclose(in);
            fclose(out);
            return 1;
        }
        
        uint8_t current_byte = (uint8_t)byte;
        uint8_t count = 1;
        
        for (long i = 1; i < file_size; i++) {
            byte = fgetc(in);
            if (byte == EOF) {
                printf("Error reading byte at position %ld\n", i);
                fclose(in);
                fclose(out);
                return 1;
            }
            
            uint8_t next_byte = (uint8_t)byte;
            
            // If same byte and not at max run length
            if (next_byte == current_byte && count < MAX_RUN) {
                count++;
            } else {
                // Write the run
                if (fputc(count, out) == EOF || fputc(current_byte, out) == EOF) {
                    printf("Error writing to output file\n");
                    fclose(in);
                    fclose(out);
                    return 1;
                }
                
                // Start new run
                current_byte = next_byte;
                count = 1;
            }
        }
        
        // Write the last run
        if (fputc(count, out) == EOF || fputc(current_byte, out) == EOF) {
            printf("Error writing final run to output file\n");
            fclose(in);
            fclose(out);
            return 1;
        }
    }
    
    printf("DEBUG: RLE compression completed successfully\n");
    
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
    if (fread(&file_size, sizeof(long), 1, in) != 1) {
        printf("Error reading original file size\n");
        fclose(in);
        return 1;
    }
    
    printf("DEBUG: RLE decompressing to size %ld bytes\n", file_size);
    
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        fclose(in);
        return 1;
    }
    
    // Perform RLE decompression
    long bytes_written = 0;
    
    while (bytes_written < file_size) {
        int count_val = fgetc(in);
        int byte_val = fgetc(in);
        
        if (count_val == EOF || byte_val == EOF) {
            printf("Error: Unexpected end of file at position %ld\n", bytes_written);
            fclose(in);
            fclose(out);
            return 1;
        }
        
        uint8_t count = (uint8_t)count_val;
        uint8_t byte = (uint8_t)byte_val;
        
        // Write 'count' copies of 'byte'
        for (int i = 0; i < count && bytes_written < file_size; i++) {
            if (fputc(byte, out) == EOF) {
                printf("Error writing to output file at position %ld\n", bytes_written);
                fclose(in);
                fclose(out);
                return 1;
            }
            bytes_written++;
        }
    }
    
    printf("DEBUG: RLE decompression completed successfully\n");
    
    fclose(in);
    fclose(out);
    return 0;
} 