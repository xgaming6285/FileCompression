/**
 * Split Archive Functionality Implementation
 * Support for splitting large compressed archives across multiple files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "split_archive.h"
#include "compression.h"
#include "large_file_utils.h"

// Magic number for split archive parts
#define SPLIT_ARCHIVE_MAGIC "SPLT"

// Structure for archive part header
typedef struct {
    char magic[4];         // Magic number "SPLT"
    uint32_t part_number;  // Part number (1-based)
    uint32_t total_parts;  // Total number of parts
    uint64_t part_size;    // Size of data in this part
    uint64_t total_size;   // Total archive size
    ChecksumType checksum_type; // Type of checksum used
    uint8_t checksum_data[32];  // Checksum data (size depends on checksum type)
} ArchivePartHeader;

// Get filename for a specific part
static char* get_part_filename(const char* base_filename, uint32_t part_number) {
    // Calculate required buffer size: base + ".part" + part number (max 4 digits) + NULL
    size_t filename_len = strlen(base_filename) + 10;
    char* part_filename = (char*)malloc(filename_len);
    
    if (!part_filename) {
        fprintf(stderr, "Error: Memory allocation failed for part filename\n");
        return NULL;
    }
    
    snprintf(part_filename, filename_len, "%s.part%04u", base_filename, part_number);
    return part_filename;
}

/**
 * Compress a file to a split archive
 * @param input_file      Path to the input file
 * @param output_base     Base path for the output files (without extension)
 * @param algorithm_index Index of the compression algorithm to use
 * @param max_part_size   Maximum size for each part of the split archive
 * @param checksum_type   Type of checksum to use for integrity checking
 * @return 0 on success, non-zero on failure
 */
