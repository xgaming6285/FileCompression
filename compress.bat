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
set PARAM5=%5
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
if /i "%ALGO_NAME%"=="progressive" (
    set ALGO=7
)
if /i "%ALGO_NAME%"=="split" (
    set ALGO=4
    set OPTIONS=%OPTIONS% -X
)

REM Check if algorithm is valid
if "%ALGO%"=="" if /i not "%ALGO_NAME%"=="progressive" if /i not "%ALGO_NAME%"=="split" (
    echo Unknown algorithm: %ALGO_NAME%
    goto help
)

REM Check if input file exists
if not exist "%INPUT%" (
    echo Input file not found: %INPUT%
    goto end
)

REM Add threads if parallel algorithm is used
if "%ALGO%"=="2" set OPTIONS=%OPTIONS% -t 4
if "%ALGO%"=="3" set OPTIONS=%OPTIONS% -t 4
if "%ALGO%"=="5" set OPTIONS=%OPTIONS% -t 4

REM Auto-generate output file if not provided
if "%OUTPUT%"=="" (
    if /i "%MODE%"=="compress" (
        if /i "%ALGO_NAME%"=="huffman" set OUTPUT=%INPUT%.huf
        if /i "%ALGO_NAME%"=="rle" set OUTPUT=%INPUT%.rle
        if /i "%ALGO_NAME%"=="huffman-parallel" set OUTPUT=%INPUT%.hufp
        if /i "%ALGO_NAME%"=="rle-parallel" set OUTPUT=%INPUT%.rlep
        if /i "%ALGO_NAME%"=="lz77" set OUTPUT=%INPUT%.lz77
        if /i "%ALGO_NAME%"=="lz77-parallel" set OUTPUT=%INPUT%.lz77p
        if /i "%ALGO_NAME%"=="lz77-encrypted" set OUTPUT=%INPUT%.lz77e
        if /i "%ALGO_NAME%"=="progressive" set OUTPUT=%INPUT%.prog
        if /i "%ALGO_NAME%"=="split" set OUTPUT=%INPUT%
    ) else (
        REM Get base name without extension for decompression
        for %%A in ("%INPUT%") do set BASENAME=%%~nA
        set OUTPUT=!BASENAME!_decompressed.txt
    )
    echo Auto-generated output file: !OUTPUT!
)

REM Check for special modes like progressive and split
if /i "%ALGO_NAME%"=="progressive" (
    REM If this is a range-based decompression for progressive format
    if /i "%MODE%"=="decompress" if not "%PARAM5%"=="" (
        set OPTIONS=%OPTIONS% -R %PARAM5%
        echo Using range: %PARAM5%
    )
)

if /i "%ALGO_NAME%"=="split" (
    REM Check if a custom part size was specified
    if not "%PARAM5%"=="" (
        set SIZE_PARAM=%PARAM5%
        set SIZE_NUMBER=
        set SIZE_UNIT=
        
        REM Extract the number and unit
        for /f "delims=0123456789" %%a in ("%SIZE_PARAM%") do set SIZE_UNIT=%%a
        for /f "delims=KMGkmg" %%a in ("%SIZE_PARAM%") do set SIZE_NUMBER=%%a
        
        if "!SIZE_UNIT!"=="K" set SIZE_UNIT=1024
        if "!SIZE_UNIT!"=="k" set SIZE_UNIT=1024
        if "!SIZE_UNIT!"=="M" set SIZE_UNIT=1048576
        if "!SIZE_UNIT!"=="m" set SIZE_UNIT=1048576
        if "!SIZE_UNIT!"=="G" set SIZE_UNIT=1073741824
        if "!SIZE_UNIT!"=="g" set SIZE_UNIT=1073741824
        
        REM Calculate size in bytes
        set /a PART_SIZE=!SIZE_NUMBER!*!SIZE_UNIT!
        
        REM If it's just a number, assume bytes
        if "!SIZE_UNIT!"=="" set PART_SIZE=!SIZE_NUMBER!
        
        set OPTIONS=%OPTIONS% -M !PART_SIZE!
        echo Setting split part size to !PART_SIZE! bytes
    )
)

REM Process checksum parameter
if not "%PARAM5%"=="" (
    if /i "%PARAM5%"=="crc32" (
        set OPTIONS=%OPTIONS% -I 1
        echo Adding CRC32 integrity verification
    ) else if /i "%PARAM5%"=="md5" (
        set OPTIONS=%OPTIONS% -I 2
        echo Adding MD5 integrity verification
    ) else if /i "%PARAM5%"=="sha256" (
        set OPTIONS=%OPTIONS% -I 3
        echo Adding SHA256 integrity verification
    )
)

REM Special case for progressive - run the command with -P first
if /i "%ALGO_NAME%"=="progressive" (
    echo Running: filecompressor.exe -P %CMD% %ALGO% %OPTIONS% "%INPUT%" "%OUTPUT%"
    filecompressor.exe -P %CMD% %ALGO% %OPTIONS% "%INPUT%" "%OUTPUT%"
) else (
    REM Run the command for other algorithms
    echo Running: filecompressor.exe %CMD% %ALGO% %OPTIONS% "%INPUT%" "%OUTPUT%"
    filecompressor.exe %CMD% %ALGO% %OPTIONS% "%INPUT%" "%OUTPUT%"
)

if %ERRORLEVEL% equ 0 (
    echo Operation completed successfully!
    echo Input file: %INPUT%
    echo Output file: %OUTPUT%
) else (
    echo Operation failed with error code %ERRORLEVEL%
)
goto end

:help
echo.
echo File Compression Helper Script
echo -----------------------------
echo.
echo Usage: compress.bat [mode] [algorithm] [input_file] [output_file] [options]
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
echo   progressive       Progressive format (enables partial decompression)
echo   split             Split archive format (divides into multiple files)
echo.
echo Options (5th parameter):
echo   crc32      CRC32 checksum for basic integrity verification
echo   md5        MD5 hash for stronger integrity verification
echo   sha256     SHA256 hash for maximum integrity verification
echo   [range]    For progressive decompress, specify block range (e.g., 0-5)
echo   [size]     For split mode, specify part size (e.g., 10M, 50M, 1G)
echo.
echo Examples:
echo   compress.bat compress huffman test.txt
echo   compress.bat decompress huffman test.txt.huf test_out.txt
echo   compress.bat compress lz77-parallel largefile.txt
echo   compress.bat compress huffman test.txt output.huf crc32
echo   compress.bat compress progressive test.txt
echo   compress.bat decompress progressive test.txt.prog test_out.txt 0-5
echo   compress.bat compress split largefile.txt output 100M
echo   compress.bat decompress split largefile.txt output.txt
echo.

:end
endlocal 