# File Compression Utility

A high-performance C-based file compression tool supporting multiple algorithms, parallel processing, and encryption capabilities.

## Overview

This utility provides efficient file compression using several algorithms with a focus on performance and flexibility. It's designed to handle files of various sizes, from small text files to very large data files, with optimizations for different use cases.

## Features

- **Multiple compression algorithms:**
  - Huffman coding (optimal for text files)
  - Run-Length Encoding (RLE) (good for files with repeated sequences)
  - LZ77 compression (general purpose algorithm)
  - Parallel versions of each algorithm for multi-core systems
  - Encrypted compression with LZ77

- **Performance options:**
  - Large file support with chunk-based processing
  - Optimization modes (speed vs. compression ratio)
  - Multi-threaded compression/decompression
  - Configurable buffer sizes

- **Additional capabilities:**
  - Built-in profiling for performance analysis
  - File integrity verification
  - Progress reporting for large files

## Project Structure

The project consists of several modules:
- Core compression algorithms (`huffman.c`, `rle.c`, `lz77.c`)
- Parallel processing support (`parallel.c`, `lz77_parallel.c`)
- Large file handling (`large_file_utils.c`)
- Security features (`encryption.c`)
- Main interface (`filecompressor.c`)

## Installation

### Prerequisites

- C compiler (GCC, Clang, or MSVC)
- Make (optional, for Unix-based systems)
- At least 512MB RAM for normal operation, more for large files

### Building the Project

#### Using Make (recommended for Unix/Linux)

```bash
# Default build
make

# Build with debug symbols
make debug

# Build with maximum optimization
make optimize

# Clean build files
make clean
```

#### Using batch file (Windows)

```cmd
# Default build
build.bat

# Build with debug symbols
build.bat debug

# Build with maximum optimization
build.bat optimize

# Clean build files
build.bat clean
```

## Usage

### Quick Start (Windows)

The `compress.bat` script provides easy access to common operations:

```cmd
# Compress a file with Huffman coding
compress.bat compress huffman test.txt

# Decompress a file
compress.bat decompress huffman test.txt.huf output.txt

# Compress using parallel LZ77
compress.bat compress lz77-parallel largefile.txt

# Compress with encryption
compress.bat compress lz77-encrypted myfile.txt mypassword
```

### Command Line Interface

```
filecompressor [options] <input_file> [output_file]
```

#### Options

- `-c [algorithm]`  Compress using specified algorithm (0-6)
- `-d [algorithm]`  Decompress the input file
- `-a`              List available compression algorithms
- `-t [threads]`    Number of threads to use (default: auto)
- `-k [key]`        Encryption key for secure algorithms
- `-O [goal]`       Optimization goal: speed or size
- `-B [size]`       Buffer size in bytes (default: 8192)
- `-L`              Enable large file mode for files > available RAM
- `-p`              Enable profiling
- `-h`              Display help message

#### Algorithm Selection

Use the algorithm index or let the program deduce it from the file extension:

| Index | Algorithm | Best For | File Extension |
|-------|-----------|----------|----------------|
| 0 | Huffman | Text files | .huf |
| 1 | RLE | Repetitive data | .rle |
| 2 | Parallel Huffman | Large text files | .hufp |
| 3 | Parallel RLE | Large repetitive data | .rlep |
| 4 | LZ77 | General purpose | .lz77 |
| 5 | Parallel LZ77 | Large general files | .lz77p |
| 6 | Encrypted LZ77 | Secure data | .lz77e |

### Examples

```bash
# Compress text file with Huffman
filecompressor -c 0 document.txt

# Decompress Huffman file
filecompressor -d document.txt.huf

# Compress image with LZ77 optimized for better compression
filecompressor -c 4 -O size -B 16384 image.raw

# Process large video file with parallel processing
filecompressor -c 5 -L -B 1048576 -t 4 video.raw

# Secure compression with encryption
filecompressor -c 6 -k "my_secure_password" confidential.txt
```

## Performance Optimization

- For large files (>1GB), use `-L` to enable chunk-based processing
- For multi-core systems, use parallel algorithms (2, 3, or 5)
- Increase buffer size (`-B`) for better throughput on SSDs
- Use `-O speed` for faster compression, `-O size` for better compression ratio
- For maximum speed, use RLE with `-O speed`
- For best compression ratio, use Huffman with `-O size`

## Troubleshooting

- If you encounter "out of memory" errors, enable large file mode (`-L`)
- For corrupted output, ensure no partial writes occurred during compression
- If decompression fails, verify you're using the same algorithm as compression

## License

This project is available under the MIT License.