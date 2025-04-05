@echo off
setlocal

REM Set compiler options
set CC=gcc
set CFLAGS=-Wall -Wextra -O2 -pthread
set LDFLAGS=-pthread

REM Source files
set SOURCES=filecompressor.c compression.c huffman.c rle.c lz77.c encryption.c parallel.c lz77_parallel.c large_file_utils.c

REM Test files
set TEST_SOURCES=test_large_file.c

REM Handle arguments
if "%1"=="clean" goto clean
if "%1"=="debug" goto debug
if "%1"=="optimize" goto optimize
if "%1"=="help" goto help

:build
echo Building filecompressor...
%CC% %CFLAGS% -c %SOURCES%
%CC% %LDFLAGS% -o filecompressor.exe *.o
echo Building tests...
%CC% %CFLAGS% -c %TEST_SOURCES%
%CC% %LDFLAGS% -o test_large_file.exe test_large_file.o compression.o huffman.o rle.o lz77.o encryption.o parallel.o lz77_parallel.o large_file_utils.o
echo Build completed.
goto end

:clean
echo Cleaning build files...
del /Q *.o
del /Q *.exe
echo Clean completed.
goto end

:debug
echo Building with debug symbols...
set CFLAGS=%CFLAGS% -g -O0
goto build

:optimize
echo Building with maximum optimization...
set CFLAGS=%CFLAGS% -O3 -march=native
goto build

:help
echo Available commands:
echo   build.bat         - Build with normal optimization
echo   build.bat clean   - Remove compiled files
echo   build.bat debug   - Build with debug symbols
echo   build.bat optimize - Build with maximum optimization
echo   build.bat help    - Show this help message

:end
endlocal 