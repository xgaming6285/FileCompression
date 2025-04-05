/**
 * Huffman Coding Implementation
 * Header file defining data structures and function prototypes
 */
#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stdio.h>
#include "large_file_utils.h"

// Configurable parameters based on optimization goal
#define DEFAULT_MAX_TREE_DEPTH 256  // Default maximum tree depth

// Optimization values
#define SPEED_MAX_TREE_DEPTH 32    // Shallower tree for speed
#define SIZE_MAX_TREE_DEPTH 512    // Deeper tree for better compression

// Current parameter (will be set at runtime)
extern int MAX_TREE_DEPTH;

// Maximum number of characters (8-bit)
#define MAX_CHAR 256

// Node structure for Huffman tree
typedef struct Node {
    uint8_t character;
    unsigned frequency;
    struct Node *left, *right;
    int code_len;
} Node;

// Min heap structure for Huffman tree building
typedef struct MinHeap {
    unsigned size;
    unsigned capacity;
    Node** array;
} MinHeap;

// Huffman code for a character
typedef struct {
    uint8_t code[MAX_CHAR]; // Bit array
    int code_len;           // Length of the code
} HuffmanCode;

// Chunked processing context for Huffman compression
typedef struct {
    unsigned long long frequency[MAX_CHAR];  // Frequency count for each character
    HuffmanCode codes[MAX_CHAR];            // Huffman codes for each character
    Node* tree_root;                        // Root of the Huffman tree
    uint64_t total_bytes;                   // Total bytes processed
    uint8_t current_byte;                   // Current byte being written
    int bit_pos;                            // Current bit position in byte (0-7)
} HuffmanContext;

// Function prototypes
Node* create_node(uint8_t character, unsigned frequency);
MinHeap* create_min_heap(unsigned capacity);
void swap_nodes(Node** a, Node** b);
void min_heapify(MinHeap* minHeap, int idx);
int is_size_one(MinHeap* minHeap);
Node* extract_min(MinHeap* minHeap);
void insert_min_heap(MinHeap* minHeap, Node* node);
void build_min_heap(MinHeap* minHeap);
Node* build_huffman_tree(uint8_t data[], unsigned size);
Node* build_huffman_tree_from_freq(unsigned long long freq[], unsigned size);
void generate_codes(Node* root, uint8_t code[], int top, HuffmanCode codes[]);
void free_huffman_tree(Node* node);
void write_tree(Node* root, FILE* output_file);
Node* read_tree(FILE* input_file);

// Initialize a Huffman context for chunked processing
HuffmanContext* huffman_context_init();

// Free a Huffman context
void huffman_context_free(HuffmanContext* context);

// Process a chunk of data for frequency counting
int huffman_count_frequencies(HuffmanContext* context, const uint8_t* data, size_t size);

// Build the Huffman tree from the accumulated frequencies
int huffman_build_tree_and_codes(HuffmanContext* context);

// Compress a chunk of data using the prepared Huffman codes
int huffman_compress_chunk(HuffmanContext* context, 
                          const uint8_t* input_data, size_t input_size,
                          uint8_t* output_data, size_t* output_size);

// Finalize compression (write any remaining bits)
int huffman_compression_finalize(HuffmanContext* context, 
                                uint8_t* output_data, size_t* output_size);

// Decompress a chunk of data
int huffman_decompress_chunk(Node* root, 
                            const uint8_t* input_data, size_t input_size,
                            uint8_t* output_data, size_t* output_size,
                            uint8_t* bit_pos, Node** current);

// Traditional compression and decompression functions
int compress_file(const char* input_file, const char* output_file);
int decompress_file(const char* input_file, const char* output_file);

// Large file support compression and decompression
int compress_large_file(const char* input_file, const char* output_file, size_t chunk_size);
int decompress_large_file(const char* input_file, const char* output_file, size_t chunk_size);

// Set Huffman optimization parameters
void set_huffman_optimization(int optimization_goal);

#endif // HUFFMAN_H 