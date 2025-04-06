/**
 * Split Archive Functionality
 * Support for splitting large compressed archives across multiple files
 */
#ifndef SPLIT_ARCHIVE_H
#define SPLIT_ARCHIVE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "large_file_utils.h"

// Default maximum size for each part of the split archive (100MB)
#define DEFAULT_SPLIT_SIZE (100 * 1024 * 1024)

// Minimum size for split parts (1MB)
#define MIN_SPLIT_SIZE (1 * 1024 * 1024)

// Maximum number of split files allowed
#define MAX_SPLIT_FILES 9999

// Compress a file to a split archive
int compress_to_split_archive(const char* input_file, const char* output_base, 
                              int algorithm_index, uint64_t max_part_size,
                              ChecksumType checksum_type);

// Decompress a split archive to a file
int decompress_from_split_archive(const char* input_base, const char* output_file,
                                 int algorithm_index, ChecksumType checksum_type);

#endif /* SPLIT_ARCHIVE_H */ 