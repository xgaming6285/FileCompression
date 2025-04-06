/**
 * Large File Utilities Implementation
 * Functions for handling files larger than available RAM
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "large_file_utils.h"

// CRC32 table for fast CRC calculation
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

// Initialize CRC32 table
static void init_crc32_table() {
    if (crc32_table_initialized) return;
    
    uint32_t polynomial = 0xEDB88320;
    for (int i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) {
                c = polynomial ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        crc32_table[i] = c;
    }
    
    crc32_table_initialized = 1;
}

// Calculate CRC32 checksum
static uint32_t calculate_crc32(const uint8_t* data, size_t size) {
    // Initialize the CRC32 table if needed
    if (!crc32_table_initialized) {
        init_crc32_table();
    }
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Calculate MD5 hash (simplified implementation - in a real application, use a library like OpenSSL)
static void calculate_md5(const uint8_t* data, size_t size, uint8_t* hash) {
    // Placeholder - in a real implementation, use a proper MD5 library
    memset(hash, 0, 16);
    
    // Simple hash function for demonstration
    for (size_t i = 0; i < size; i++) {
        hash[i % 16] ^= data[i];
    }
}

// Calculate SHA256 hash (simplified implementation - in a real application, use a library like OpenSSL)
static void calculate_sha256(const uint8_t* data, size_t size, uint8_t* hash) {
    // Placeholder - in a real implementation, use a proper SHA256 library
    memset(hash, 0, 32);
    
    // Simple hash function for demonstration
    for (size_t i = 0; i < size; i++) {
        hash[i % 32] ^= data[i];
    }
}

// Calculate checksums for a buffer
void calculate_checksum(const uint8_t* data, size_t size, ChecksumData* checksum, ChecksumType type) {
    if (!data || !checksum) return;
    
    checksum->type = type;
    
    switch (type) {
        case CHECKSUM_CRC32:
            checksum->crc32 = calculate_crc32(data, size);
            break;
        
        case CHECKSUM_MD5:
            calculate_md5(data, size, checksum->md5);
            break;
        
        case CHECKSUM_SHA256:
            calculate_sha256(data, size, checksum->sha256);
            break;
        
        case CHECKSUM_NONE:
        default:
            break;
    }
}

// Verify data against a checksum
int verify_checksum(const uint8_t* data, size_t size, const ChecksumData* checksum) {
    if (!data || !checksum) return 0;
    
    switch (checksum->type) {
        case CHECKSUM_CRC32: {
            uint32_t calculated_crc = calculate_crc32(data, size);
            return (calculated_crc == checksum->crc32) ? 1 : 0;
        }
        
        case CHECKSUM_MD5: {
            uint8_t calculated_md5[16];
            calculate_md5(data, size, calculated_md5);
            return (memcmp(calculated_md5, checksum->md5, 16) == 0) ? 1 : 0;
        }
        
        case CHECKSUM_SHA256: {
            uint8_t calculated_sha256[32];
            calculate_sha256(data, size, calculated_sha256);
            return (memcmp(calculated_sha256, checksum->sha256, 32) == 0) ? 1 : 0;
        }
        
        case CHECKSUM_NONE:
        default:
            return 1;  // No checksum means verification always passes
    }
}

// Get the size of the checksum data based on type
size_t get_checksum_size(ChecksumType type) {
    switch (type) {
        case CHECKSUM_CRC32:
            return sizeof(uint32_t);
        case CHECKSUM_MD5:
            return 16;
        case CHECKSUM_SHA256:
            return 32;
        case CHECKSUM_NONE:
        default:
            return 0;
    }
}

// Get a string representation of the checksum
char* checksum_to_string(const ChecksumData* checksum, char* buffer, size_t buffer_size) {
    if (!checksum || !buffer || buffer_size == 0) {
        return NULL;
    }
    
    switch (checksum->type) {
        case CHECKSUM_CRC32:
            snprintf(buffer, buffer_size, "CRC32: %08X", checksum->crc32);
            break;
        
        case CHECKSUM_MD5: {
            char* ptr = buffer;
            size_t remaining = buffer_size;
            int bytes_written = snprintf(ptr, remaining, "MD5: ");
            
            if (bytes_written > 0) {
                ptr += bytes_written;
                remaining -= bytes_written;
                
                for (int i = 0; i < 16 && remaining > 2; i++) {
                    bytes_written = snprintf(ptr, remaining, "%02X", checksum->md5[i]);
                    ptr += bytes_written;
                    remaining -= bytes_written;
                }
            }
            break;
        }
        
        case CHECKSUM_SHA256: {
            char* ptr = buffer;
            size_t remaining = buffer_size;
            int bytes_written = snprintf(ptr, remaining, "SHA256: ");
            
            if (bytes_written > 0) {
                ptr += bytes_written;
                remaining -= bytes_written;
                
                for (int i = 0; i < 32 && remaining > 2; i++) {
                    bytes_written = snprintf(ptr, remaining, "%02X", checksum->sha256[i]);
                    ptr += bytes_written;
                    remaining -= bytes_written;
                }
            }
            break;
        }
        
        case CHECKSUM_NONE:
        default:
            snprintf(buffer, buffer_size, "No checksum");
            break;
    }
    
    return buffer;
}

// Initialize a large file reader
LargeFileReader* large_file_reader_init(const char* filename, size_t chunk_size) {
    LargeFileReader* reader = (LargeFileReader*)malloc(sizeof(LargeFileReader));
    if (!reader) {
        fprintf(stderr, "Memory allocation error for LargeFileReader\n");
        return NULL;
    }
    
    // Initialize with zeros
    memset(reader, 0, sizeof(LargeFileReader));
    
    // Open the file
    reader->file = fopen(filename, "rb");
    if (!reader->file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        free(reader);
        return NULL;
    }
    
    // Set chunk size to default if not specified
    if (chunk_size == 0) {
        chunk_size = DEFAULT_CHUNK_SIZE;
    }
    
    // Allocate filename string
    reader->filename = strdup(filename);
    if (!reader->filename) {
        fprintf(stderr, "Memory allocation error for filename\n");
        fclose(reader->file);
        free(reader);
        return NULL;
    }
    
    // Get file size using fseek/ftell
    if (fseek(reader->file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error seeking in file: %s\n", filename);
        fclose(reader->file);
        free(reader->filename);
        free(reader);
        return NULL;
    }
    
    // Get file size
    int64_t size = ftell(reader->file);
    if (size < 0) {
        fprintf(stderr, "Error getting file size: %s\n", filename);
        fclose(reader->file);
        free(reader->filename);
        free(reader);
        return NULL;
    }
    
    reader->file_size = (uint64_t)size;
    
    // Return to beginning of file
    if (fseek(reader->file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking to start of file: %s\n", filename);
        fclose(reader->file);
        free(reader->filename);
        free(reader);
        return NULL;
    }
    
    // Allocate buffer for reading
    reader->buffer = (uint8_t*)malloc(chunk_size);
    if (!reader->buffer) {
        fprintf(stderr, "Memory allocation error for read buffer\n");
        fclose(reader->file);
        free(reader->filename);
        free(reader);
        return NULL;
    }
    
    // Set remaining fields
    reader->chunk_size = chunk_size;
    reader->current_position = 0;
    reader->eof_reached = 0;
    reader->checksum_type = CHECKSUM_NONE;
    
    return reader;
}

// Initialize a large file reader with checksum verification
LargeFileReader* large_file_reader_init_with_checksum(const char* filename, size_t chunk_size, ChecksumType checksum_type) {
    LargeFileReader* reader = large_file_reader_init(filename, chunk_size);
    
    if (reader) {
        reader->checksum_type = checksum_type;
    }
    
    return reader;
}

// Free resources used by a large file reader
void large_file_reader_free(LargeFileReader* reader) {
    if (!reader) return;
    
    if (reader->file) {
        fclose(reader->file);
    }
    
    if (reader->filename) {
        free(reader->filename);
    }
    
    if (reader->buffer) {
        free(reader->buffer);
    }
    
    free(reader);
}

// Read the next chunk from a file
uint8_t* large_file_reader_next_chunk(LargeFileReader* reader, size_t* bytes_read) {
    if (!reader || !reader->file || !bytes_read || reader->eof_reached) {
        if (bytes_read) *bytes_read = 0;
        return NULL;
    }
    
    // Check if we need to read checksums
    ChecksumData checksum;
    
    if (reader->checksum_type != CHECKSUM_NONE) {
        // Read the checksum type first (always 4 bytes)
        uint32_t stored_type;
        if (fread(&stored_type, sizeof(uint32_t), 1, reader->file) != 1) {
            fprintf(stderr, "Error reading checksum type\n");
            *bytes_read = 0;
            return NULL;
        }
        
        reader->current_position += sizeof(uint32_t);
        
        // Read checksum data based on type
        if (stored_type > CHECKSUM_NONE && stored_type <= CHECKSUM_SHA256) {
            checksum.type = (ChecksumType)stored_type;
            
            switch (checksum.type) {
                case CHECKSUM_CRC32:
                    if (fread(&checksum.crc32, sizeof(uint32_t), 1, reader->file) != 1) {
                        fprintf(stderr, "Error reading CRC32 checksum\n");
                        *bytes_read = 0;
                        return NULL;
                    }
                    reader->current_position += sizeof(uint32_t);
                    break;
                
                case CHECKSUM_MD5:
                    if (fread(checksum.md5, 16, 1, reader->file) != 1) {
                        fprintf(stderr, "Error reading MD5 checksum\n");
                        *bytes_read = 0;
                        return NULL;
                    }
                    reader->current_position += 16;
                    break;
                
                case CHECKSUM_SHA256:
                    if (fread(checksum.sha256, 32, 1, reader->file) != 1) {
                        fprintf(stderr, "Error reading SHA256 checksum\n");
                        *bytes_read = 0;
                        return NULL;
                    }
                    reader->current_position += 32;
                    break;
                
                default:
                    break;
            }
        }
        
        // Read data length (4 bytes)
        uint32_t data_length;
        if (fread(&data_length, sizeof(uint32_t), 1, reader->file) != 1) {
            fprintf(stderr, "Error reading data length\n");
            *bytes_read = 0;
            return NULL;
        }
        
        reader->current_position += sizeof(uint32_t);
        
        // Calculate remaining bytes in file
        uint64_t remaining = reader->file_size - reader->current_position;
        
        // Determine how many bytes to read (use the lesser of data_length or chunk_size)
        size_t to_read = (data_length < reader->chunk_size) ? data_length : reader->chunk_size;
        
        // If there's nothing left to read, return EOF
        if (to_read == 0 || to_read > remaining) {
            reader->eof_reached = 1;
            *bytes_read = 0;
            return NULL;
        }
        
        // Read data into buffer
        size_t actual_read = fread(reader->buffer, 1, to_read, reader->file);
        
        // Update position
        reader->current_position += actual_read;
        
        // Check if we've reached EOF
        if (actual_read < to_read || reader->current_position >= reader->file_size) {
            reader->eof_reached = 1;
        }
        
        // Verify checksum
        if (actual_read > 0 && !verify_checksum(reader->buffer, actual_read, &checksum)) {
            fprintf(stderr, "Checksum verification failed! Data may be corrupted.\n");
            // You could choose to return NULL here to indicate failure,
            // but we'll let the caller decide how to handle the corruption
        }
        
        // Return results
        *bytes_read = actual_read;
        return reader->buffer;
    } else {
        // Standard file reading without checksums
        
        // Calculate remaining bytes in file
        uint64_t remaining = reader->file_size - reader->current_position;
        
        // Determine how many bytes to read
        size_t to_read = (remaining < reader->chunk_size) ? (size_t)remaining : reader->chunk_size;
        
        // If there's nothing left to read, return EOF
        if (to_read == 0) {
            reader->eof_reached = 1;
            *bytes_read = 0;
            return NULL;
        }
        
        // Read data into buffer
        size_t actual_read = fread(reader->buffer, 1, to_read, reader->file);
        
        // Update position
        reader->current_position += actual_read;
        
        // Check if we've reached EOF
        if (actual_read < to_read || reader->current_position >= reader->file_size) {
            reader->eof_reached = 1;
        }
        
        // Return results
        *bytes_read = actual_read;
        return reader->buffer;
    }
}

// Reset reader to the beginning of the file
int large_file_reader_reset(LargeFileReader* reader) {
    if (!reader || !reader->file) {
        return -1;
    }
    
    // Seek to beginning of file
    if (fseek(reader->file, 0, SEEK_SET) != 0) {
        return -1;
    }
    
    // Reset position and EOF flag
    reader->current_position = 0;
    reader->eof_reached = 0;
    
    return 0;
}

// Initialize a large file writer
LargeFileWriter* large_file_writer_init(const char* filename, size_t chunk_size) {
    LargeFileWriter* writer = (LargeFileWriter*)malloc(sizeof(LargeFileWriter));
    if (!writer) {
        fprintf(stderr, "Memory allocation error for LargeFileWriter\n");
        return NULL;
    }
    
    // Initialize with zeros
    memset(writer, 0, sizeof(LargeFileWriter));
    
    // Open the file
    writer->file = fopen(filename, "wb");
    if (!writer->file) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        free(writer);
        return NULL;
    }
    
    // Set chunk size to default if not specified
    if (chunk_size == 0) {
        chunk_size = DEFAULT_CHUNK_SIZE;
    }
    
    // Allocate filename string
    writer->filename = strdup(filename);
    if (!writer->filename) {
        fprintf(stderr, "Memory allocation error for filename\n");
        fclose(writer->file);
        free(writer);
        return NULL;
    }
    
    // Allocate buffer for writing
    writer->buffer = (uint8_t*)malloc(chunk_size);
    if (!writer->buffer) {
        fprintf(stderr, "Memory allocation error for write buffer\n");
        fclose(writer->file);
        free(writer->filename);
        free(writer);
        return NULL;
    }
    
    // Set remaining fields
    writer->chunk_size = chunk_size;
    writer->buffer_pos = 0;
    writer->bytes_written = 0;
    writer->checksum_type = CHECKSUM_NONE;
    
    return writer;
}

// Initialize a large file writer with checksum generation
LargeFileWriter* large_file_writer_init_with_checksum(const char* filename, size_t chunk_size, ChecksumType checksum_type) {
    LargeFileWriter* writer = large_file_writer_init(filename, chunk_size);
    
    if (writer) {
        writer->checksum_type = checksum_type;
    }
    
    return writer;
}

// Free resources used by a large file writer
void large_file_writer_free(LargeFileWriter* writer) {
    if (!writer) return;
    
    // Flush any remaining data
    large_file_writer_flush(writer);
    
    if (writer->file) {
        fclose(writer->file);
    }
    
    if (writer->filename) {
        free(writer->filename);
    }
    
    if (writer->buffer) {
        free(writer->buffer);
    }
    
    free(writer);
}

// Write data to the output file
int large_file_writer_write(LargeFileWriter* writer, const uint8_t* data, size_t size) {
    if (!writer || !writer->file || !data || size == 0) {
        return -1;
    }
    
    if (writer->checksum_type != CHECKSUM_NONE) {
        // With checksum, we write directly to file with header and checksum
        ChecksumData checksum;
        
        // Calculate checksum
        calculate_checksum(data, size, &checksum, writer->checksum_type);
        
        // Write checksum type
        uint32_t type_val = (uint32_t)writer->checksum_type;
        if (fwrite(&type_val, sizeof(uint32_t), 1, writer->file) != 1) {
            return -1;
        }
        
        // Write checksum data
        switch (writer->checksum_type) {
            case CHECKSUM_CRC32:
                if (fwrite(&checksum.crc32, sizeof(uint32_t), 1, writer->file) != 1) {
                    return -1;
                }
                break;
            
            case CHECKSUM_MD5:
                if (fwrite(checksum.md5, 16, 1, writer->file) != 1) {
                    return -1;
                }
                break;
            
            case CHECKSUM_SHA256:
                if (fwrite(checksum.sha256, 32, 1, writer->file) != 1) {
                    return -1;
                }
                break;
            
            default:
                break;
        }
        
        // Write data length
        uint32_t data_length = (uint32_t)size;
        if (fwrite(&data_length, sizeof(uint32_t), 1, writer->file) != 1) {
            return -1;
        }
        
        // Write data
        if (fwrite(data, 1, size, writer->file) != size) {
            return -1;
        }
        
        writer->bytes_written += size;
        return 0;
    } else {
        // Without checksums, we buffer data for efficiency
        
        // If buffer is too small, write directly
        if (size >= writer->chunk_size) {
            // First flush any buffered data
            if (writer->buffer_pos > 0) {
                if (fwrite(writer->buffer, 1, writer->buffer_pos, writer->file) != writer->buffer_pos) {
                    return -1;
                }
                writer->bytes_written += writer->buffer_pos;
                writer->buffer_pos = 0;
            }
            
            // Write data directly
            if (fwrite(data, 1, size, writer->file) != size) {
                return -1;
            }
            writer->bytes_written += size;
        } else {
            // Check if we need to flush the buffer
            if (writer->buffer_pos + size > writer->chunk_size) {
                // Flush buffer
                if (fwrite(writer->buffer, 1, writer->buffer_pos, writer->file) != writer->buffer_pos) {
                    return -1;
                }
                writer->bytes_written += writer->buffer_pos;
                writer->buffer_pos = 0;
            }
            
            // Copy data to buffer
            memcpy(writer->buffer + writer->buffer_pos, data, size);
            writer->buffer_pos += size;
        }
        
        return 0;
    }
}

// Flush any remaining data in the buffer to the file
int large_file_writer_flush(LargeFileWriter* writer) {
    if (!writer || !writer->file) {
        return -1;
    }
    
    if (writer->buffer_pos > 0) {
        if (writer->checksum_type != CHECKSUM_NONE) {
            // For checksum mode, we need to write with full header
            
            ChecksumData checksum;
            calculate_checksum(writer->buffer, writer->buffer_pos, &checksum, writer->checksum_type);
            
            // Write checksum type
            uint32_t type_val = (uint32_t)writer->checksum_type;
            if (fwrite(&type_val, sizeof(uint32_t), 1, writer->file) != 1) {
                return -1;
            }
            
            // Write checksum data
            switch (writer->checksum_type) {
                case CHECKSUM_CRC32:
                    if (fwrite(&checksum.crc32, sizeof(uint32_t), 1, writer->file) != 1) {
                        return -1;
                    }
                    break;
                
                case CHECKSUM_MD5:
                    if (fwrite(checksum.md5, 16, 1, writer->file) != 1) {
                        return -1;
                    }
                    break;
                
                case CHECKSUM_SHA256:
                    if (fwrite(checksum.sha256, 32, 1, writer->file) != 1) {
                        return -1;
                    }
                    break;
                
                default:
                    break;
            }
            
            // Write data length
            uint32_t data_length = (uint32_t)writer->buffer_pos;
            if (fwrite(&data_length, sizeof(uint32_t), 1, writer->file) != 1) {
                return -1;
            }
        }
        
        // Write data
        if (fwrite(writer->buffer, 1, writer->buffer_pos, writer->file) != writer->buffer_pos) {
            return -1;
        }
        
        writer->bytes_written += writer->buffer_pos;
        writer->buffer_pos = 0;
    }
    
    return 0;
} 