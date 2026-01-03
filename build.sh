#!/bin/bash
# Limbo Engine - Cross-platform build script (Linux/macOS)

set -e

BUILD_DIR="build"
BUILD_TYPE="Release"
CLEAN=false
RUN_TESTS=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        -j*)
            JOBS="${1#-j}"
            shift
            ;;
        --help)
            echo "Limbo Engine Build Script"
            echo ""
            echo "Usage: ./build.sh [options]"
            echo ""
            echo "Options:"
            echo "  --debug     Build in Debug mode"
            echo "  --release   Build in Release mode (default)"
            echo "  --clean     Clean build directory before building"
            echo "  --test      Run tests after building"
            echo "  -jN         Use N parallel jobs (default: auto-detect)"
            echo "  --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "========================================"
echo "  Limbo Engine Build"
echo "  Config: $BUILD_TYPE"
echo "  Jobs: $JOBS"
echo "========================================"

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "[1/3] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "[1/3] Configuring CMake..."
    cmake -B "$BUILD_DIR" -S . -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
else
    echo "[1/3] Build already configured (use --clean to reconfigure)"
fi

# Build
echo "[2/3] Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"

# Test
if [ "$RUN_TESTS" = true ]; then
    echo "[3/3] Running tests..."
    ctest --test-dir "$BUILD_DIR" --build-config "$BUILD_TYPE" --output-on-failure
else
    echo "[3/3] Skipping tests (use --test to run)"
fi

echo ""
echo "========================================"
echo "  Build complete!"
echo "  Binary: $BUILD_DIR/bin/sandbox"
echo "========================================"
