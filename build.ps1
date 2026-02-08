# Limbo Engine - compatibility wrapper around CMake presets

param(
    [switch]$Debug,
    [switch]$Release,
    [switch]$Asan,
    [switch]$Ubsan,
    [switch]$Clean,
    [switch]$Test,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Host "Limbo Engine Build Script (Preset Wrapper)"
    Write-Host ""
    Write-Host "Usage: .\build.ps1 [options]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Debug      Use debug preset"
    Write-Host "  -Release    Use release preset (default)"
    Write-Host "  -Asan       Use address sanitizer preset"
    Write-Host "  -Ubsan      Use undefined behavior sanitizer preset"
    Write-Host "  -Clean      Remove build/<preset> before configuring"
    Write-Host "  -Test       Run tests after build"
    Write-Host "  -Help       Show this help message"
    exit 0
}

$preset = "release"
if ($Debug) { $preset = "debug" }
if ($Release) { $preset = "release" }
if ($Asan) { $preset = "asan" }
if ($Ubsan) { $preset = "ubsan" }

if ($Clean) {
    $buildDir = Join-Path "build" $preset
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
}

& cmake --preset $preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& cmake --build --preset $preset --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Test) {
    & ctest --preset $preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
