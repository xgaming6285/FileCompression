/**
 * File Compression Utility
 * Supports multiple compression algorithms
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "filecompressor.h"
#include "compression.h"
#include "parallel.h"
#include "encryption.h"
#include "progressive.h"
#include "split_archive.h"
#include "deduplication.h"

// Global variables
OptimizationGoal opt_goal = OPT_NONE;
static size_t buffer_size = 8192; // Default buffer size (8KB)

// Get optimization goal
OptimizationGoal get_optimization_goal() {
    return opt_goal;
}

// Get buffer size
size_t get_buffer_size() {
    return buffer_size;
}

void print_usage() {
    printf("Usage: filecompressor [options] <input_file> [output_file]\n");
    printf("Options:\n");
    printf("  -c [algorithm]  Compress the input file\n");
    printf("  -d [algorithm]  Decompress the input file\n");
    printf("  -a              List available compression algorithms\n");
    printf("  -t [threads]    Number of threads to use (default: auto)\n");
    printf("  -k [key]        Encryption key for encrypted algorithms\n");
    printf("  -O [goal]       Optimization goal: speed or size\n");
    printf("  -B [size]       Buffer size in bytes (default: 8192)\n");
    printf("  -L              Enable large file mode for files larger than available RAM\n");
    printf("  -I [type]       Enable integrity verification with checksum (1=CRC32, 2=MD5, 3=SHA256)\n");
    printf("  -p              Enable profiling\n");
    printf("  -P              Use progressive format (supports partial decompression)\n");
    printf("  -R [start-end]  Decompress only a range of blocks (requires -P)\n");
    printf("  -S [output]     Stream output to a callback function (e.g., display or process)\n");
    printf("  -X              Enable split archive mode (create multiple files)\n");
    printf("  -M [size]       Maximum size in bytes for each split archive part (default: 100MB)\n");
    printf("  -D              Enable deduplication (identify and eliminate redundant data)\n");
    printf("  -C [size]       Chunk size for deduplication in bytes (default: 64KB)\n");
    printf("  -H [algorithm]  Hash algorithm for deduplication (0=SHA1, 1=MD5, 2=CRC32, 3=XXH64, default: 0)\n");
    printf("  -V [mode]       Deduplication mode (0=fixed, 1=variable, 2=smart, default: 0)\n");
    printf("  -h              Display this help message\n");
    printf("\n");
    printf("If algorithm is not specified, Huffman coding (0) is used by default.\n");
    printf("Use -a to see available algorithms and their indices.\n");
    printf("Examples:\n");
    printf("  filecompressor -c 0 input.txt                   # Compress with Huffman\n");
    printf("  filecompressor -d input.txt.huf                 # Decompress Huffman file\n");
    printf("  filecompressor -c 4 -O speed input.txt          # Compress with LZ77 optimized for speed\n");
    printf("  filecompressor -c 4 -O size -B 16384 input.txt  # Compress with LZ77 optimized for size\n");
    printf("  filecompressor -c 0 -L -B 1048576 largefile.txt # Compress large file with 1MB chunks\n");
    printf("  filecompressor -c 0 -I 1 input.txt              # Compress with CRC32 integrity verification\n");
    printf("  filecompressor -c 0 -P input.txt                # Compress with progressive format\n");
    printf("  filecompressor -d -P -R 5-10 input.prog out.txt # Decompress blocks 5-10 only\n");
    printf("  filecompressor -c 0 -X input.txt                # Create split archive with default part size\n");
    printf("  filecompressor -c 0 -X -M 10485760 input.txt    # Create split archive with 10MB part size\n");
    printf("  filecompressor -d input.txt output.txt -X       # Decompress split archive\n");
}

// External key for encrypted algorithms
static char encryption_key[256] = "default_encryption_key"; // Default key

// Get encryption key
const char* get_encryption_key() {
    return encryption_key;
}

// Set encryption key
void set_encryption_key(const char* key) {
    if (key && strlen(key) > 0) {
        strncpy(encryption_key, key, sizeof(encryption_key) - 1);
        encryption_key[sizeof(encryption_key) - 1] = '\0'; // Ensure null termination
    }
}

// Helper function to check if output file was provided
int output_file_provided(int argc, char *argv[], const char *option) {
    if (strcmp(option, "-c") == 0 || strcmp(option, "-d") == 0) {
        // Check if there's an algorithm index
        if (argc > 3) {
            char *end;
            long idx = strtol(argv[3], &end, 10);
            
            if (end != argv[3] && *end == '\0' && idx >= 0 && idx < get_algorithm_count()) {
                // Algorithm index provided
                return argc > 4;
            } else {
                // No algorithm index
                return argc > 3;
            }
        }
    }
    return 0;
}

// Callback function for streaming output
int stream_output_callback(const uint8_t* data, size_t size, void* user_data) {
    FILE* output = (FILE*)user_data;
    
    // Check if this is stdout
    if (output == stdout) {
        // For text data, print to stdout
        // In a real application, we'd have more sophisticated handling here
        for (size_t i = 0; i < size && i < 100; i++) {
            putchar(data[i]);
        }
        
        if (size > 100) {
            printf("\n... (showing first 100 bytes only) ...\n");
        }
        
        return 0;
    }
    
    // Otherwise, write to the file
    if (fwrite(data, 1, size, output) != size) {
        fprintf(stderr, "Error writing to output stream\n");
        return 1; // Signal to stop processing
    }
    
    return 0; // Continue processing
}

int main(int argc, char *argv[]) {
    // Initialize compression algorithms
    init_compression_algorithms();
    
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Process command-line arguments
    int i = 1;
    int compress_mode = -1; // -1 = unset, 0 = decompress, 1 = compress
    int algorithm_index = -1; // Default algorithm (will be set later)
    char* input_file = NULL;
    char* output_file = NULL;
    int profiling_enabled = 0;
    int large_file_mode = 0;  // Add large file mode flag
    int progressive_mode = 0; // Progressive format flag
    int split_mode = 0;       // Split archive mode flag
    uint64_t max_part_size = DEFAULT_SPLIT_SIZE; // Default max part size for split archives
    uint32_t start_block = 0, end_block = UINT32_MAX; // For partial decompression
    int range_specified = 0;
    int stream_mode = 0;
    ChecksumType checksum_type = CHECKSUM_NONE; // Default to no checksum
    
    // Add deduplication flags
    int deduplication_enabled = 0;
    size_t dedup_chunk_size = DEFAULT_DEDUP_CHUNK_SIZE;
    DedupHashAlgorithm dedup_hash_algorithm = DEDUP_HASH_SHA1;
    DedupMode dedup_mode = DEDUP_MODE_FIXED;
    
    while (i < argc) {
        char *arg = argv[i];
        
        if (arg[0] == '-') {
            // Option
            if (strcmp(arg, "-a") == 0) {
                print_available_algorithms();
                return 0;
            } else if (strcmp(arg, "-h") == 0) {
                print_usage();
                return 0;
            } else if (strcmp(arg, "-t") == 0) {
                // Thread count
                if (i + 1 < argc) {
                    int thread_count = atoi(argv[i + 1]);
                    set_thread_count(thread_count);
                    i += 2;
                } else {
                    printf("Error: Missing thread count after -t option\n");
                    return 1;
                }
            } else if (strcmp(arg, "-I") == 0) {
                // Checksum type
                if (i + 1 < argc) {
                    int type = atoi(argv[i + 1]);
                    if (type >= CHECKSUM_NONE && type <= CHECKSUM_SHA256) {
                        checksum_type = (ChecksumType)type;
                        
                        const char* checksum_names[] = {
                            "None", "CRC32", "MD5", "SHA256"
                        };
                        
                        printf("Integrity verification: %s\n", checksum_names[checksum_type]);
                    } else {
                        printf("Error: Invalid checksum type. Use 0 (none), 1 (CRC32), 2 (MD5), or 3 (SHA256)\n");
                        print_usage();
                        return 1;
                    }
                    i += 2;
                } else {
                    printf("Error: Missing checksum type after -I\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-O") == 0) {
                // Optimization goal
                if (i + 1 < argc) {
                    if (strcmp(argv[i + 1], "speed") == 0) {
                        opt_goal = OPT_SPEED;
                        printf("Optimization goal: SPEED\n");
                    } else if (strcmp(argv[i + 1], "size") == 0) {
                        opt_goal = OPT_SIZE;
                        printf("Optimization goal: SIZE\n");
                    } else {
                        printf("Error: Invalid optimization goal. Use 'speed' or 'size'\n");
                        print_usage();
                        return 1;
                    }
                    i += 2;
                } else {
                    printf("Error: Missing optimization goal after -O\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-B") == 0) {
                // Buffer size
                if (i + 1 < argc) {
                    buffer_size = atoi(argv[i + 1]);
                    if (buffer_size < 1024) {
                        printf("Warning: Small buffer size may impact performance. Minimum 1024 recommended.\n");
                    }
                    printf("Buffer size set to: %zu bytes\n", buffer_size);
                    i += 2;
                } else {
                    printf("Error: Missing buffer size after -B\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-L") == 0) {
                // Enable large file mode
                large_file_mode = 1;
                printf("Large file mode enabled (chunk-based processing)\n");
                i++;
            } else if (strcmp(arg, "-P") == 0) {
                // Enable progressive format
                progressive_mode = 1;
                printf("Progressive format enabled (supports partial decompression)\n");
                algorithm_index = PROGRESSIVE; // Set algorithm to progressive
                i++;
            } else if (strcmp(arg, "-X") == 0) {
                // Enable split archive mode
                split_mode = 1;
                printf("Split archive mode enabled (creates multiple files)\n");
                i++;
            } else if (strcmp(arg, "-M") == 0) {
                // Set maximum part size for split archives
                if (i + 1 < argc) {
                    max_part_size = strtoull(argv[i + 1], NULL, 10);
                    if (max_part_size < MIN_SPLIT_SIZE) {
                        printf("Warning: Split size too small, using minimum %u bytes\n", MIN_SPLIT_SIZE);
                        max_part_size = MIN_SPLIT_SIZE;
                    }
                    printf("Maximum split archive part size set to: %llu bytes\n", 
                           (unsigned long long)max_part_size);
                    i += 2;
                } else {
                    printf("Error: Missing size value after -M\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-R") == 0) {
                // Range of blocks to decompress
                if (i + 1 < argc) {
                    // Parse range in format "start-end"
                    char* range = argv[i + 1];
                    char* hyphen = strchr(range, '-');
                    if (hyphen) {
                        *hyphen = '\0'; // Split the string at the hyphen
                        start_block = atoi(range);
                        end_block = atoi(hyphen + 1);
                        if (start_block > end_block) {
                            printf("Error: Invalid block range. Start must be <= end\n");
                            print_usage();
                            return 1;
                        }
                        range_specified = 1;
                        printf("Decompressing blocks %u to %u\n", start_block, end_block);
                    } else {
                        printf("Error: Invalid range format. Use 'start-end'\n");
                        print_usage();
                        return 1;
                    }
                    i += 2;
                } else {
                    printf("Error: Missing range after -R\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-S") == 0) {
                // Stream mode
                stream_mode = 1;
                printf("Stream mode enabled\n");
                i++;
            } else if (strcmp(arg, "-p") == 0) {
                // Profiling
                profiling_enabled = 1;
                printf("Profiling enabled\n");
                i++;
            } else if (strcmp(arg, "-k") == 0) {
                // Encryption key
                if (i + 1 < argc) {
                    // Just track that we received a key; we'll handle encryption differently
                    if (strcasecmp(argv[i + 1], "none") != 0) {
                        printf("Encryption key received\n");
                    }
                    i += 2;
                } else {
                    printf("Error: Missing key after -k option\n");
                    return 1;
                }
            } else if (strcmp(arg, "-D") == 0) {
                // Enable deduplication
                deduplication_enabled = 1;
                printf("Deduplication enabled (identify and eliminate redundant data)\n");
                i++;
            } else if (strcmp(arg, "-C") == 0) {
                // Chunk size for deduplication
                if (i + 1 < argc) {
                    dedup_chunk_size = atoi(argv[i + 1]);
                    if (dedup_chunk_size < MIN_DEDUP_CHUNK_SIZE) {
                        printf("Warning: Deduplication chunk size too small, using minimum size: %d bytes\n", MIN_DEDUP_CHUNK_SIZE);
                        dedup_chunk_size = MIN_DEDUP_CHUNK_SIZE;
                    } else if (dedup_chunk_size > MAX_DEDUP_CHUNK_SIZE) {
                        printf("Warning: Deduplication chunk size too large, using maximum size: %d bytes\n", MAX_DEDUP_CHUNK_SIZE);
                        dedup_chunk_size = MAX_DEDUP_CHUNK_SIZE;
                    }
                    printf("Deduplication chunk size set to: %zu bytes\n", dedup_chunk_size);
                    i += 2;
                } else {
                    printf("Error: Missing chunk size after -C\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-H") == 0) {
                // Hash algorithm for deduplication
                if (i + 1 < argc) {
                    int algorithm = atoi(argv[i + 1]);
                    if (algorithm >= 0 && algorithm <= 3) {
                        dedup_hash_algorithm = (DedupHashAlgorithm)algorithm;
                        
                        const char* hash_algorithm_names[] = {
                            "SHA1", "MD5", "CRC32", "XXH64"
                        };
                        
                        printf("Deduplication hash algorithm: %s\n", hash_algorithm_names[dedup_hash_algorithm]);
                    } else {
                        printf("Error: Invalid hash algorithm. Use 0 (SHA1), 1 (MD5), 2 (CRC32), or 3 (XXH64)\n");
                        print_usage();
                        return 1;
                    }
                    i += 2;
                } else {
                    printf("Error: Missing hash algorithm after -H\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-V") == 0) {
                // Deduplication mode
                if (i + 1 < argc) {
                    int mode = atoi(argv[i + 1]);
                    if (mode >= 0 && mode <= 2) {
                        dedup_mode = (DedupMode)mode;
                        
                        const char* dedup_mode_names[] = {
                            "Fixed-size chunking", "Variable-size chunking", "Smart chunking"
                        };
                        
                        printf("Deduplication mode: %s\n", dedup_mode_names[dedup_mode]);
                    } else {
                        printf("Error: Invalid deduplication mode. Use 0 (fixed), 1 (variable), or 2 (smart)\n");
                        print_usage();
                        return 1;
                    }
                    i += 2;
                } else {
                    printf("Error: Missing deduplication mode after -V\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "-d") == 0) {
                compress_mode = (strcmp(arg, "-c") == 0) ? 1 : 0;
                i++;
                
                // Check for algorithm index
                if (i < argc && argv[i][0] != '-' && isdigit(argv[i][0])) {
                    char *end;
                    long idx = strtol(argv[i], &end, 10);
                    
                    if (end != argv[i] && *end == '\0' && idx >= 0 && idx < get_algorithm_count()) {
                        // Valid algorithm index
                        algorithm_index = (int)idx;
                        i++; // Move past the algorithm index
                    }
                }
                
                // Get input file
                if (i < argc && argv[i][0] != '-') {
                    input_file = argv[i];
                    i++;
                }
            } else {
                printf("Error: Unknown option '%s'\n", arg);
                print_usage();
                return 1;
            }
        } else {
            // Not an option, assume it's a file
            if (input_file == NULL) {
                input_file = arg;
            } else if (output_file == NULL) {
                output_file = arg;
            } else {
                printf("Error: Too many arguments\n");
                print_usage();
                return 1;
            }
            i++;
        }
    }
    
    // Check if we have required arguments
    if (compress_mode == -1) {
        printf("Error: No operation (-c or -d) specified\n");
        print_usage();
        return 1;
    }
    
    if (!input_file) {
        printf("Error: No input file specified\n");
        print_usage();
        return 1;
    }
    
    // Validate progressive mode options
    if (range_specified && !progressive_mode) {
        printf("Error: Block range (-R) requires progressive format (-P)\n");
        return 1;
    }
    
    if (stream_mode && !progressive_mode) {
        printf("Error: Streaming mode (-S) requires progressive format (-P)\n");
        return 1;
    }
    
    // Auto-generate output file name if not provided
    char auto_output_file[1024] = {0};
    if (!output_file) {
        if (compress_mode == 1) {
            // For compression, append appropriate extension
            CompressionAlgorithm* algorithm = get_algorithm(algorithm_index);
            snprintf(auto_output_file, sizeof(auto_output_file), "%s%s", 
                    input_file, algorithm->extension);
            output_file = auto_output_file;
        } else {
            // For decompression, try to remove extension
            size_t len = strlen(input_file);
            CompressionAlgorithm* algorithm = NULL;
            int found_match = 0;
            
            // Find matching extension to remove
            for (int j = 0; j < get_algorithm_count(); j++) {
                algorithm = get_algorithm(j);
                size_t ext_len = strlen(algorithm->extension);
                
                if (len > ext_len && 
                    strcmp(input_file + len - ext_len, algorithm->extension) == 0) {
                    // Found a match
                    strncpy(auto_output_file, input_file, len - ext_len);
                    auto_output_file[len - ext_len] = '\0';
                    algorithm_index = j;  // Use the matched algorithm
                    found_match = 1;
                    break;
                }
            }
            
            if (!found_match) {
                // If no matching extension, append .decoded
                snprintf(auto_output_file, sizeof(auto_output_file), "%s.decoded", input_file);
            }
            
            output_file = auto_output_file;
        }
        
        printf("Auto-generated output file: %s\n", output_file);
    }
    
    // Initialize deduplication if enabled
    if (deduplication_enabled) {
        init_deduplication(dedup_chunk_size, dedup_hash_algorithm, dedup_mode);
    }
    
    // Execute the operation
    int result = 0;
    ProfileData profile;
    if (profiling_enabled) {
        start_profiling(&profile, compress_mode == 1 ? "-c" : "-d");
    }
    
    // Get algorithm if not in progressive mode
    CompressionAlgorithm* algorithm = NULL;
    if (!progressive_mode) {
        algorithm = get_algorithm(algorithm_index);
        if (!algorithm) {
            printf("Error: Invalid algorithm index %d\n", algorithm_index);
            return 1;
        }
    }
    
    // Perform the requested operation
    if (compress_mode == 1) {
        // Compression
        if (deduplication_enabled) {
            // Deduplicate and then compress
            result = deduplicate_file(input_file, output_file, algorithm_index, checksum_type);
            // Print deduplication statistics
            print_dedup_stats();
            cleanup_deduplication();
        } else if (split_mode) {
            // Split archive compression
            result = compress_to_split_archive(input_file, output_file, algorithm_index, max_part_size, checksum_type);
        } else if (progressive_mode) {
            // Progressive compression
            result = progressive_compress_file(input_file, output_file, checksum_type);
        } else if (large_file_mode) {
            // Large file compression
            result = compress_large_file(input_file, output_file, buffer_size);
        } else {
            // Regular compression
            printf("DEBUG: Using algorithm_index=%d, input=%s, output=%s\n", algorithm_index, input_file, output_file);
            result = compress_file_with_algorithm(input_file, output_file, algorithm_index, checksum_type);
            printf("DEBUG: compress_file_with_algorithm returned %d\n", result);
        }
    } else if (compress_mode == 0) {
        // Decompression
        if (split_mode) {
            // Split archive decompression
            result = decompress_from_split_archive(input_file, output_file, algorithm_index, checksum_type);
        } else if (progressive_mode && range_specified) {
            // Progressive partial decompression
            result = progressive_decompress_range(input_file, output_file, start_block, end_block);
        } else if (progressive_mode) {
            // Progressive full decompression
            result = progressive_decompress_file(input_file, output_file);
        } else if (large_file_mode) {
            // Large file decompression
            result = decompress_large_file(input_file, output_file, buffer_size);
        } else {
            // Regular decompression
            result = decompress_file_with_algorithm(input_file, output_file, algorithm_index, checksum_type);
        }
    }
    
    if (profiling_enabled) {
        end_profiling(&profile);
        print_profiling_results(&profile);
    }
    
    // Check the result value based on operation
    if (deduplication_enabled) {
        // For deduplication, 0 always means success
        if (result == 0) {
            printf("Operation completed successfully!\n");
            printf("Input file: %s\n", input_file);
            printf("Output file: %s\n", output_file);
            return 0;
        } else {
            printf("Operation failed\n");
            return 1;
        }
    } else if ((algorithm_index == 1 || algorithm_index == 3) && result == 0) {
        // For RLE and RLE-Parallel, 0 means success
        printf("Operation completed successfully!\n");
        printf("Input file: %s\n", input_file);
        printf("Output file: %s\n", output_file);
    } else if ((algorithm_index != 1 && algorithm_index != 3) && result != 0) {
        // For other algorithms, non-zero means success
        printf("Operation completed successfully!\n");
        printf("Input file: %s\n", input_file);
        printf("Output file: %s\n", output_file);
    } else {
        printf("Operation failed\n");
    }
    
    // Clean up
    if (output_file && output_file != input_file) {
        free(output_file);
    }
    
    // For RLE algorithms, return 0 (success) if result was 0
    // For other algorithms, return 0 (success) if result was non-zero
    if (deduplication_enabled) {
        return result ? 1 : 0; // For deduplication, return 0 if result is 0 (success)
    } else if ((algorithm_index == 1 || algorithm_index == 3) && result == 0) {
        return 0;
    }
    
    return result ? 0 : 1;
} 