int compress_to_split_archive(const char* input_file, const char* output_base, 
                             int algorithm_index, uint64_t max_part_size,
                             ChecksumType checksum_type) {
    // Validate input parameters
    if (!input_file || !output_base) {
        fprintf(stderr, "Error: Invalid input or output path\n");
        return -1;
    }
    
    // Validate max_part_size
    if (max_part_size < MIN_SPLIT_SIZE) {
        fprintf(stderr, "Warning: Split size %llu is below minimum, using %u bytes instead\n", 
                (unsigned long long)max_part_size, MIN_SPLIT_SIZE);
        max_part_size = MIN_SPLIT_SIZE;
    }
    
    // Open input file
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Error: Could not open input file %s\n", input_file);
        return -1;
    }
    
    // Get file size
    fseek(input, 0, SEEK_END);
    int64_t file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fprintf(stderr, "Error: Invalid file size for %s\n", input_file);
        fclose(input);
        return -1;
    }
    
    // Calculate number of parts needed
    uint32_t total_parts = (uint32_t)((file_size + max_part_size - 1) / max_part_size);
    
    if (total_parts > MAX_SPLIT_FILES) {
        fprintf(stderr, "Error: Required %u parts exceeds maximum of %u\n", 
                total_parts, MAX_SPLIT_FILES);
        fclose(input);
        return -1;
    }
    
    printf("Splitting archive into %u parts of max %llu bytes each\n", 
           total_parts, (unsigned long long)max_part_size);
    
    // Allocate buffer for compressed data
    size_t buffer_size = DEFAULT_CHUNK_SIZE;
    uint8_t* input_buffer = (uint8_t*)malloc(buffer_size);
    uint8_t* compressed_buffer = (uint8_t*)malloc(buffer_size * 2); // Worst case expansion
    
    if (!input_buffer || !compressed_buffer) {
        fprintf(stderr, "Error: Memory allocation failed for buffers\n");
        free(input_buffer);
        free(compressed_buffer);
        fclose(input);
        return -1;
    }
    
    // Process each part
    uint32_t current_part = 1;
    uint64_t total_written = 0;
    uint64_t remaining_size = file_size;
    
    while (current_part <= total_parts) {
        // Open output file for this part
        char* part_filename = get_part_filename(output_base, current_part);
        if (!part_filename) {
            free(input_buffer);
            free(compressed_buffer);
            fclose(input);
            return -1;
        }
        
        FILE* output = fopen(part_filename, "wb");
        if (!output) {
            fprintf(stderr, "Error: Could not open output file %s\n", part_filename);
            free(part_filename);
            free(input_buffer);
            free(compressed_buffer);
            fclose(input);
            return -1;
        }
        
        printf("Creating part %u/%u: %s\n", current_part, total_parts, part_filename);
        free(part_filename);
        
        // Calculate part size
        uint64_t this_part_size = (remaining_size > max_part_size) ? 
                                 max_part_size : remaining_size;
        
        // Write header
        ArchivePartHeader header;
        memset(&header, 0, sizeof(header));
        memcpy(header.magic, SPLIT_ARCHIVE_MAGIC, 4);
        header.part_number = current_part;
        header.total_parts = total_parts;
        header.part_size = this_part_size;
        header.total_size = file_size;
        header.checksum_type = checksum_type;
        
        if (fwrite(&header, sizeof(header), 1, output) != 1) {
            fprintf(stderr, "Error: Failed to write header for part %u\n", current_part);
            fclose(output);
            free(input_buffer);
            free(compressed_buffer);
            fclose(input);
            return -1;
        }
        
        // Process the part in chunks
        uint64_t part_remaining = this_part_size;
        
        while (part_remaining > 0) {
            // Read a chunk from input
            size_t read_size = (part_remaining > buffer_size) ? 
                             buffer_size : (size_t)part_remaining;
            
            size_t bytes_read = fread(input_buffer, 1, read_size, input);
            if (bytes_read != read_size) {
                fprintf(stderr, "Error: Failed to read from input file\n");
                fclose(output);
                free(input_buffer);
                free(compressed_buffer);
                fclose(input);
                return -1;
            }
            
            // Compress the chunk using the specified algorithm
            size_t output_size = buffer_size * 2;
            int compressed_size = compress_buffer(algorithm_index, input_buffer, bytes_read,
                                                 compressed_buffer, &output_size);
            
            if (compressed_size < 0) {
                fprintf(stderr, "Error: Compression failed\n");
                fclose(output);
                free(input_buffer);
                free(compressed_buffer);
                fclose(input);
                return -1;
            }
            
            // Write compressed data to output
            if (fwrite(compressed_buffer, 1, output_size, output) != output_size) {
                fprintf(stderr, "Error: Failed to write to output file\n");
                fclose(output);
                free(input_buffer);
                free(compressed_buffer);
                fclose(input);
                return -1;
            }
            
            // Update counters
            part_remaining -= bytes_read;
            total_written += bytes_read;
        }
        
        fclose(output);
        current_part++;
        remaining_size -= this_part_size;
        
        // Print progress
        printf("Progress: %llu / %llu bytes (%.1f%%)\n", 
               (unsigned long long)total_written,
               (unsigned long long)file_size,
               (float)total_written * 100.0f / file_size);
    }
    
    // Clean up
    free(input_buffer);
    free(compressed_buffer);
    fclose(input);
    
    printf("Split archive creation completed: %u parts created\n", total_parts);
    return 0;
}

/**
 * Decompress a split archive to a file
 * @param input_base      Base path for the input files (without extension)
 * @param output_file     Path to the output file
 * @param algorithm_index Index of the compression algorithm to use
 * @param checksum_type   Type of checksum to use for integrity checking
 * @return 0 on success, non-zero on failure
 */
