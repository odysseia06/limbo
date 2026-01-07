#!/bin/bash
# Limbo Engine - Build script (Clang + Ninja)

set -e

BUILD_DIR="build"
BUILD_TYPE="Release"
CLEAN=false
RUN_TESTS=false

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
echo "  Compiler: Clang + Ninja"
echo "========================================"

# Check for required tools
if ! command -v clang &> /dev/null; then
    echo "Error: clang not found in PATH"
    echo "Install clang: sudo apt install clang"
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo "Error: ninja not found in PATH"
    echo "Install ninja: sudo apt install ninja-build"
    exit 1
fi

echo "Using Clang: $(which clang)"
echo "Using Ninja: $(which ninja)"

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "[1/3] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "[1/3] Configuring CMake..."
    cmake -B "$BUILD_DIR" -S . -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DLIMBO_BUILD_TESTS=ON
else
    echo "[1/3] Build already configured (use --clean to reconfigure)"
fi

# Build
echo "[2/3] Building..."
cmake --build "$BUILD_DIR" --parallel

# Test
if [ "$RUN_TESTS" = true ]; then
    echo "[3/3] Running tests..."
    ctest --test-dir "$BUILD_DIR" --output-on-failure
else
    echo "[3/3] Skipping tests (use --test to run)"
fi

echo ""
echo "========================================"
echo "  Build complete!"
echo "  Binaries: $BUILD_DIR/bin/"
echo "========================================"
