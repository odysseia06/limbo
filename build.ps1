# Limbo Engine - PowerShell build script

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

$BuildDir = "build"
$BuildType = if ($Debug) { "Debug" } else { "Release" }

Write-Host "========================================"
Write-Host "  Limbo Engine Build"
Write-Host "  Config: $BuildType"
Write-Host "========================================"

# Find Visual Studio installation
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Host "Error: Could not find vswhere.exe. Is Visual Studio installed?" -ForegroundColor Red
    exit 1
}

$vsPath = & $vsWhere -latest -property installationPath
if (-not $vsPath) {
    Write-Host "Error: Could not find Visual Studio installation." -ForegroundColor Red
    exit 1
}

# Import Visual Studio environment
$vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvarsPath)) {
    Write-Host "Error: Could not find vcvars64.bat at $vcvarsPath" -ForegroundColor Red
    exit 1
}

Write-Host "Using Visual Studio: $vsPath"

# Run vcvars64.bat and capture environment
$envCmd = "`"$vcvarsPath`" && set"
$envOutput = cmd /c $envCmd 2>&1
foreach ($line in $envOutput) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# Clean if requested
if ($Clean) {
    Write-Host "[1/3] Cleaning build directory..."
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
}

# Configure
if (-not (Test-Path "$BuildDir\CMakeCache.txt")) {
    Write-Host "[1/3] Configuring CMake..."
    & cmake -B $BuildDir -S . -DLIMBO_BUILD_TESTS=ON
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "[1/3] Build already configured (use -Clean to reconfigure)"
}

# Build
Write-Host "[2/3] Building..."
& cmake --build $BuildDir --config $BuildType --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

# Test
if ($Test) {
    Write-Host "[3/3] Running tests..."
    & ctest --test-dir $BuildDir --build-config $BuildType --output-on-failure
} else {
    Write-Host "[3/3] Skipping tests (use -Test to run)"
}

Write-Host ""
Write-Host "========================================"
Write-Host "  Build complete!"
Write-Host "  Binaries: $BuildDir\bin\$BuildType\"
Write-Host "========================================"
