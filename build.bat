@echo off
setlocal enabledelayedexpansion

:: Limbo Engine - Windows build script

set BUILD_DIR=build
set BUILD_TYPE=Release
set CLEAN=false
set RUN_TESTS=false

:: Parse arguments
:parse_args
if "%~1"=="" goto :done_parsing
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="--test" (
    set RUN_TESTS=true
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    echo Limbo Engine Build Script
    echo.
    echo Usage: build.bat [options]
    echo.
    echo Options:
    echo   --debug     Build in Debug mode
    echo   --release   Build in Release mode ^(default^)
    echo   --clean     Clean build directory before building
    echo   --test      Run tests after building
    echo   --help      Show this help message
    exit /b 0
)
echo Unknown option: %~1
echo Use --help for usage information
exit /b 1

:done_parsing

echo ========================================
echo   Limbo Engine Build
echo   Config: %BUILD_TYPE%
echo ========================================

:: Clean if requested
if "%CLEAN%"=="true" (
    echo [1/3] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

:: Configure
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [1/3] Configuring CMake...
    cmake -B "%BUILD_DIR%" -S .
) else (
    echo [1/3] Build already configured ^(use --clean to reconfigure^)
)

:: Build
echo [2/3] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

:: Test
if "%RUN_TESTS%"=="true" (
    echo [3/3] Running tests...
    ctest --test-dir "%BUILD_DIR%" --build-config %BUILD_TYPE% --output-on-failure
) else (
    echo [3/3] Skipping tests ^(use --test to run^)
)

echo.
echo ========================================
echo   Build complete!
echo   Binary: %BUILD_DIR%\bin\%BUILD_TYPE%\sandbox.exe
echo ========================================

endlocal
