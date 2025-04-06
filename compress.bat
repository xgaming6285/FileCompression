@echo off
setlocal enabledelayedexpansion

REM Get the operation from the first argument
set OPERATION=%1

REM If no operation is provided, show usage
if "%OPERATION%"=="" (
    goto usage
)

REM Get the algorithm from the second argument
set ALGORITHM=%2

REM If no algorithm is provided, use default or show usage based on operation
if "%ALGORITHM%"=="" (
    if "%OPERATION%"=="list" (
        set ALGORITHM=none
    ) else (
        goto usage
    )
)

REM Get the input file from the third argument
set INPUT_FILE=%3

REM If no input file is provided (except for 'list' operation), show usage
if "%INPUT_FILE%"=="" (
    if not "%OPERATION%"=="list" (
        goto usage
    )
)

REM Get the output file or other parameter from the fourth argument
set OUTPUT_FILE=%4

REM Handle different operations
if /i "%OPERATION%"=="compress" goto compress
if /i "%OPERATION%"=="decompress" goto decompress
if /i "%OPERATION%"=="list" goto list
if /i "%OPERATION%"=="help" goto usage
if /i "%OPERATION%"=="deduplicate" goto deduplicate

echo Unknown operation: %OPERATION%
goto usage

:list
echo Available compression algorithms:
filecompressor -a
goto end

:compress
REM Map algorithm names to indices
set ALGO_INDEX=0

if /i "%ALGORITHM%"=="huffman" set ALGO_INDEX=0
if /i "%ALGORITHM%"=="rle" set ALGO_INDEX=1
if /i "%ALGORITHM%"=="huffman-parallel" set ALGO_INDEX=2
if /i "%ALGORITHM%"=="rle-parallel" set ALGO_INDEX=3
if /i "%ALGORITHM%"=="lz77" set ALGO_INDEX=4
if /i "%ALGORITHM%"=="lz77-parallel" set ALGO_INDEX=5
if /i "%ALGORITHM%"=="lz77-encrypted" set ALGO_INDEX=6
if /i "%ALGORITHM%"=="progressive" set ALGO_INDEX=7
if /i "%ALGORITHM%"=="split" goto split_compress

REM Check if encryption key is provided for encrypted algorithms
set ENCRYPTION_KEY=
if /i "%ALGORITHM%"=="lz77-encrypted" (
    set ENCRYPTION_KEY=%OUTPUT_FILE%
    set OUTPUT_FILE=
    
    REM Generate output file name if not provided
    if "%ENCRYPTION_KEY%"=="" (
        echo Error: Encryption key required for lz77-encrypted algorithm
        goto usage
    )
)

REM Generate output file name if not provided
if "%OUTPUT_FILE%"=="" (
    if /i "%ALGORITHM%"=="huffman" set OUTPUT_FILE=%INPUT_FILE%.huf
    if /i "%ALGORITHM%"=="rle" set OUTPUT_FILE=%INPUT_FILE%.rle
    if /i "%ALGORITHM%"=="huffman-parallel" set OUTPUT_FILE=%INPUT_FILE%.hufp
    if /i "%ALGORITHM%"=="rle-parallel" set OUTPUT_FILE=%INPUT_FILE%.rlep
    if /i "%ALGORITHM%"=="lz77" set OUTPUT_FILE=%INPUT_FILE%.lz77
    if /i "%ALGORITHM%"=="lz77-parallel" set OUTPUT_FILE=%INPUT_FILE%.lz77p
    if /i "%ALGORITHM%"=="lz77-encrypted" set OUTPUT_FILE=%INPUT_FILE%.lz77e
    if /i "%ALGORITHM%"=="progressive" set OUTPUT_FILE=%INPUT_FILE%.prog
)

echo Compressing %INPUT_FILE% using %ALGORITHM% algorithm...

REM Build command based on algorithm
set CMD=filecompressor -c %ALGO_INDEX% 

if /i "%ALGORITHM%"=="huffman-parallel" set CMD=%CMD% -t 0
if /i "%ALGORITHM%"=="rle-parallel" set CMD=%CMD% -t 0
if /i "%ALGORITHM%"=="lz77-parallel" set CMD=%CMD% -t 0
if /i "%ALGORITHM%"=="lz77-encrypted" set CMD=%CMD% -k "%ENCRYPTION_KEY%"
if /i "%ALGORITHM%"=="progressive" set CMD=%CMD% -P

