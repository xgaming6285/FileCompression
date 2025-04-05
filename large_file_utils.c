/**
 * Large File Utilities Implementation
 * Functions for handling files larger than available RAM
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "large_file_utils.h"

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
    writer->bytes_written = 0;
    writer->buffer_pos = 0;
    
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
    if (!writer || !writer->file || !data) {
        return -1;
    }
    
    size_t bytes_to_write = size;
    size_t data_offset = 0;
    
    while (bytes_to_write > 0) {
        // Calculate available space in buffer
        size_t space_available = writer->chunk_size - writer->buffer_pos;
        
        // Determine how much to copy to buffer
        size_t copy_size = (bytes_to_write < space_available) ? bytes_to_write : space_available;
        
        // Copy data to buffer
        memcpy(writer->buffer + writer->buffer_pos, data + data_offset, copy_size);
        writer->buffer_pos += copy_size;
        data_offset += copy_size;
        bytes_to_write -= copy_size;
        
        // If buffer is full, flush it
        if (writer->buffer_pos >= writer->chunk_size) {
            if (large_file_writer_flush(writer) != 0) {
                return -1;
            }
        }
    }
    
    return 0;
}

// Flush any remaining data in the buffer to the file
int large_file_writer_flush(LargeFileWriter* writer) {
    if (!writer || !writer->file) {
        return -1;
    }
    
    // Only flush if there's data in the buffer
    if (writer->buffer_pos > 0) {
        size_t written = fwrite(writer->buffer, 1, writer->buffer_pos, writer->file);
        
        if (written != writer->buffer_pos) {
            fprintf(stderr, "Error writing to file: %s\n", writer->filename);
            return -1;
        }
        
        // Update total bytes written and reset buffer position
        writer->bytes_written += written;
        writer->buffer_pos = 0;
    }
    
    // Flush file stream
    fflush(writer->file);
    
    return 0;
} 