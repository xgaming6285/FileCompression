/**
 * File Compression Utility
 * Supports multiple compression algorithms
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filecompressor.h"
#include "compression.h"
#include "parallel.h"
#include "encryption.h"

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
    printf("  -p              Enable profiling\n");
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

int main(int argc, char *argv[]) {
    // Initialize compression algorithms
    init_compression_algorithms();
    
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Process command-line arguments
    int i = 1;
    int threads_set = 0;
    char *option = NULL;
    char *input_file = NULL;
    char *output_file = NULL;
    int algorithm_idx = 0; // Default to Huffman
    int profiling_enabled = 0;
    int large_file_mode = 0;  // Add large file mode flag
    
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
                    threads_set = 1;
                    i += 2;
                } else {
                    printf("Error: Missing thread count after -t\n");
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
            } else if (strcmp(arg, "-p") == 0) {
                // Enable profiling
                profiling_enabled = 1;
                printf("Profiling enabled\n");
                i++;
            } else if (strcmp(arg, "-k") == 0) {
                // Encryption key
                if (i + 1 < argc) {
                    set_encryption_key(argv[i + 1]);
                    i += 2;
                } else {
                    printf("Error: Missing encryption key after -k\n");
                    print_usage();
                    return 1;
                }
            } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "-d") == 0) {
                option = arg;
                i++;
                
                // Get input file
                if (i < argc) {
                    input_file = argv[i];
                    i++;
                    
                    // Check for algorithm index
                    if (i < argc) {
                        char *end;
                        long idx = strtol(argv[i], &end, 10);
                        
                        if (end != argv[i] && *end == '\0' && idx >= 0 && idx < get_algorithm_count()) {
                            // Valid algorithm index
                            algorithm_idx = (int)idx;
                            i++;
                            
                            // Check for output file
                            if (i < argc && argv[i][0] != '-') {
                                output_file = argv[i];
                                i++;
                            }
                        } else if (argv[i][0] != '-') {
                            // Not an algorithm index, must be output file
                            output_file = argv[i];
                            i++;
                        }
                    }
                } else {
                    printf("Error: Missing input file\n");
                    print_usage();
                    return 1;
                }
            } else {
                printf("Unknown option: %s\n", arg);
                print_usage();
                return 1;
            }
        } else {
            // Not an option, should be input file
            if (!input_file) {
                input_file = arg;
                i++;
            } else if (!output_file) {
                output_file = arg;
                i++;
            } else {
                printf("Error: Too many arguments\n");
                print_usage();
                return 1;
            }
        }
    }
    
    // Check if we have a valid option and input file
    if (!option || !input_file) {
        printf("Error: Missing required arguments\n");
        print_usage();
        return 1;
    }
    
    // Get the selected algorithm
    CompressionAlgorithm *algorithm = get_algorithm(algorithm_idx);
    if (!algorithm) {
        printf("Invalid algorithm index. Use -a to see available algorithms.\n");
        return 1;
    }
    
    // Create default output filename if not provided
    if (!output_file) {
        output_file = malloc(strlen(input_file) + 10); // Extra space for extension
        if (!output_file) {
            printf("Memory allocation error\n");
            return 1;
        }
        
        if (strcmp(option, "-c") == 0) {
            sprintf(output_file, "%s%s", input_file, algorithm->extension);
        } else if (strcmp(option, "-d") == 0) {
            // Check if input file has the algorithm's extension
            size_t input_len = strlen(input_file);
            size_t ext_len = strlen(algorithm->extension);
            
            if (input_len > ext_len && 
                strcmp(input_file + input_len - ext_len, algorithm->extension) == 0) {
                // Remove extension
                strncpy(output_file, input_file, input_len - ext_len);
                output_file[input_len - ext_len] = '\0';
            } else {
                sprintf(output_file, "%s.decoded", input_file);
            }
        }
    }
    
    int thread_count = get_thread_count();
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Algorithm: %s\n", algorithm->name);
    if (strstr(algorithm->name, "Parallel")) {
        printf("Using %d threads\n", thread_count);
    }
    
    // Set up profiling if enabled
    ProfileData profile;
    if (profiling_enabled) {
        char operation[256];
        snprintf(operation, sizeof(operation), "%s %s", 
                 strcmp(option, "-c") == 0 ? "Compressing" : "Decompressing",
                 algorithm->name);
        start_profiling(&profile, operation);
    }
    
    int result = 1; // Default to error
    
    // Check file size to automatically enable large file mode if necessary
    if (!large_file_mode) {
        FILE *test_file = fopen(input_file, "rb");
        if (test_file) {
            if (fseek(test_file, 0, SEEK_END) == 0) {
                long file_size = ftell(test_file);
                // If file is larger than 100MB, suggest large file mode
                if (file_size > 100 * 1024 * 1024) {
                    printf("Notice: Input file is large (%ld MB). Consider using -L for large file mode.\n", 
                           file_size / (1024 * 1024));
                }
            }
            fclose(test_file);
        }
    }
    
    if (strcmp(option, "-c") == 0) {
        printf("Compressing file...\n");
        
        if (large_file_mode) {
            // For large file compression, we currently only support Huffman
            if (strcmp(algorithm->name, "Huffman") == 0) {
                result = compress_large_file(input_file, output_file, buffer_size);
            } else {
                printf("Warning: Large file mode is currently only supported for Huffman compression. Using standard mode.\n");
                result = algorithm->compress(input_file, output_file);
            }
        } else {
            result = algorithm->compress(input_file, output_file);
        }
        
        if (result == 0) {
            printf("File compressed successfully.\n");
            
            // Print file size details
            FILE *in = fopen(input_file, "rb");
            FILE *out = fopen(output_file, "rb");
            
            if (in && out) {
                fseek(in, 0, SEEK_END);
                fseek(out, 0, SEEK_END);
                long in_size = ftell(in);
                long out_size = ftell(out);
                
                printf("Original size: %ld bytes\n", in_size);
                printf("Compressed size: %ld bytes\n", out_size);
                printf("Compression ratio: %.2f%%\n", 
                       (1.0 - ((double)out_size / in_size)) * 100);
                
                fclose(in);
                fclose(out);
            }
        } else {
            printf("Error compressing file.\n");
        }
    } else if (strcmp(option, "-d") == 0) {
        printf("Decompressing file...\n");
        
        if (large_file_mode) {
            // For large file decompression, we currently only support Huffman
            if (strcmp(algorithm->name, "Huffman") == 0) {
                result = decompress_large_file(input_file, output_file, buffer_size);
            } else {
                printf("Warning: Large file mode is currently only supported for Huffman decompression. Using standard mode.\n");
                result = algorithm->decompress(input_file, output_file);
            }
        } else {
            result = algorithm->decompress(input_file, output_file);
        }
        
        if (result == 0) {
            printf("File decompressed successfully.\n");
        } else {
            printf("Error decompressing file.\n");
        }
    } else {
        printf("Invalid option: %s\n", option);
        print_usage();
    }
    
    // End profiling if enabled
    if (profiling_enabled) {
        end_profiling(&profile);
        print_profiling_results(&profile);
    }
    
    // Free output filename if it was allocated
    if (!output_file_provided(argc, argv, option)) {
        free(output_file);
    }
    
    return result;
} 