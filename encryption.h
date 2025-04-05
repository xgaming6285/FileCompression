/**
 * Encryption Utility
 * Header file for encryption and decryption
 */
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>

// AES-like simple encryption implementation (XOR-based for simplicity)
// In a production setting, you'd use a proper crypto library

// Encrypts a buffer in place
int encrypt_buffer(uint8_t *buffer, size_t buffer_size, const char *key, size_t key_length);

// Decrypts a buffer in place
int decrypt_buffer(uint8_t *buffer, size_t buffer_size, const char *key, size_t key_length);

// Encrypts a file
int encrypt_file(const char *input_file, const char *output_file, const char *key);

// Decrypts a file
int decrypt_file(const char *input_file, const char *output_file, const char *key);

// Compress and encrypt in one step
int compress_and_encrypt(const char *input_file, const char *output_file, const char *key);

// Decrypt and decompress in one step
int decrypt_and_decompress(const char *input_file, const char *output_file, const char *key);

#endif // ENCRYPTION_H 