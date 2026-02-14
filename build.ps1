# Limbo Engine - PowerShell build script (Clang + Ninja)

param(
    [switch]$Debug,
    [switch]$Release,
    [switch]$Clean,
    [switch]$Test,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Host "Limbo Engine Build Script"
    Write-Host ""
    Write-Host "Usage: .\build.ps1 [options]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Debug     Build in Debug mode"
    Write-Host "  -Release   Build in Release mode (default)"
    Write-Host "  -Clean     Clean build directory before building"
    Write-Host "  -Test      Run tests after building"
    Write-Host "  -Help      Show this help message"
    exit 0
}

$script:Config = if ($Debug) { "Debug" } else { "Release" }
$script:PresetName = if ($Debug) { "debug" } else { "release" }
$script:BuildDir = "build/$script:PresetName"

Write-Host "========================================"
Write-Host "  Limbo Engine Build"
Write-Host "  Config: $script:Config"
Write-Host "  Compiler: Clang + Ninja"
Write-Host "========================================"

# Check for required tools
$clang = Get-Command clang -ErrorAction SilentlyContinue
$ninja = Get-Command ninja -ErrorAction SilentlyContinue

if (-not $clang) {
    Write-Host "Error: clang not found in PATH" -ForegroundColor Red
    Write-Host "Install LLVM/Clang or add it to PATH" -ForegroundColor Yellow
    exit 1
}

if (-not $ninja) {
    Write-Host "Error: ninja not found in PATH" -ForegroundColor Red
    Write-Host "Install Ninja or add it to PATH" -ForegroundColor Yellow
    exit 1
}

Write-Host "Using Clang: $($clang.Source)"
Write-Host "Using Ninja: $($ninja.Source)"

# Clean if requested
if ($Clean) {
    Write-Host "[1/3] Cleaning build directory..."
    if (Test-Path $script:BuildDir) {
        Remove-Item -Recurse -Force $script:BuildDir
    }
}

# Configure
if (-not (Test-Path "$script:BuildDir\CMakeCache.txt")) {
    Write-Host "[1/3] Configuring CMake..."
    $cmakeArgs = @(
        "-B", $script:BuildDir,
        "-S", ".",
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=$($script:Config)",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DLIMBO_BUILD_TESTS=ON"
    )
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "[1/3] Build already configured (use -Clean to reconfigure)"
}

# Build
Write-Host "[2/3] Building..."
& cmake --build $script:BuildDir --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

# Test
if ($Test) {
    Write-Host "[3/3] Running tests..."
    & ctest --test-dir $script:BuildDir --output-on-failure
} else {
    Write-Host "[3/3] Skipping tests (use -Test to run)"
}

Write-Host ""
Write-Host "========================================"
Write-Host "  Build complete!"
Write-Host "  Binaries: $script:BuildDir\bin\"
Write-Host "========================================"
