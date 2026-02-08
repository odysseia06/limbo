#!/usr/bin/env bash
# Limbo Engine - compatibility wrapper around CMake presets

set -euo pipefail

preset="release"
clean=false
run_tests=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            preset="debug"
            shift
            ;;
        --release)
            preset="release"
            shift
            ;;
        --asan)
            preset="asan"
            shift
            ;;
        --ubsan)
            preset="ubsan"
            shift
            ;;
        --clean)
            clean=true
            shift
            ;;
        --test)
            run_tests=true
            shift
            ;;
        --help)
            cat <<'USAGE'
Limbo Engine Build Script (Preset Wrapper)

Usage: ./build.sh [options]

Options:
  --debug     Use debug preset
  --release   Use release preset (default)
  --asan      Use address sanitizer preset
  --ubsan     Use undefined behavior sanitizer preset
  --clean     Remove build/<preset> before configuring
  --test      Run tests after build
  --help      Show this help message
USAGE
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

if [[ "$clean" == true ]]; then
    rm -rf "build/${preset}"
fi

cmake --preset "$preset"
cmake --build --preset "$preset" --parallel

if [[ "$run_tests" == true ]]; then
    ctest --preset "$preset"
fi
