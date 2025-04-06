/**
 * Deduplication Utility
 * Header file for identifying and eliminating redundant data
 */
#ifndef DEDUPLICATION_H
#define DEDUPLICATION_H

#include <stdint.h>
#include <stddef.h>
#include "large_file_utils.h"

// Minimum chunk size for deduplication (4KB)
#define MIN_DEDUP_CHUNK_SIZE 4096

// Maximum chunk size for deduplication (1MB)
#define MAX_DEDUP_CHUNK_SIZE 1048576

// Default chunk size for deduplication (64KB)
#define DEFAULT_DEDUP_CHUNK_SIZE 65536

// Minimum chunk size that will be considered for deduplication
#define MIN_DEDUP_SIZE 512

// Hash algorithm for deduplication
typedef enum {
    DEDUP_HASH_SHA1,
    DEDUP_HASH_MD5,
    DEDUP_HASH_CRC32,
    DEDUP_HASH_XXH64
} DedupHashAlgorithm;

// Deduplication mode
typedef enum {
    DEDUP_MODE_FIXED,      // Fixed-size chunking
    DEDUP_MODE_VARIABLE,   // Content-defined chunking
    DEDUP_MODE_SMART       // Smart chunking based on file type
} DedupMode;

// Statistics for deduplication
typedef struct {
    uint64_t total_bytes;             // Total bytes processed
    uint64_t bytes_after_dedup;       // Bytes after deduplication
    uint64_t total_chunks;            // Total number of chunks
    uint64_t duplicate_chunks;        // Number of duplicate chunks
    uint64_t duplicate_bytes_saved;   // Number of bytes saved via deduplication
    double deduplication_ratio;       // Deduplication ratio (1 - bytes_after_dedup/total_bytes)
} DedupStats;

// Function prototypes

/**
 * Initialize deduplication module
 * @param chunk_size Size of chunks for fixed-size chunking
 * @param hash_algorithm Hash algorithm to use
 * @param dedup_mode Chunking mode (fixed, variable, or smart)
 * @return 0 if successful, non-zero otherwise
 */
int init_deduplication(size_t chunk_size, DedupHashAlgorithm hash_algorithm, DedupMode dedup_mode);

/**
 * Deduplicate a file or archive
 * @param input_file Input file or archive
 * @param output_file Output deduplicated file or archive
 * @param algorithm_index Compression algorithm to use
 * @param checksum_type Checksum type for integrity verification
 * @return 0 if successful, non-zero otherwise
 */
int deduplicate_file(const char* input_file, const char* output_file, int algorithm_index, ChecksumType checksum_type);

/**
 * Get deduplication statistics
 * @return DedupStats structure with deduplication statistics
 */
DedupStats get_dedup_stats();

/**
 * Print deduplication statistics
 */
void print_dedup_stats();

/**
 * Clean up deduplication resources
 */
void cleanup_deduplication();

#endif // DEDUPLICATION_H 