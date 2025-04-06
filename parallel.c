/**
 * Parallel Compression Utility
 * Implementation file for multi-threaded compression
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "parallel.h"
#include "huffman.h"
#include "rle.h"

// Chunk information structure
typedef struct {
    uint8_t *data;      // Chunk data
    size_t size;        // Size of the chunk
    size_t original_offset; // Original offset in the file
    char *output_path;  // Temporary output path for this chunk
    CompressionAlgorithm *algorithm; // Compression algorithm to use
    int thread_id;      // Thread ID
} ChunkInfo;

// Initialize parallel compression subsystem
void init_parallel_compression(int num_threads) {
    (void)num_threads; // Mark as unused for now
    // Set up any global resources needed
    // Currently nothing needed beyond per-thread setup
}

// Get optimal number of threads based on system capabilities
int get_optimal_threads() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (num_cores > 0) ? num_cores : DEFAULT_THREADS;
#endif
}

// Helper function to create temporary filenames for chunks
char* create_temp_filename(const char* base_path, int thread_id) {
    char* temp_path = malloc(strlen(base_path) + 32);
    if (!temp_path) {
        perror("Memory allocation error");
        return NULL;
    }
    sprintf(temp_path, "%s.chunk%d.tmp", base_path, thread_id);
    return temp_path;
}

// Thread function for compressing chunks
void* compress_chunk_thread(void *arg) {
    ChunkInfo *chunk = (ChunkInfo*)arg;
    
    // Create temporary file for this chunk
    FILE *temp_file = fopen(chunk->output_path, "wb");
    if (!temp_file) {
        perror("Error creating temporary file");
        return NULL;
    }
    
    // Write the chunk header - original size and offset
    fwrite(&chunk->original_offset, sizeof(size_t), 1, temp_file);
    fwrite(&chunk->size, sizeof(size_t), 1, temp_file);
    
    // Create temporary input file for the chunk
    char temp_input_path[256];
    sprintf(temp_input_path, "chunk_input_%d.tmp", chunk->thread_id);
    FILE *temp_input = fopen(temp_input_path, "wb");
    if (!temp_input) {
        perror("Error creating temporary input file");
        fclose(temp_file);
        return NULL;
    }
    
    // Write chunk data to temporary input file
    fwrite(chunk->data, 1, chunk->size, temp_input);
    fclose(temp_input);
    
    // Close output file first so it can be reopened by compression algorithm
    fclose(temp_file);
    
    // Compress the chunk using the specified algorithm
    printf("Thread %d: Compressing chunk of size %zu\n", chunk->thread_id, chunk->size);
    chunk->algorithm->compress(temp_input_path, chunk->output_path);
    
    // Clean up temporary input file
    remove(temp_input_path);
    
    return NULL;
}

// Thread function for decompressing chunks
void* decompress_chunk_thread(void *arg) {
    ChunkInfo *chunk = (ChunkInfo*)arg;
    
    // Create temporary input file for the chunk
    char temp_input_path[256];
    sprintf(temp_input_path, "chunk_input_%d.tmp", chunk->thread_id);
    FILE *temp_input = fopen(temp_input_path, "wb");
    if (!temp_input) {
        perror("Error creating temporary input file");
        return NULL;
    }
    
    // Write chunk data to temporary input file
    fwrite(chunk->data, 1, chunk->size, temp_input);
    fclose(temp_input);
    
    // Decompress the chunk using the specified algorithm
    printf("Thread %d: Decompressing chunk\n", chunk->thread_id);
    chunk->algorithm->decompress(temp_input_path, chunk->output_path);
    
    // Clean up temporary input file
    remove(temp_input_path);
    
    return NULL;
}

// Compress a file in parallel using multiple threads
int compress_file_parallel(const char *input_file, const char *output_file, CompressionAlgorithm *algorithm, int num_threads) {
    FILE *in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Get file size
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    if (file_size == 0) {
        printf("Empty input file\n");
        fclose(in);
        return 1;
    }
    
    // Determine optimal number of threads if not specified
    if (num_threads <= 0) {
        num_threads = get_optimal_threads();
    }
    if (num_threads > MAX_THREADS) {
        num_threads = MAX_THREADS;
    }
    
    // Adjust number of threads for small files
    if (file_size < num_threads * 1024) {  // Less than 1KB per thread
        num_threads = 1;
    }
    
    printf("Using %d threads for compression\n", num_threads);
    
    // Calculate chunk size
    size_t chunk_size = file_size / num_threads;
    // Ensure chunk size is at least 1KB
    if (chunk_size < 1024 && num_threads > 1) {
        chunk_size = 1024;
        num_threads = file_size / chunk_size + (file_size % chunk_size != 0);
        printf("Adjusted to %d threads based on minimum chunk size\n", num_threads);
    }
    
    // Read file data
    uint8_t *file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        printf("Memory allocation error\n");
        fclose(in);
        return 1;
    }
    
    fread(file_data, 1, file_size, in);
    fclose(in);
    
    // Create and initialize chunks
    ChunkInfo *chunks = (ChunkInfo*)malloc(num_threads * sizeof(ChunkInfo));
    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!chunks || !threads) {
        printf("Memory allocation error\n");
        free(file_data);
        if (chunks) free(chunks);
        if (threads) free(threads);
        return 1;
    }
    
    // Prepare output file
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        free(file_data);
        free(chunks);
        free(threads);
        return 1;
    }
    
    // Write header: number of chunks
    fwrite(&num_threads, sizeof(int), 1, out);
    
    // Close now, we'll reopen later to append compressed chunks
    fclose(out);
    
    // Create and launch threads for each chunk
    for (int i = 0; i < num_threads; i++) {
        size_t offset = i * chunk_size;
        size_t current_chunk_size;
        
        // Calculate chunk size (last chunk may be smaller)
        if (i == num_threads - 1) {
            current_chunk_size = file_size - offset;
        } else {
            current_chunk_size = chunk_size;
        }
        
        chunks[i].data = file_data + offset;
        chunks[i].size = current_chunk_size;
        chunks[i].original_offset = offset;
        chunks[i].algorithm = algorithm;
        chunks[i].thread_id = i;
        chunks[i].output_path = create_temp_filename(output_file, i);
        
        if (!chunks[i].output_path) {
            printf("Failed to create temporary filename\n");
            // Clean up previous chunks
            for (int j = 0; j < i; j++) {
                free(chunks[j].output_path);
            }
            free(file_data);
            free(chunks);
            free(threads);
            return 1;
        }
        
        // Create thread
        if (pthread_create(&threads[i], NULL, compress_chunk_thread, &chunks[i]) != 0) {
            perror("Thread creation failed");
            // Clean up
            for (int j = 0; j <= i; j++) {
                free(chunks[j].output_path);
            }
            free(file_data);
            free(chunks);
            free(threads);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Combine all compressed chunks into the final file
    out = fopen(output_file, "ab");
    if (!out) {
        printf("Error reopening output file: %s\n", output_file);
        // Clean up
        for (int i = 0; i < num_threads; i++) {
            free(chunks[i].output_path);
            remove(chunks[i].output_path);
        }
        free(file_data);
        free(chunks);
        free(threads);
        return 1;
    }
    
    // Combine chunks
    for (int i = 0; i < num_threads; i++) {
        FILE *chunk_file = fopen(chunks[i].output_path, "rb");
        if (!chunk_file) {
            printf("Error opening chunk file: %s\n", chunks[i].output_path);
            continue;
        }
        
        // Get chunk file size
        fseek(chunk_file, 0, SEEK_END);
        long chunk_file_size = ftell(chunk_file);
        fseek(chunk_file, 0, SEEK_SET);
        
        // Write chunk size to output
        fwrite(&chunk_file_size, sizeof(long), 1, out);
        
        // Copy chunk data
        uint8_t buffer[4096];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), chunk_file)) > 0) {
            fwrite(buffer, 1, bytes_read, out);
        }
        
        fclose(chunk_file);
        remove(chunks[i].output_path);
        free(chunks[i].output_path);
    }
    
    fclose(out);
    
    // Clean up
    free(file_data);
    free(chunks);
    free(threads);
    
    printf("Parallel compression completed successfully\n");
    return 0;
}

// Decompress a file in parallel using multiple threads
int decompress_file_parallel(const char *input_file, const char *output_file, CompressionAlgorithm *algorithm, int num_threads) {
    FILE *in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Read header: number of chunks
    int chunk_count;
    if (fread(&chunk_count, sizeof(int), 1, in) != 1) {
        printf("Error reading chunk count from file\n");
        fclose(in);
        return 1;
    }
    
    printf("Decompressing file with %d chunks\n", chunk_count);
    
    // Determine optimal number of threads if not specified
    if (num_threads <= 0) {
        num_threads = get_optimal_threads();
    }
    if (num_threads > MAX_THREADS || num_threads > chunk_count) {
        num_threads = (chunk_count < MAX_THREADS) ? chunk_count : MAX_THREADS;
    }
    
    printf("Using %d threads for decompression\n", num_threads);
    
    // Create arrays for chunk info and threads
    ChunkInfo *chunks = (ChunkInfo*)malloc(chunk_count * sizeof(ChunkInfo));
    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!chunks || !threads) {
        printf("Memory allocation error\n");
        fclose(in);
        if (chunks) free(chunks);
        if (threads) free(threads);
        return 1;
    }
    
    // Read chunk information
    for (int i = 0; i < chunk_count; i++) {
        long chunk_size;
        if (fread(&chunk_size, sizeof(long), 1, in) != 1) {
            printf("Error reading chunk size\n");
            fclose(in);
            free(chunks);
            free(threads);
            return 1;
        }
        
        chunks[i].data = (uint8_t*)malloc(chunk_size);
        if (!chunks[i].data) {
            printf("Memory allocation error for chunk %d\n", i);
            // Clean up previous chunks
            for (int j = 0; j < i; j++) {
                free(chunks[j].data);
            }
            fclose(in);
            free(chunks);
            free(threads);
            return 1;
        }
        
        chunks[i].size = chunk_size;
        chunks[i].algorithm = algorithm;
        chunks[i].thread_id = i;
        chunks[i].output_path = create_temp_filename(output_file, i);
        
        if (!chunks[i].output_path) {
            printf("Failed to create temporary filename for chunk %d\n", i);
            // Clean up
            for (int j = 0; j <= i; j++) {
                if (j < i) free(chunks[j].data);
                if (chunks[j].output_path) free(chunks[j].output_path);
            }
            fclose(in);
            free(chunks);
            free(threads);
            return 1;
        }
        
        // Read chunk data
        size_t read_size = fread(chunks[i].data, 1, chunk_size, in);
        if ((long)read_size != (long)chunk_size) {
            fprintf(stderr, "Error reading chunk %d: Expected %ld bytes, got %zu\n", 
                    i, (long)chunk_size, read_size);
            // Clean up
            for (int j = 0; j <= i; j++) {
                free(chunks[j].data);
                free(chunks[j].output_path);
            }
            fclose(in);
            free(chunks);
            free(threads);
            return 1;
        }
    }
    
    fclose(in);
    
    // Process chunks in batches to control memory usage
    int remaining_chunks = chunk_count;
    int current_chunk = 0;
    
    while (remaining_chunks > 0) {
        int current_batch_size = (remaining_chunks < num_threads) ? remaining_chunks : num_threads;
        
        // Launch threads for current batch
        for (int i = 0; i < current_batch_size; i++) {
            int chunk_idx = current_chunk + i;
            
            if (pthread_create(&threads[i], NULL, decompress_chunk_thread, &chunks[chunk_idx]) != 0) {
                perror("Thread creation failed");
                // Clean up
                for (int j = 0; j < chunk_count; j++) {
                    free(chunks[j].data);
                    free(chunks[j].output_path);
                    if (j < chunk_idx) remove(chunks[j].output_path);
                }
                free(chunks);
                free(threads);
                return 1;
            }
        }
        
        // Wait for all threads in this batch to complete
        for (int i = 0; i < current_batch_size; i++) {
            pthread_join(threads[i], NULL);
        }
        
        current_chunk += current_batch_size;
        remaining_chunks -= current_batch_size;
    }
    
    // Combine all decompressed chunks into the final file
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Error opening output file: %s\n", output_file);
        // Clean up
        for (int i = 0; i < chunk_count; i++) {
            free(chunks[i].data);
            free(chunks[i].output_path);
            remove(chunks[i].output_path);
        }
        free(chunks);
        free(threads);
        return 1;
    }
    
    // Sort chunks by original offset before combining
    // (We'd implement a sort here if needed)
    
    // Combine chunks
    for (int i = 0; i < chunk_count; i++) {
        FILE *chunk_file = fopen(chunks[i].output_path, "rb");
        if (!chunk_file) {
            printf("Error opening decompressed chunk file: %s\n", chunks[i].output_path);
            continue;
        }
        
        // Copy chunk data
        uint8_t buffer[4096];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), chunk_file)) > 0) {
            fwrite(buffer, 1, bytes_read, out);
        }
        
        fclose(chunk_file);
        remove(chunks[i].output_path);
    }
    
    fclose(out);
    
    // Clean up
    for (int i = 0; i < chunk_count; i++) {
        free(chunks[i].data);
        free(chunks[i].output_path);
    }
    free(chunks);
    free(threads);
    
    printf("Parallel decompression completed successfully\n");
    return 0;
} 