int decompress_from_split_archive(const char* input_base, const char* output_file,
                                 int algorithm_index, ChecksumType checksum_type) {
    // Validate input parameters
    (void)checksum_type; // Mark as unused for now
    
    if (!input_base || !output_file) {
        fprintf(stderr, "Error: Invalid input or output path\n");
        return -1;
    }
    
    // Open the first part to read the header
    char* first_part_filename = get_part_filename(input_base, 1);
    if (!first_part_filename) {
        return -1;
    }
    
    FILE* first_part = fopen(first_part_filename, "rb");
    if (!first_part) {
        fprintf(stderr, "Error: Could not open first part file %s\n", first_part_filename);
        free(first_part_filename);
        return -1;
    }
    
    // Read and validate header
    ArchivePartHeader header;
    if (fread(&header, sizeof(header), 1, first_part) != 1) {
        fprintf(stderr, "Error: Failed to read header from %s\n", first_part_filename);
        fclose(first_part);
        free(first_part_filename);
        return -1;
    }
    
    // Verify magic number
    if (memcmp(header.magic, SPLIT_ARCHIVE_MAGIC, 4) != 0) {
        fprintf(stderr, "Error: Invalid split archive format in %s\n", first_part_filename);
        fclose(first_part);
        free(first_part_filename);
        return -1;
    }
    
    fclose(first_part);
    free(first_part_filename);
    
    // Create output file
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Error: Could not create output file %s\n", output_file);
        return -1;
    }
    
    uint32_t total_parts = header.total_parts;
    
    printf("Decompressing split archive with %u parts\n", total_parts);
    
    // Allocate buffers
    size_t buffer_size = DEFAULT_CHUNK_SIZE;
    uint8_t* compressed_buffer = (uint8_t*)malloc(buffer_size);
    uint8_t* decompressed_buffer = (uint8_t*)malloc(buffer_size);
    
    if (!compressed_buffer || !decompressed_buffer) {
        fprintf(stderr, "Error: Memory allocation failed for buffers\n");
        free(compressed_buffer);
        free(decompressed_buffer);
        fclose(output);
        return -1;
    }
    
    // Process each part
    uint64_t total_processed = 0;
    
    for (uint32_t part = 1; part <= total_parts; part++) {
        char* part_filename = get_part_filename(input_base, part);
        if (!part_filename) {
            free(compressed_buffer);
            free(decompressed_buffer);
            fclose(output);
            return -1;
        }
        
        FILE* part_file = fopen(part_filename, "rb");
        if (!part_file) {
            fprintf(stderr, "Error: Could not open part file %s\n", part_filename);
            free(part_filename);
            free(compressed_buffer);
            free(decompressed_buffer);
            fclose(output);
            return -1;
        }
        
        printf("Processing part %u/%u: %s\n", part, total_parts, part_filename);
        free(part_filename);
        
        // Skip header
        fseek(part_file, sizeof(ArchivePartHeader), SEEK_SET);
        
        // Process the part in chunks
        while (!feof(part_file)) {
            // Read a chunk of compressed data
            size_t bytes_read = fread(compressed_buffer, 1, buffer_size, part_file);
            if (bytes_read == 0) break;
            
            // Decompress the chunk
            size_t output_size = buffer_size;
            int decompressed_size = decompress_buffer(algorithm_index, compressed_buffer, bytes_read,
                                                     decompressed_buffer, &output_size);
            
            if (decompressed_size < 0) {
                fprintf(stderr, "Error: Decompression failed\n");
                fclose(part_file);
                free(compressed_buffer);
                free(decompressed_buffer);
                fclose(output);
                return -1;
            }
            
            // Write decompressed data to output
            if (fwrite(decompressed_buffer, 1, output_size, output) != output_size) {
                fprintf(stderr, "Error: Failed to write to output file\n");
                fclose(part_file);
                free(compressed_buffer);
                free(decompressed_buffer);
                fclose(output);
                return -1;
            }
            
            // Update progress
            total_processed += decompressed_size;
        }
        
        fclose(part_file);
        
        // Print progress if we know the total size
        if (header.total_size > 0) {
            printf("Progress: %llu / %llu bytes (%.1f%%)\n", 
                   (unsigned long long)total_processed,
                   (unsigned long long)header.total_size,
                   (float)total_processed * 100.0f / header.total_size);
        } else {
            printf("Progress: Processed %llu bytes\n", (unsigned long long)total_processed);
        }
    }
    
    // Clean up
    free(compressed_buffer);
    free(decompressed_buffer);
    fclose(output);
    
    printf("Split archive decompression completed\n");
    return 0;
} 