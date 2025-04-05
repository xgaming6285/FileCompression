# File Compression Utility

A C-based file compression utility that supports multiple compression algorithms, parallel processing, and encryption.

## Features

- Multiple compression algorithms:
  - Huffman coding
  - Run-Length Encoding (RLE)
  - LZ77 compression
  - Parallel versions of each algorithm
  - Encrypted compression (LZ77)
- Large file support with chunk-based processing
- Performance optimization options (speed vs. size)
- Multi-threaded compression/decompression
- Profiling capabilities

## Building the Project

### Using Make (recommended if installed)

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

### Using batch file (Windows)

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

### Using the helper script (Windows)

The `compress.bat` script simplifies common operations:

```cmd
# Compress a file with Huffman coding
compress.bat compress huffman test.txt

# Decompress a file
compress.bat decompress huffman test.txt.huf output.txt

# Compress using parallel LZ77
compress.bat compress lz77-parallel largefile.txt
```

### Direct command line usage

```
filecompressor [options] <input_file> [output_file]
```

#### Options

- `-c [algorithm]`  Compress the input file
- `-d [algorithm]`  Decompress the input file
- `-a`              List available compression algorithms
- `-t [threads]`    Number of threads to use (default: auto)
- `-k [key]`        Encryption key for encrypted algorithms
- `-O [goal]`       Optimization goal: speed or size
- `-B [size]`       Buffer size in bytes (default: 8192)
- `-L`              Enable large file mode for files larger than available RAM
- `-p`              Enable profiling
- `-h`              Display help message

#### Examples

```
# Compress with Huffman
filecompressor -c 0 input.txt

# Decompress Huffman file
filecompressor -d input.txt.huf

# Compress with LZ77 optimized for speed
filecompressor -c 4 -O speed input.txt

# Compress with LZ77 optimized for size
filecompressor -c 4 -O size -B 16384 input.txt

# Compress large file with 1MB chunks
filecompressor -c 0 -L -B 1048576 largefile.txt
```

## Algorithm Selection

Use the algorithm index or let the program deduce it from the file extension:

0. Huffman
1. RLE
2. Parallel Huffman
3. Parallel RLE
4. LZ77
5. Parallel LZ77
6. Encrypted LZ77

## Performance Tips

- For large files, use `-L` to enable chunk-based processing
- For multi-core systems, use parallel algorithms (2, 3, or 5)
- Increase buffer size (`-B`) for better throughput on fast storage
- Use `-O speed` for faster compression, `-O size` for better compression ratio