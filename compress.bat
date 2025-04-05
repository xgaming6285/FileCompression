@echo off
setlocal enabledelayedexpansion

REM Check if help is requested
if "%1"=="help" goto help
if "%1"=="--help" goto help
if "%1"=="-h" goto help

REM Check for minimum arguments
if "%1"=="" goto help

set MODE=%1
set ALGO_NAME=%2
set INPUT=%3
set OUTPUT=%4
set OPTIONS=

REM Process compression mode
if /i "%MODE%"=="compress" (
    set CMD=-c
) else if /i "%MODE%"=="decompress" (
    set CMD=-d
) else (
    echo Unknown mode: %MODE%
    goto help
)

REM Check for required arguments
if "%ALGO_NAME%"=="" (
    echo Missing algorithm name
    goto help
)

if "%INPUT%"=="" (
    echo Missing input file
    goto help
)

REM Map algorithm name to index
set ALGO=
if /i "%ALGO_NAME%"=="huffman" set ALGO=0
if /i "%ALGO_NAME%"=="rle" set ALGO=1
if /i "%ALGO_NAME%"=="huffman-parallel" set ALGO=2
if /i "%ALGO_NAME%"=="rle-parallel" set ALGO=3
if /i "%ALGO_NAME%"=="lz77" set ALGO=4
if /i "%ALGO_NAME%"=="lz77-parallel" set ALGO=5
if /i "%ALGO_NAME%"=="lz77-encrypted" set ALGO=6

REM Check if algorithm is valid
if "%ALGO%"=="" (
    echo Unknown algorithm: %ALGO_NAME%
    goto help
)

REM Check if input file exists
if not exist "%INPUT%" (
    echo Input file not found: %INPUT%
    goto end
)

REM Add threads if parallel algorithm is used
if "%ALGO%"=="2" set OPTIONS=%OPTIONS% -t 0
if "%ALGO%"=="3" set OPTIONS=%OPTIONS% -t 0
if "%ALGO%"=="5" set OPTIONS=%OPTIONS% -t 0

REM Run the command
echo Running: filecompressor.exe %CMD% %INPUT% %ALGO% %OPTIONS% %OUTPUT%
if "%OUTPUT%"=="" (
    filecompressor.exe %CMD% %INPUT% %ALGO% %OPTIONS%
) else (
    filecompressor.exe %CMD% %INPUT% %ALGO% %OUTPUT% %OPTIONS%
)
goto end

:help
echo.
echo File Compression Helper Script
echo -----------------------------
echo.
echo Usage: compress.bat [mode] [algorithm] [input_file] [output_file] 
echo.
echo Modes:
echo   compress     Compress the input file
echo   decompress   Decompress the input file
echo.
echo Algorithms:
echo   huffman            Huffman coding (good compression ratio)
echo   rle                Run-Length Encoding (fast for repetitive data)
echo   huffman-parallel   Parallel Huffman coding with multiple threads
echo   rle-parallel       Parallel RLE with multiple threads
echo   lz77              Lempel-Ziv 77 (excellent compression ratio)
echo   lz77-parallel     Parallel LZ77 with multiple threads
echo   lz77-encrypted    LZ77 with encryption
echo.
echo Examples:
echo   compress.bat compress huffman test.txt
echo   compress.bat decompress huffman test.txt.huf test_out.txt
echo   compress.bat compress lz77-parallel largefile.txt
echo.

:end
endlocal 