REM Add input and output files
set CMD=%CMD% "%INPUT_FILE%" "%OUTPUT_FILE%"

echo Executing: %CMD%
%CMD%

if errorlevel 1 (
    echo Error: Compression failed.
) else (
    echo Compression complete. Output file: %OUTPUT_FILE%
)
goto end

:deduplicate
REM Deduplication operation
echo DEDUPLICATION OPERATION

REM Parse options for deduplication
set DEDUP_COMPRESSION=%4
set DEDUP_MODE=%5
set DEDUP_CHUNK_SIZE=%6
set DEDUP_HASH=%7

REM Set default compression algorithm if not specified
if "%DEDUP_COMPRESSION%"=="" set DEDUP_COMPRESSION=none

REM Map compression algorithm to index
set DEDUP_ALGO_INDEX=-1
if /i "%DEDUP_COMPRESSION%"=="none" set DEDUP_ALGO_INDEX=-1
if /i "%DEDUP_COMPRESSION%"=="huffman" set DEDUP_ALGO_INDEX=0
if /i "%DEDUP_COMPRESSION%"=="rle" set DEDUP_ALGO_INDEX=1
if /i "%DEDUP_COMPRESSION%"=="huffman-parallel" set DEDUP_ALGO_INDEX=2
if /i "%DEDUP_COMPRESSION%"=="rle-parallel" set DEDUP_ALGO_INDEX=3
if /i "%DEDUP_COMPRESSION%"=="lz77" set DEDUP_ALGO_INDEX=4
if /i "%DEDUP_COMPRESSION%"=="lz77-parallel" set DEDUP_ALGO_INDEX=5
if /i "%DEDUP_COMPRESSION%"=="lz77-encrypted" set DEDUP_ALGO_INDEX=6
if /i "%DEDUP_COMPRESSION%"=="progressive" set DEDUP_ALGO_INDEX=7

REM Set default output filename if not provided
set DEDUP_OUTPUT=%INPUT_FILE%.dedup
if "%DEDUP_COMPRESSION%"!="none" (
    if /i "%DEDUP_COMPRESSION%"=="huffman" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.huf
    if /i "%DEDUP_COMPRESSION%"=="rle" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.rle
    if /i "%DEDUP_COMPRESSION%"=="huffman-parallel" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.hufp
    if /i "%DEDUP_COMPRESSION%"=="rle-parallel" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.rlep
    if /i "%DEDUP_COMPRESSION%"=="lz77" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.lz77
    if /i "%DEDUP_COMPRESSION%"=="lz77-parallel" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.lz77p
    if /i "%DEDUP_COMPRESSION%"=="lz77-encrypted" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.lz77e
    if /i "%DEDUP_COMPRESSION%"=="progressive" set DEDUP_OUTPUT=%INPUT_FILE%.dedup.prog
)

REM Set default chunking mode if not specified
set DEDUP_MODE_INDEX=0
if "%DEDUP_MODE%"=="" set DEDUP_MODE=fixed
if /i "%DEDUP_MODE%"=="fixed" set DEDUP_MODE_INDEX=0
if /i "%DEDUP_MODE%"=="variable" set DEDUP_MODE_INDEX=1
if /i "%DEDUP_MODE%"=="smart" set DEDUP_MODE_INDEX=2

REM Set default chunk size if not specified
set DEDUP_CHUNK_SIZE_BYTES=65536
if not "%DEDUP_CHUNK_SIZE%"=="" (
    set DEDUP_CHUNK_SIZE_NUM=%DEDUP_CHUNK_SIZE:~0,-1%
    set DEDUP_CHUNK_SIZE_UNIT=%DEDUP_CHUNK_SIZE:~-1%
    
    set /a DEDUP_CHUNK_SIZE_BYTES=DEDUP_CHUNK_SIZE_NUM
    
    if "%DEDUP_CHUNK_SIZE_UNIT%"=="k" set /a DEDUP_CHUNK_SIZE_BYTES=DEDUP_CHUNK_SIZE_NUM*1024
    if "%DEDUP_CHUNK_SIZE_UNIT%"=="K" set /a DEDUP_CHUNK_SIZE_BYTES=DEDUP_CHUNK_SIZE_NUM*1024
    if "%DEDUP_CHUNK_SIZE_UNIT%"=="m" set /a DEDUP_CHUNK_SIZE_BYTES=DEDUP_CHUNK_SIZE_NUM*1024*1024
    if "%DEDUP_CHUNK_SIZE_UNIT%"=="M" set /a DEDUP_CHUNK_SIZE_BYTES=DEDUP_CHUNK_SIZE_NUM*1024*1024
)

