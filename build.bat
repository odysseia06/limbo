@echo off
setlocal

rem Limbo Engine - compatibility wrapper around CMake presets

set PRESET=release
set CLEAN=false
set RUN_TESTS=false

:parse_args
if "%~1"=="" goto :done_parsing

if /i "%~1"=="--debug" (
    set PRESET=debug
    shift
    goto :parse_args
)
if /i "%~1"=="--release" (
    set PRESET=release
    shift
    goto :parse_args
)
if /i "%~1"=="--asan" (
    set PRESET=asan
    shift
    goto :parse_args
)
if /i "%~1"=="--ubsan" (
    set PRESET=ubsan
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
    echo Limbo Engine Build Script ^(Preset Wrapper^)
    echo.
    echo Usage: build.bat [options]
    echo.
    echo Options:
    echo   --debug     Use debug preset
    echo   --release   Use release preset ^(default^)
    echo   --asan      Use address sanitizer preset
    echo   --ubsan     Use undefined behavior sanitizer preset
    echo   --clean     Remove build\%%PRESET%% before configuring
    echo   --test      Run tests after build
    echo   --help      Show this help message
    exit /b 0
)

echo Unknown option: %~1
echo Use --help for usage information
exit /b 1

:done_parsing

if "%CLEAN%"=="true" (
    if exist "build\%PRESET%" rmdir /s /q "build\%PRESET%"
)

cmake --preset %PRESET%
if errorlevel 1 exit /b 1

cmake --build --preset %PRESET% --parallel
if errorlevel 1 exit /b 1

if "%RUN_TESTS%"=="true" (
    ctest --preset %PRESET%
    if errorlevel 1 exit /b 1
)

endlocal
