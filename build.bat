@echo off
setlocal enabledelayedexpansion

:: Compiler setup
set CC=gcc
set CFLAGS=-Wall -Wextra -g
set LIBS=-lm -lz
set OPENSSL_LIBS=-lssl -lcrypto

:: Release mode flag
set RELEASE=0
set OBJDIR=obj
set TARGET=filecompressor.exe
set DEBUGGER=

:: Process command-line arguments
:arg_loop
if "%1"=="" goto end_arg_loop
if /i "%1"=="clean" (
    echo Cleaning build files...
    if exist %TARGET% del %TARGET%
    if exist %OBJDIR% rmdir /s /q %OBJDIR%
    goto :EOF
)
if /i "%1"=="debug" (
    echo Debug build selected
    set CFLAGS=%CFLAGS% -g -DDEBUG
    set DEBUGGER=gdb
    shift
    goto arg_loop
)
if /i "%1"=="optimize" (
    echo Optimized build selected
    set CFLAGS=%CFLAGS% -O3 -march=native -DNDEBUG
    set RELEASE=1
    shift
    goto arg_loop
)
if /i "%1"=="run" (
    set RUN=1
    shift
    goto arg_loop
)
echo Unknown option: %1
shift
goto arg_loop
:end_arg_loop

:: Create obj directory if it doesn't exist
if not exist %OBJDIR% mkdir %OBJDIR%

:: Source files
set SOURCES=filecompressor.c huffman.c rle.c lz77.c parallel.c compression.c large_file_utils.c lz77_parallel.c encryption.c progressive.c split_archive.c deduplication.c

:: Handle release build
if %RELEASE%==1 (
    set CFLAGS=%CFLAGS% -O3 -march=native -DNDEBUG
) else (
    set CFLAGS=%CFLAGS% -g -DDEBUG
)

echo Building with: %CC% %CFLAGS%

:: Compile each source file
set OBJECTS=
for %%f in (%SOURCES%) do (
    echo Compiling %%f...
    %CC% %CFLAGS% -c %%f -o %OBJDIR%\%%~nf.o
    if errorlevel 1 (
        echo Build failed on %%f
        exit /b 1
    )
    set OBJECTS=!OBJECTS! %OBJDIR%\%%~nf.o
)

:: Link the executable
echo Linking %TARGET%...
%CC% %CFLAGS% %OBJECTS% -o %TARGET% %LIBS% %OPENSSL_LIBS%
if errorlevel 1 (
    echo Link failed
    exit /b 1
)

echo Build successful: %TARGET%

:: Run the executable if requested
if defined RUN (
    echo Running %TARGET%...
    %DEBUGGER% %TARGET%
)

endlocal 