REM Set default hash algorithm if not specified
set DEDUP_HASH_INDEX=0
if "%DEDUP_HASH%"=="" set DEDUP_HASH=sha1
if /i "%DEDUP_HASH%"=="sha1" set DEDUP_HASH_INDEX=0
if /i "%DEDUP_HASH%"=="md5" set DEDUP_HASH_INDEX=1
if /i "%DEDUP_HASH%"=="crc32" set DEDUP_HASH_INDEX=2
if /i "%DEDUP_HASH%"=="xxh64" set DEDUP_HASH_INDEX=3

echo Deduplicating %INPUT_FILE%...
echo Compression: %DEDUP_COMPRESSION% (index: %DEDUP_ALGO_INDEX%)
echo Chunk mode: %DEDUP_MODE% (index: %DEDUP_MODE_INDEX%)
echo Chunk size: %DEDUP_CHUNK_SIZE_BYTES% bytes
echo Hash algorithm: %DEDUP_HASH% (index: %DEDUP_HASH_INDEX%)
echo Output file: %DEDUP_OUTPUT%

REM Build the deduplication command
set CMD=filecompressor -c %DEDUP_ALGO_INDEX% -D -V %DEDUP_MODE_INDEX% -C %DEDUP_CHUNK_SIZE_BYTES% -H %DEDUP_HASH_INDEX% "%INPUT_FILE%" "%DEDUP_OUTPUT%"

echo Executing: %CMD%
%CMD%

if errorlevel 1 (
    echo Error: Deduplication failed.
) else (
    echo Deduplication complete. Output file: %DEDUP_OUTPUT%
)
goto end

:decompress
REM ... existing decompress section ...

:split_compress
REM ... existing split_compress section ...

:split_decompress
REM ... existing split_decompress section ...

:usage
echo.
echo File Compression Utility Helper Script
echo -----------------------------------
echo.
echo Usage: compress.bat OPERATION ALGORITHM INPUT_FILE [OUTPUT_FILE/PARAMS]
echo.
echo Operations:
echo   compress     - Compress a file
echo   decompress   - Decompress a file
echo   list         - List available algorithms
echo   deduplicate  - Deduplicate a file (identify and eliminate redundant data)
echo.
echo Compression Algorithms:
echo   huffman             - Huffman coding
echo   rle                 - Run-Length Encoding
echo   huffman-parallel    - Parallel Huffman coding
echo   rle-parallel        - Parallel Run-Length Encoding
echo   lz77                - LZ77 compression
echo   lz77-parallel       - Parallel LZ77 compression
echo   lz77-encrypted      - Encrypted LZ77 compression (requires key)
echo   progressive         - Progressive format
echo   split               - Split archive for large files
echo.
echo Deduplication Options (for 'deduplicate' operation):
echo   compress.bat deduplicate INPUT_FILE [COMPRESSION] [MODE] [CHUNK_SIZE] [HASH]
echo   - COMPRESSION: none, huffman, rle, lz77, etc. (default: none)
echo   - MODE: fixed, variable, smart (default: fixed)
echo   - CHUNK_SIZE: 4k, 8k, 16k, 32k, 64k, 128k, 256k, 512k, 1m (default: 64k)
echo   - HASH: sha1, md5, crc32, xxh64 (default: sha1)
echo.
echo Examples:
echo   compress.bat compress huffman myfile.txt
echo   compress.bat decompress huffman myfile.txt.huf output.txt
echo   compress.bat compress lz77-encrypted myfile.txt mypassword
echo   compress.bat deduplicate myarchive.zip none variable 128k sha1
echo.

:end
endlocal 