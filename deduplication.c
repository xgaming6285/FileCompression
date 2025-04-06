/**
 * Deduplication Utility
 * Implementation file for identifying and eliminating redundant data
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "deduplication.h"
#include "large_file_utils.h"
#include "compression.h"
#include "filecompressor.h"

// Chunk hash entry
typedef struct ChunkHash {
    unsigned char hash[SHA_DIGEST_LENGTH];   // SHA1 hash (20 bytes)
    uint64_t offset;                         // Original offset in file
    uint32_t size;                           // Chunk size
    uint32_t ref_count;                      // Reference count
    struct ChunkHash* next;                  // For hash collision handling
} ChunkHash;

// Hash table for deduplication
#define HASH_TABLE_SIZE 65536
static ChunkHash* hash_table[HASH_TABLE_SIZE] = {NULL};

// Deduplication settings
static size_t current_chunk_size = DEFAULT_DEDUP_CHUNK_SIZE;
static DedupHashAlgorithm current_hash_algorithm = DEDUP_HASH_SHA1;
static DedupMode current_dedup_mode = DEDUP_MODE_FIXED;

// Deduplication statistics
static DedupStats dedup_stats = {0};

// Content-defined chunking parameters (CDC)
#define CDC_WINDOW_SIZE 48
#define CDC_PRIME 31
#define CDC_MASK 0x0000FFFF  // Mask for 16 bits

// Special value for MIN_DEDUP_SIZE to test with smaller files
#define MIN_DEDUP_SIZE 64

// Helper function to compute SHA1 hash
static void compute_sha1_hash(const uint8_t* data, size_t size, unsigned char* hash) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, size);
    SHA1_Final(hash, &ctx);
}

// Helper function to compute MD5 hash
static void compute_md5_hash(const uint8_t* data, size_t size, unsigned char* hash) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data, size);
    MD5_Final(hash, &ctx);
}

// Helper function to compute CRC32 hash
static uint32_t compute_crc32(const uint8_t* data, size_t size) {
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return ~crc;
}

// Helper function to compute hash based on selected algorithm
static void compute_hash(const uint8_t* data, size_t size, unsigned char* hash) {
    switch (current_hash_algorithm) {
        case DEDUP_HASH_SHA1:
            compute_sha1_hash(data, size, hash);
            break;
        case DEDUP_HASH_MD5:
            compute_md5_hash(data, size, hash);
            break;
        case DEDUP_HASH_CRC32: {
            uint32_t crc = compute_crc32(data, size);
            memcpy(hash, &crc, sizeof(crc));
            memset(hash + sizeof(crc), 0, SHA_DIGEST_LENGTH - sizeof(crc));
            break;
        }
        case DEDUP_HASH_XXH64:
            // XXH64 not implemented in this example, use SHA1 instead
            compute_sha1_hash(data, size, hash);
            break;
    }
}

// Helper function to get hash table index from a hash
static uint16_t get_hash_index(const unsigned char* hash) {
    // Use first 2 bytes of the hash as an index
    return (hash[0] << 8) | hash[1];
}

// Helper function to compare two hashes
static int compare_hashes(const unsigned char* hash1, const unsigned char* hash2) {
    return memcmp(hash1, hash2, SHA_DIGEST_LENGTH);
}

// Initialize deduplication module
int init_deduplication(size_t chunk_size, DedupHashAlgorithm hash_algorithm, DedupMode dedup_mode) {
    // Validate chunk size
    if (chunk_size < MIN_DEDUP_CHUNK_SIZE) {
        current_chunk_size = MIN_DEDUP_CHUNK_SIZE;
        printf("Warning: Chunk size too small, using minimum size: %zu bytes\n", current_chunk_size);
    } else if (chunk_size > MAX_DEDUP_CHUNK_SIZE) {
        current_chunk_size = MAX_DEDUP_CHUNK_SIZE;
        printf("Warning: Chunk size too large, using maximum size: %zu bytes\n", current_chunk_size);
    } else {
        current_chunk_size = chunk_size;
    }
    
    // Set hash algorithm
    current_hash_algorithm = hash_algorithm;
    
    // Set deduplication mode
    current_dedup_mode = dedup_mode;
    
    // Clear hash table
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (hash_table[i] != NULL) {
            ChunkHash* current = hash_table[i];
            while (current != NULL) {
                ChunkHash* next = current->next;
                free(current);
                current = next;
            }
            hash_table[i] = NULL;
        }
    }
    
    // Clear statistics
    memset(&dedup_stats, 0, sizeof(dedup_stats));
    
    printf("Deduplication initialized with:\n");
    printf("  - Chunk size: %zu bytes\n", current_chunk_size);
    printf("  - Hash algorithm: %d\n", current_hash_algorithm);
    printf("  - Deduplication mode: %d\n", current_dedup_mode);
    
    return 0;
}

// Helper function to roll hash for CDC
static uint32_t roll_hash(uint32_t current_hash, uint8_t out_byte, uint8_t in_byte, uint32_t power) {
    return CDC_PRIME * (current_hash - out_byte * power) + in_byte;
}

// Find chunk boundaries using content-defined chunking
static size_t find_chunk_boundary(const uint8_t* data, size_t size) {
    if (size <= MIN_DEDUP_SIZE) {
        return size;
    }
    
    // Use Rabin-Karp rolling hash for CDC
    uint32_t hash = 0;
    uint32_t power = 1;
    
    // Compute initial hash and power
    size_t window_size = CDC_WINDOW_SIZE < size ? CDC_WINDOW_SIZE : size;
    
    for (size_t i = 0; i < window_size; i++) {
        hash = hash * CDC_PRIME + data[i];
        if (i < window_size - 1) {
            power = (power * CDC_PRIME);
        }
    }
    
    // Look for chunk boundaries using rolling hash
    for (size_t i = window_size; i < size; i++) {
        hash = roll_hash(hash, data[i - window_size], data[i], power);
        
        // Check if we found a boundary
        if ((hash & CDC_MASK) == 0) {
            return i + 1;
        }
    }
    
    // No boundary found, return the whole chunk
    return size;
}

// Add a chunk to the hash table
static int add_chunk(const uint8_t* data, size_t size, uint64_t offset) {
    // For small test files, remove the minimum size limit
    // if (size < MIN_DEDUP_SIZE) {
    //     return 0; // Too small to deduplicate
    // }
    
    // Compute hash of the chunk
    unsigned char hash[SHA_DIGEST_LENGTH];
    compute_hash(data, size, hash);
    
    // Get index in hash table
    uint16_t index = get_hash_index(hash);
    
    // Check if the chunk already exists
    ChunkHash* current = hash_table[index];
    while (current != NULL) {
        if (compare_hashes(current->hash, hash) == 0 && current->size == size) {
            // Found a duplicate
            current->ref_count++;
            dedup_stats.duplicate_chunks++;
            dedup_stats.duplicate_bytes_saved += size;
            return 1; // Duplicate found
        }
        current = current->next;
    }
    
    // Add new chunk to hash table
    ChunkHash* new_chunk = (ChunkHash*)malloc(sizeof(ChunkHash));
    if (new_chunk == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for chunk hash\n");
        return -1;
    }
    
    memcpy(new_chunk->hash, hash, SHA_DIGEST_LENGTH);
    new_chunk->offset = offset;
    new_chunk->size = size;
    new_chunk->ref_count = 1;
    new_chunk->next = hash_table[index];
    hash_table[index] = new_chunk;
    
    return 0; // New chunk added
}

// Deduplicate a file
int deduplicate_file(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type) {
    FILE* in_file = fopen(input_file, "rb");
    if (in_file == NULL) {
        fprintf(stderr, "Error: Failed to open input file for deduplication: %s\n", input_file);
        return -1;
    }
    
    FILE* out_file = fopen(output_file, "wb");
    if (out_file == NULL) {
        fprintf(stderr, "Error: Failed to open output file for deduplication: %s\n", output_file);
        fclose(in_file);
        return -1;
    }
    
    // Get file size
    fseek(in_file, 0, SEEK_END);
    uint64_t file_size = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);
    
    dedup_stats.total_bytes = file_size;
    
    // Write deduplication header
    fprintf(out_file, "DEDUP");
    
    // Write original file size
    fwrite(&file_size, sizeof(file_size), 1, out_file);
    
    // Buffer for reading chunks
    uint8_t* buffer = (uint8_t*)malloc(MAX_DEDUP_CHUNK_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for deduplication buffer\n");
        fclose(in_file);
        fclose(out_file);
        return -1;
    }
    
    // Temporary index file for later processing
    FILE* index_file = tmpfile();
    if (index_file == NULL) {
        fprintf(stderr, "Error: Failed to create temporary index file\n");
        free(buffer);
        fclose(in_file);
        fclose(out_file);
        return -1;
    }
    
    // First pass: Identify chunks and build hash table
    uint64_t offset = 0;
    
    while (offset < file_size) {
        size_t bytes_to_read = current_chunk_size;
        if (offset + bytes_to_read > file_size) {
            bytes_to_read = file_size - offset;
        }
        
        size_t bytes_read = fread(buffer, 1, bytes_to_read, in_file);
        if (bytes_read <= 0) {
            break;
        }
        
        // Find chunk boundary based on deduplication mode
        size_t chunk_size;
        
        if (current_dedup_mode == DEDUP_MODE_FIXED) {
            chunk_size = bytes_read;
        } else {
            // Variable-size chunking or smart chunking
            chunk_size = find_chunk_boundary(buffer, bytes_read);
        }
        
        // Add chunk to hash table and get result
        int result = add_chunk(buffer, chunk_size, offset);
        
        // Write chunk information to index file
        fwrite(&offset, sizeof(offset), 1, index_file);
        fwrite(&chunk_size, sizeof(chunk_size), 1, index_file);
        fwrite(&result, sizeof(result), 1, index_file);
        
        dedup_stats.total_chunks++;
        
        offset += chunk_size;
        
        // Adjust file position if we read more than the chunk size
        if (chunk_size < bytes_read) {
            fseek(in_file, offset, SEEK_SET);
        }
    }
    
    // Reset index file for reading
    fseek(index_file, 0, SEEK_SET);
    
    // Second pass: Write unique chunks and references
    fseek(in_file, 0, SEEK_SET);
    
    // Write number of chunks
    uint64_t total_chunks = dedup_stats.total_chunks;
    fwrite(&total_chunks, sizeof(total_chunks), 1, out_file);
    
    uint64_t bytes_after_dedup = 0;
    
    for (uint64_t i = 0; i < total_chunks; i++) {
        uint64_t chunk_offset;
        size_t chunk_size;
        int is_duplicate;
        
        // Read chunk info from index file
        fread(&chunk_offset, sizeof(chunk_offset), 1, index_file);
        fread(&chunk_size, sizeof(chunk_size), 1, index_file);
        fread(&is_duplicate, sizeof(is_duplicate), 1, index_file);
        
        // Write chunk metadata
        fwrite(&chunk_size, sizeof(chunk_size), 1, out_file);
        
        if (is_duplicate) {
            // For duplicate chunks, write a flag and original offset
            uint8_t is_ref = 1;
            fwrite(&is_ref, sizeof(is_ref), 1, out_file);
            
            // Find the original chunk in the hash table
            fseek(in_file, chunk_offset, SEEK_SET);
            fread(buffer, 1, chunk_size, in_file);
            
            unsigned char hash[SHA_DIGEST_LENGTH];
            compute_hash(buffer, chunk_size, hash);
            
            uint16_t index = get_hash_index(hash);
            ChunkHash* current = hash_table[index];
            
            uint64_t original_offset = 0;
            while (current != NULL) {
                if (compare_hashes(current->hash, hash) == 0 && current->size == chunk_size) {
                    original_offset = current->offset;
                    if (original_offset != chunk_offset) {
                        break;
                    }
                }
                current = current->next;
            }
            
            fwrite(&original_offset, sizeof(original_offset), 1, out_file);
            bytes_after_dedup += sizeof(is_ref) + sizeof(original_offset);
        } else {
            // For unique chunks, write the actual data
            uint8_t is_ref = 0;
            fwrite(&is_ref, sizeof(is_ref), 1, out_file);
            
            fseek(in_file, chunk_offset, SEEK_SET);
            fread(buffer, 1, chunk_size, in_file);
            fwrite(buffer, 1, chunk_size, out_file);
            
            bytes_after_dedup += sizeof(is_ref) + chunk_size;
        }
    }
    
    dedup_stats.bytes_after_dedup = bytes_after_dedup;
    
    if (dedup_stats.total_bytes > 0) {
        dedup_stats.deduplication_ratio = 1.0 - ((double)dedup_stats.bytes_after_dedup / (double)dedup_stats.total_bytes);
    } else {
        dedup_stats.deduplication_ratio = 0.0;
    }
    
    // Clean up
    free(buffer);
    fclose(in_file);
    fclose(out_file);
    fclose(index_file);
    
    // Apply compression if requested
    if (algorithm_index >= 0) {
        char temp_file[1024];
        snprintf(temp_file, sizeof(temp_file), "%s.temp", output_file);
        
        // Rename the deduplicated file to a temporary name
        rename(output_file, temp_file);
        
        // Compress the deduplicated file
        int result = compress_file_with_algorithm(temp_file, output_file, algorithm_index, checksum_type);
        
        // Remove the temporary file
        remove(temp_file);
        
        if (result != 0) {
            fprintf(stderr, "Error: Compression failed after deduplication\n");
            return -1;
        }
    }
    
    return 0;
}

// Get deduplication statistics
DedupStats get_dedup_stats() {
    return dedup_stats;
}

// Print deduplication statistics
void print_dedup_stats() {
    printf("\nDeduplication Statistics:\n");
    printf("------------------------\n");
    printf("Total bytes processed:      %llu\n", (unsigned long long)dedup_stats.total_bytes);
    printf("Bytes after deduplication:  %llu\n", (unsigned long long)dedup_stats.bytes_after_dedup);
    printf("Total chunks:               %llu\n", (unsigned long long)dedup_stats.total_chunks);
    printf("Duplicate chunks:           %llu\n", (unsigned long long)dedup_stats.duplicate_chunks);
    printf("Bytes saved by deduplication: %llu\n", (unsigned long long)dedup_stats.duplicate_bytes_saved);
    printf("Deduplication ratio:        %.2f%%\n", dedup_stats.deduplication_ratio * 100.0);
}

// Clean up deduplication resources
void cleanup_deduplication() {
    // Free hash table entries
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ChunkHash* current = hash_table[i];
        while (current != NULL) {
            ChunkHash* next = current->next;
            free(current);
            current = next;
        }
        hash_table[i] = NULL;
    }
} 