/**
 * Encryption Utility
 * Implementation file for encryption and decryption
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encryption.h"
#include "compression.h"
#include "lz77.h" // We'll use LZ77 as the default compression algorithm

// Simple encryption using XOR with key cycling
// Note: This is a simplified implementation for demonstration purposes
// In production, use a proper cryptographic library
int encrypt_buffer(uint8_t *buffer, size_t buffer_size, const char *key, size_t key_length) {
    if (!buffer || !key || key_length == 0) {
        return 1; // Error
    }

    // XOR each byte with the corresponding byte from the key
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] ^= key[i % key_length];
    }

    return 0; // Success
}

// Decryption is the same as encryption for XOR (applying the same operation twice cancels out)
int decrypt_buffer(uint8_t *buffer, size_t buffer_size, const char *key, size_t key_length) {
    return encrypt_buffer(buffer, buffer_size, key, key_length); // XOR is its own inverse
}

// Encrypt a file
int encrypt_file(const char *input_file, const char *output_file, const char *key) {
    FILE *in_file = fopen(input_file, "rb");
    if (!in_file) {
        printf("Error: Could not open input file %s\n", input_file);
        return 1;
    }

    FILE *out_file = fopen(output_file, "wb");
    if (!out_file) {
        printf("Error: Could not create output file %s\n", output_file);
        fclose(in_file);
        return 1;
    }

    // Get the key length
    size_t key_length = strlen(key);
    
    // Buffer for reading/writing
    uint8_t buffer[4096];
    size_t bytes_read;

    // Write a simple header for encrypted files (you might want to add more info here)
    const char *header = "ENCRYPTED";
    fwrite(header, 1, strlen(header), out_file);

    // Process the file in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
        // Encrypt the buffer
        encrypt_buffer(buffer, bytes_read, key, key_length);
        
        // Write the encrypted data
        fwrite(buffer, 1, bytes_read, out_file);
    }

    fclose(in_file);
    fclose(out_file);
    return 0;
}

// Decrypt a file
int decrypt_file(const char *input_file, const char *output_file, const char *key) {
    FILE *in_file = fopen(input_file, "rb");
    if (!in_file) {
        printf("Error: Could not open input file %s\n", input_file);
        return 1;
    }

    FILE *out_file = fopen(output_file, "wb");
    if (!out_file) {
        printf("Error: Could not create output file %s\n", output_file);
        fclose(in_file);
        return 1;
    }

    // Get the key length
    size_t key_length = strlen(key);
    
    // Buffer for reading/writing
    uint8_t buffer[4096];
    size_t bytes_read;

    // Read and verify the header
    char header[10] = {0};
    if (fread(header, 1, 9, in_file) != 9 || strcmp(header, "ENCRYPTED") != 0) {
        printf("Error: Input file is not an encrypted file or is corrupted\n");
        fclose(in_file);
        fclose(out_file);
        return 1;
    }

    // Process the file in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
        // Decrypt the buffer
        decrypt_buffer(buffer, bytes_read, key, key_length);
        
        // Write the decrypted data
        fwrite(buffer, 1, bytes_read, out_file);
    }

    fclose(in_file);
    fclose(out_file);
    return 0;
}

// Compress and encrypt in one step
int compress_and_encrypt(const char *input_file, const char *output_file, const char *key) {
    // Create a temporary file for compressed data
    char temp_file[1024];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", input_file);
    
    // Compress the file using LZ77
    if (compress_lz77(input_file, temp_file) != 0) {
        printf("Error compressing file\n");
        return 1;
    }
    
    // Encrypt the compressed file
    int result = encrypt_file(temp_file, output_file, key);
    
    // Clean up the temporary file
    remove(temp_file);
    
    return result;
}

// Decrypt and decompress in one step
int decrypt_and_decompress(const char *input_file, const char *output_file, const char *key) {
    // Create a temporary file for decrypted data
    char temp_file[1024];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", input_file);
    
    // Decrypt the file
    if (decrypt_file(input_file, temp_file, key) != 0) {
        printf("Error decrypting file\n");
        return 1;
    }
    
    // Decompress the decrypted file
    int result = decompress_lz77(temp_file, output_file);
    
    // Clean up the temporary file
    remove(temp_file);
    
    return result;
} 