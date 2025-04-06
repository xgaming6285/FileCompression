/**
 * Huffman Coding Implementation
 * Implementation file with compression and decompression functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "filecompressor.h" // For optimization settings

// Initialize global parameters with default values
int MAX_TREE_DEPTH = DEFAULT_MAX_TREE_DEPTH;

// Set Huffman parameters based on optimization goal
void set_huffman_optimization(int optimization_goal) {
    switch(optimization_goal) {
        case OPT_SPEED:
            printf("Optimizing Huffman for speed\n");
            MAX_TREE_DEPTH = SPEED_MAX_TREE_DEPTH;
            break;
        case OPT_SIZE:
            printf("Optimizing Huffman for size\n");
            MAX_TREE_DEPTH = SIZE_MAX_TREE_DEPTH;
            break;
        default:
            // Use defaults
            MAX_TREE_DEPTH = DEFAULT_MAX_TREE_DEPTH;
            break;
    }
}

// Create a new node with given character and frequency
Node* create_node(uint8_t character, unsigned frequency) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        printf("Memory allocation error\n");
        exit(1);
    }
    
    node->character = character;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    node->code_len = 0;
    
    return node;
}

// Create a min heap with given capacity
MinHeap* create_min_heap(unsigned capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!minHeap) {
        printf("Memory allocation error\n");
        exit(1);
    }
    
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (Node**)malloc(capacity * sizeof(Node*));
    
    if (!minHeap->array) {
        printf("Memory allocation error\n");
        exit(1);
    }
    
    return minHeap;
}

// Swap two nodes
void swap_nodes(Node** a, Node** b) {
    Node* temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify the min heap
void min_heapify(MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;
    
    if (left < (int)minHeap->size && 
        minHeap->array[left]->frequency < minHeap->array[smallest]->frequency) {
        smallest = left;
    }
    
    if (right < (int)minHeap->size && 
        minHeap->array[right]->frequency < minHeap->array[smallest]->frequency) {
        smallest = right;
    }
    
    if (smallest != idx) {
        swap_nodes(&minHeap->array[smallest], &minHeap->array[idx]);
        min_heapify(minHeap, smallest);
    }
}

// Check if size of heap is 1
int is_size_one(MinHeap* minHeap) {
    return (minHeap->size == 1);
}

// Extract the minimum value node from min heap
Node* extract_min(MinHeap* minHeap) {
    Node* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    min_heapify(minHeap, 0);
    return temp;
}

// Insert a new node to min heap
void insert_min_heap(MinHeap* minHeap, Node* node) {
    ++minHeap->size;
    int i = minHeap->size - 1;
    
    while (i && node->frequency < minHeap->array[(i - 1) / 2]->frequency) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    
    minHeap->array[i] = node;
}

// Build min heap
void build_min_heap(MinHeap* minHeap) {
    int n = minHeap->size - 1;
    int i;
    
    for (i = (n - 1) / 2; i >= 0; --i)
        min_heapify(minHeap, i);
}

// Build Huffman tree
Node* build_huffman_tree(uint8_t data[], unsigned size) {
    Node *left, *right, *top;
    unsigned frequency[MAX_CHAR] = {0};
    
    // Calculate frequency of each character
    for (unsigned i = 0; i < size; ++i)
        ++frequency[data[i]];
    
    // Create a min heap for all characters with frequency > 0
    MinHeap* minHeap = create_min_heap(MAX_CHAR);
    
    // Add leaf nodes to the min heap
    for (int i = 0; i < MAX_CHAR; ++i) {
        if (frequency[i] > 0) {
            minHeap->array[minHeap->size] = create_node(i, frequency[i]);
            ++minHeap->size;
        }
    }
    
    build_min_heap(minHeap);
    
    // Build Huffman tree with nodes from min heap
    while (!is_size_one(minHeap)) {
        left = extract_min(minHeap);
        right = extract_min(minHeap);
        
        // Create a new internal node with frequency equal to the sum
        // of the two nodes and character value is '$' (placeholder)
        top = create_node('$', left->frequency + right->frequency);
        
        top->left = left;
        top->right = right;
        
        insert_min_heap(minHeap, top);
    }
    
    // The remaining node is the root node and the tree is complete
    return extract_min(minHeap);
}

// Traverse the Huffman tree and store codes in a table
void generate_codes(Node* root, uint8_t code[], int top, HuffmanCode codes[]) {
    // Assign 0 to left edge and 1 to right edge
    if (root->left) {
        code[top] = 0;
        
        // Optimization: Limit tree depth for speed vs size tradeoff
        if (top < MAX_TREE_DEPTH - 1) {
            generate_codes(root->left, code, top + 1, codes);
        } else {
            // If we've reached maximum depth, make this a leaf node
            for (int i = 0; i < top; ++i) {
                codes[root->character].code[i] = code[i];
            }
            codes[root->character].code_len = top;
        }
    }
    
    if (root->right) {
        code[top] = 1;
        
        // Optimization: Limit tree depth for speed vs size tradeoff
        if (top < MAX_TREE_DEPTH - 1) {
            generate_codes(root->right, code, top + 1, codes);
        } else {
            // If we've reached maximum depth, make this a leaf node
            for (int i = 0; i < top; ++i) {
                codes[root->character].code[i] = code[i];
            }
            codes[root->character].code_len = top;
        }
    }
    
    // If this is a leaf node, store the code
    if (!root->left && !root->right) {
        for (int i = 0; i < top; ++i) {
            codes[root->character].code[i] = code[i];
        }
        codes[root->character].code_len = top;
    }
}

// Free the Huffman tree
void free_huffman_tree(Node* node) {
    if (node) {
        free_huffman_tree(node->left);
        free_huffman_tree(node->right);
        free(node);
    }
}

// Write the Huffman tree structure to the output file
void write_tree(Node* root, FILE* output_file) {
    // Mark internal node
    if (root->left || root->right) {
        fputc(0, output_file);
        write_tree(root->left, output_file);
        write_tree(root->right, output_file);
    } 
    // Mark leaf node and write the character
    else {
        fputc(1, output_file);
        fputc(root->character, output_file);
    }
}

// Read the Huffman tree structure from the input file
Node* read_tree(FILE* input_file) {
    int flag = fgetc(input_file);
    
    // If internal node
    if (flag == 0) {
        Node* node = create_node('$', 0);
        node->left = read_tree(input_file);
        node->right = read_tree(input_file);
        return node;
    } 
    // If leaf node
    else {
        uint8_t character = fgetc(input_file);
        return create_node(character, 0);
    }
}

// Function to compress a file using Huffman coding
int compress_file(const char* input_file, const char* output_file) {
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Apply optimization settings
    set_huffman_optimization(get_optimization_goal());
    
    // Use buffered I/O with configurable buffer size
    size_t buffer_size = get_buffer_size();
    
    // Get file size
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    // Read file data with buffered I/O
    uint8_t* data = (uint8_t*)malloc(file_size);
    if (!data) {
        printf("Memory allocation error\n");
        fclose(in);
        return 1;
    }
    
    size_t bytes_read = 0;
    size_t total_read = 0;
    
    while ((bytes_read = fread(data + total_read, 1, 
           (file_size - total_read < buffer_size) ? file_size - total_read : buffer_size, 
           in)) > 0) {
        total_read += bytes_read;
    }
    
    if ((long)total_read != file_size) {
        printf("Error: Failed to read the entire file\n");
        free(data);
        fclose(in);
        return 1;
    }
    
    fclose(in);
    
    // Build Huffman tree
    Node* root = build_huffman_tree(data, file_size);
    
    // Generate Huffman codes for each character
    HuffmanCode codes[MAX_CHAR] = {{{0}, 0}};
    uint8_t code[MAX_CHAR];
    generate_codes(root, code, 0, codes);
    
    // Open output file
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        printf("Error creating output file: %s\n", output_file);
        free(data);
        free_huffman_tree(root);
        return 1;
    }
    
    // Write the original file size
    fwrite(&file_size, sizeof(long), 1, out);
    
    // Write the Huffman tree structure
    write_tree(root, out);
    
    // Compress and write the data
    int current_bit = 0;
    uint8_t current_byte = 0;
    
    for (long i = 0; i < file_size; i++) {
        uint8_t ch = data[i];
        
        for (int j = 0; j < codes[ch].code_len; j++) {
            // If the current bit is 1, set the bit in the current byte
            if (codes[ch].code[j]) {
                current_byte |= (1 << (7 - current_bit));
            }
            
            current_bit++;
            
            // If we've filled a byte, write it to the output file
            if (current_bit == 8) {
                fwrite(&current_byte, 1, 1, out);
                current_bit = 0;
                current_byte = 0;
            }
        }
    }
    
    // Write any remaining bits
    if (current_bit > 0) {
        fwrite(&current_byte, 1, 1, out);
    }
    
    fclose(out);
    free(data);
    free_huffman_tree(root);
    
    return 0;
}

// Function to decompress a file using Huffman coding
int decompress_file(const char* input_file, const char* output_file) {
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Apply optimization settings
    set_huffman_optimization(get_optimization_goal());
    size_t buffer_size = get_buffer_size();
    
    // Read the original file size
    long original_size;
    if (fread(&original_size, sizeof(long), 1, in) != 1) {
        printf("Error reading file header\n");
        fclose(in);
        return 1;
    }
    
    // Read the Huffman tree
    Node* root = read_tree(in);
    if (!root) {
        printf("Error reading Huffman tree\n");
        fclose(in);
        return 1;
    }
    
    // Open output file
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        printf("Error creating output file: %s\n", output_file);
        free_huffman_tree(root);
        fclose(in);
        return 1;
    }
    
    // Decompress the data
    long bytes_written = 0;
    Node* current = root;
    
    uint8_t buffer[buffer_size];
    size_t bytes_read = 0;
    uint8_t* output_buffer = (uint8_t*)malloc(buffer_size);
    size_t output_pos = 0;
    
    while (bytes_written < original_size && (bytes_read = fread(buffer, 1, buffer_size, in)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            uint8_t byte = buffer[i];
            
            for (int bit = 7; bit >= 0 && bytes_written < original_size; bit--) {
                // Traverse Huffman tree based on current bit
                if ((byte >> bit) & 1) {
                    current = current->right;
                } else {
                    current = current->left;
                }
                
                // If this is a leaf node (character node)
                if (!current->left && !current->right) {
                    output_buffer[output_pos++] = current->character;
                    bytes_written++;
                    current = root;
                    
                    // Write to output file when buffer is full
                    if (output_pos == buffer_size) {
                        fwrite(output_buffer, 1, output_pos, out);
                        output_pos = 0;
                    }
                }
            }
        }
    }
    
    // Write any remaining bytes
    if (output_pos > 0) {
        fwrite(output_buffer, 1, output_pos, out);
    }
    
    free(output_buffer);
    free_huffman_tree(root);
    fclose(in);
    fclose(out);
    
    return 0;
}

// Initialize a Huffman context for chunked processing
HuffmanContext* huffman_context_init() {
    HuffmanContext* context = (HuffmanContext*)malloc(sizeof(HuffmanContext));
    if (!context) {
        return NULL;
    }
    
    // Clear all data
    memset(context, 0, sizeof(HuffmanContext));
    
    // Initialize bit position for writing
    context->bit_pos = 0;
    context->current_byte = 0;
    
    return context;
}

// Free a Huffman context
void huffman_context_free(HuffmanContext* context) {
    if (!context) return;
    
    // Free the Huffman tree if it exists
    if (context->tree_root) {
        free_huffman_tree(context->tree_root);
    }
    
    free(context);
}

// Process a chunk of data for frequency counting
int huffman_count_frequencies(HuffmanContext* context, const uint8_t* data, size_t size) {
    if (!context || !data) {
        return -1;
    }
    
    // Count frequencies of each character in the chunk
    for (size_t i = 0; i < size; i++) {
        context->frequency[data[i]]++;
    }
    
    // Update total bytes processed
    context->total_bytes += size;
    
    return 0;
}

// Build the Huffman tree from the accumulated frequencies
int huffman_build_tree_and_codes(HuffmanContext* context) {
    if (!context) {
        return -1;
    }
    
    // Build the Huffman tree using the frequency table
    context->tree_root = build_huffman_tree_from_freq(context->frequency, MAX_CHAR);
    if (!context->tree_root) {
        return -1;
    }
    
    // Generate codes for each character
    uint8_t code[MAX_CHAR];
    generate_codes(context->tree_root, code, 0, context->codes);
    
    return 0;
}

// Helper function to write a bit to the output
static void write_bit(HuffmanContext* context, int bit, uint8_t* output, size_t* pos) {
    // Add the bit to the current byte
    if (bit) {
        context->current_byte |= (1 << context->bit_pos);
    }
    
    // Increment bit position
    context->bit_pos++;
    
    // If we've filled a byte, write it to the output
    if (context->bit_pos == 8) {
        output[(*pos)++] = context->current_byte;
        context->current_byte = 0;
        context->bit_pos = 0;
    }
}

// Compress a chunk of data using the prepared Huffman codes
int huffman_compress_chunk(HuffmanContext* context, 
                          const uint8_t* input_data, size_t input_size,
                          uint8_t* output_data, size_t* output_size) {
    if (!context || !input_data || !output_data || !output_size) {
        return -1;
    }
    
    size_t output_pos = 0;
    
    // Process each byte in the input
    for (size_t i = 0; i < input_size; i++) {
        uint8_t ch = input_data[i];
        HuffmanCode code = context->codes[ch];
        
        // Write each bit of the code to the output
        for (int j = 0; j < code.code_len; j++) {
            // Make sure we don't overflow the output buffer
            if (output_pos >= *output_size - 1 && context->bit_pos > 0) {
                return -1; // Not enough space in output buffer
            }
            
            // Write the bit
            int bit = (code.code[j / 8] >> (j % 8)) & 1;
            write_bit(context, bit, output_data, &output_pos);
        }
    }
    
    // Update output size
    *output_size = output_pos;
    
    return 0;
}

// Finalize compression (write any remaining bits)
int huffman_compression_finalize(HuffmanContext* context, 
                                uint8_t* output_data, size_t* output_size) {
    if (!context || !output_data || !output_size) {
        return -1;
    }
    
    size_t output_pos = 0;
    
    // If we have any bits left to write, pad with zeros and write the final byte
    if (context->bit_pos > 0) {
        output_data[output_pos++] = context->current_byte;
    }
    
    *output_size = output_pos;
    return 0;
}

// Decompress a chunk of data
int huffman_decompress_chunk(Node* root, 
                            const uint8_t* input_data, size_t input_size,
                            uint8_t* output_data, size_t* output_size,
                            uint8_t* bit_pos_ptr, Node** current_ptr) {
    if (!root || !input_data || !output_data || !output_size || 
        !bit_pos_ptr || !current_ptr) {
        return -1;
    }
    
    Node* current = *current_ptr;
    uint8_t bit_pos = *bit_pos_ptr;
    size_t output_pos = 0;
    
    // If we don't have a current node, start at the root
    if (!current) {
        current = root;
    }
    
    // Process each byte in the input
    for (size_t i = 0; i < input_size; i++) {
        uint8_t byte = input_data[i];
        
        // Process each bit in the byte
        while (bit_pos < 8) {
            // Extract the current bit
            int bit = (byte >> bit_pos) & 1;
            bit_pos++;
            
            // Navigate the Huffman tree
            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }
            
            // If we've reached a leaf node, output the character
            if (!current->left && !current->right) {
                // Check if we have space in the output buffer
                if (output_pos >= *output_size) {
                    // Save state and return
                    *bit_pos_ptr = bit_pos;
                    *current_ptr = root; // Start fresh at the root
                    *output_size = output_pos;
                    return -1; // Not enough space
                }
                
                // Output the character
                output_data[output_pos++] = current->character;
                
                // Return to the root for the next character
                current = root;
            }
        }
        
        // Reset bit position for the next byte
        bit_pos = 0;
    }
    
    // Save state for the next chunk
    *bit_pos_ptr = bit_pos;
    *current_ptr = current;
    *output_size = output_pos;
    
    return 0;
}

// Build a Huffman tree from a frequency array
Node* build_huffman_tree_from_freq(unsigned long long freq[], unsigned size) {
    // Count non-zero frequencies
    unsigned count = 0;
    for (unsigned i = 0; i < size; i++) {
        if (freq[i] > 0) {
            count++;
        }
    }
    
    // Create a min heap with capacity equal to the number of characters
    MinHeap* minHeap = create_min_heap(count);
    
    // Add all characters with non-zero frequency to the min heap
    for (unsigned i = 0; i < size; i++) {
        if (freq[i] > 0) {
            insert_min_heap(minHeap, create_node(i, freq[i]));
        }
    }
    
    // Special case: only one character in input
    if (minHeap->size == 1) {
        Node* singleNode = extract_min(minHeap);
        Node* root = create_node('$', singleNode->frequency);
        root->left = singleNode;
        return root;
    }
    
    // Build the Huffman tree
    while (!is_size_one(minHeap)) {
        // Extract the two nodes with lowest frequency
        Node* left = extract_min(minHeap);
        Node* right = extract_min(minHeap);
        
        // Create a new internal node with these two nodes as children
        // and with frequency equal to the sum of the two nodes' frequencies
        unsigned sum = left->frequency + right->frequency;
        Node* parent = create_node('$', sum);
        parent->left = left;
        parent->right = right;
        
        // Add the new node to the min heap
        insert_min_heap(minHeap, parent);
    }
    
    // The remaining node is the root of the Huffman tree
    Node* root = extract_min(minHeap);
    
    // Free the min heap
    free(minHeap->array);
    free(minHeap);
    
    return root;
}

// Compress a large file using Huffman coding with chunked processing
int compress_large_file(const char* input_file, const char* output_file, size_t chunk_size) {
    if (chunk_size == 0) {
        chunk_size = DEFAULT_CHUNK_SIZE;
    }
    
    // Initialize large file readers and writers
    LargeFileReader* reader = large_file_reader_init(input_file, chunk_size);
    if (!reader) {
        return 1;
    }
    
    // Initialize Huffman context
    HuffmanContext* context = huffman_context_init();
    if (!context) {
        large_file_reader_free(reader);
        return 1;
    }
    
    // First pass: Count frequencies
    printf("Processing file in chunks to build frequency table...\n");
    uint8_t* chunk;
    size_t bytes_read;
    
    while ((chunk = large_file_reader_next_chunk(reader, &bytes_read)) != NULL) {
        huffman_count_frequencies(context, chunk, bytes_read);
    }
    
    // Build the Huffman tree and generate codes
    printf("Building Huffman tree and generating codes...\n");
    if (huffman_build_tree_and_codes(context) != 0) {
        printf("Error building Huffman tree\n");
        huffman_context_free(context);
        large_file_reader_free(reader);
        return 1;
    }
    
    // Reset the reader to start of file
    large_file_reader_reset(reader);
    
    // Open output file
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        printf("Error creating output file: %s\n", output_file);
        huffman_context_free(context);
        large_file_reader_free(reader);
        return 1;
    }
    
    // Write the original file size
    uint64_t file_size = reader->file_size;
    fwrite(&file_size, sizeof(uint64_t), 1, out);
    
    // Write the Huffman tree
    write_tree(context->tree_root, out);
    
    // Initialize a buffer for compressed output
    uint8_t* output_buffer = (uint8_t*)malloc(chunk_size);
    if (!output_buffer) {
        printf("Memory allocation error for output buffer\n");
        fclose(out);
        huffman_context_free(context);
        large_file_reader_free(reader);
        return 1;
    }
    
    // Second pass: Compress the file
    printf("Compressing file in chunks...\n");
    while ((chunk = large_file_reader_next_chunk(reader, &bytes_read)) != NULL) {
        size_t output_size = chunk_size;
        
        if (huffman_compress_chunk(context, chunk, bytes_read, output_buffer, &output_size) != 0) {
            printf("Error compressing chunk\n");
            free(output_buffer);
            fclose(out);
            huffman_context_free(context);
            large_file_reader_free(reader);
            return 1;
        }
        
        // Write compressed data to output file
        if (output_size > 0) {
            if (fwrite(output_buffer, 1, output_size, out) != output_size) {
                printf("Error writing compressed data\n");
                free(output_buffer);
                fclose(out);
                huffman_context_free(context);
                large_file_reader_free(reader);
                return 1;
            }
        }
    }
    
    // Finalize compression (write any remaining bits)
    size_t output_size = chunk_size;
    if (huffman_compression_finalize(context, output_buffer, &output_size) != 0) {
        printf("Error finalizing compression\n");
        free(output_buffer);
        fclose(out);
        huffman_context_free(context);
        large_file_reader_free(reader);
        return 1;
    }
    
    // Write final data if any
    if (output_size > 0) {
        if (fwrite(output_buffer, 1, output_size, out) != output_size) {
            printf("Error writing final data\n");
            free(output_buffer);
            fclose(out);
            huffman_context_free(context);
            large_file_reader_free(reader);
            return 1;
        }
    }
    
    // Clean up
    free(output_buffer);
    fclose(out);
    huffman_context_free(context);
    large_file_reader_free(reader);
    
    printf("Large file compression complete\n");
    return 0;
}

// Decompress a large file using Huffman coding with chunked processing
int decompress_large_file(const char* input_file, const char* output_file, size_t chunk_size) {
    if (chunk_size == 0) {
        chunk_size = DEFAULT_CHUNK_SIZE;
    }
    
    // Open input file
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        printf("Error opening input file: %s\n", input_file);
        return 1;
    }
    
    // Read the original file size
    uint64_t original_size;
    if (fread(&original_size, sizeof(uint64_t), 1, in) != 1) {
        printf("Error reading file header\n");
        fclose(in);
        return 1;
    }
    
    // Read the Huffman tree
    Node* root = read_tree(in);
    if (!root) {
        printf("Error reading Huffman tree\n");
        fclose(in);
        return 1;
    }
    
    // Get current position after reading header and tree
    long header_size = ftell(in);
    
    // Close and reopen file with large file reader starting at current position
    fclose(in);
    
    // Initialize large file reader and writer
    LargeFileReader* reader = large_file_reader_init(input_file, chunk_size);
    if (!reader) {
        free_huffman_tree(root);
        return 1;
    }
    
    // Seek past the header
    if (fseek(reader->file, header_size, SEEK_SET) != 0) {
        printf("Error seeking in input file\n");
        large_file_reader_free(reader);
        free_huffman_tree(root);
        return 1;
    }
    reader->current_position = header_size;
    
    // Initialize large file writer
    LargeFileWriter* writer = large_file_writer_init(output_file, chunk_size);
    if (!writer) {
        large_file_reader_free(reader);
        free_huffman_tree(root);
        return 1;
    }
    
    // Initialize buffers for decompression
    uint8_t* input_buffer;
    uint8_t* output_buffer = (uint8_t*)malloc(chunk_size);
    if (!output_buffer) {
        printf("Memory allocation error for output buffer\n");
        large_file_writer_free(writer);
        large_file_reader_free(reader);
        free_huffman_tree(root);
        return 1;
    }
    
    // Initialize decompression state
    uint8_t bit_pos = 0;
    Node* current = root;
    uint64_t total_written = 0;
    
    // Decompress the file in chunks
    printf("Decompressing file in chunks...\n");
    size_t bytes_read;
    while ((input_buffer = large_file_reader_next_chunk(reader, &bytes_read)) != NULL) {
        size_t output_size = chunk_size;
        
        if (huffman_decompress_chunk(root, input_buffer, bytes_read, 
                                    output_buffer, &output_size, 
                                    &bit_pos, &current) != 0) {
            // Buffer full, write it out and continue
            if (output_size > 0) {
                if (large_file_writer_write(writer, output_buffer, output_size) != 0) {
                    printf("Error writing decompressed data\n");
                    free(output_buffer);
                    large_file_writer_free(writer);
                    large_file_reader_free(reader);
                    free_huffman_tree(root);
                    return 1;
                }
                total_written += output_size;
            }
            
            // Continue with a fresh buffer
            output_size = chunk_size;
            if (huffman_decompress_chunk(root, input_buffer, bytes_read, 
                                        output_buffer, &output_size, 
                                        &bit_pos, &current) != 0) {
                printf("Error decompressing chunk\n");
                free(output_buffer);
                large_file_writer_free(writer);
                large_file_reader_free(reader);
                free_huffman_tree(root);
                return 1;
            }
        }
        
        // Write decompressed data
        if (output_size > 0) {
            if (large_file_writer_write(writer, output_buffer, output_size) != 0) {
                printf("Error writing decompressed data\n");
                free(output_buffer);
                large_file_writer_free(writer);
                large_file_reader_free(reader);
                free_huffman_tree(root);
                return 1;
            }
            total_written += output_size;
        }
        
        // Check if we've decompressed all bytes
        if (total_written >= original_size) {
            break;
        }
    }
    
    // Cleanup
    free(output_buffer);
    large_file_writer_free(writer);
    large_file_reader_free(reader);
    free_huffman_tree(root);
    
    // Verify decompressed size
    if (total_written != original_size) {
        printf("Warning: Decompressed size (%llu) doesn't match original size (%llu)\n",
               (unsigned long long)total_written, (unsigned long long)original_size);
    }
    
    printf("Large file decompression complete\n");
    return 0;
} 