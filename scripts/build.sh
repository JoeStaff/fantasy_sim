#!/bin/bash
# Build script for Linux/Mac
# This script configures and builds the project using CMake

set -e  # Exit on error

echo "========================================"
echo "Fantasy Sim - Build Script"
echo "========================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VCPKG_DIR="$PROJECT_ROOT/vcpkg"
VCPKG_TOOLCHAIN="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"

# Check if vcpkg exists
if [ ! -d "$VCPKG_DIR" ]; then
    echo "[ERROR] vcpkg not found at: $VCPKG_DIR"
    echo "[INFO] Please run setup_env.sh first to set up the environment."
    exit 1
fi

# Check if vcpkg toolchain file exists
if [ ! -f "$VCPKG_TOOLCHAIN" ]; then
    echo "[ERROR] vcpkg toolchain file not found at: $VCPKG_TOOLCHAIN"
    echo "[INFO] Please run setup_env.sh first to set up the environment."
    exit 1
fi

# Check if vcpkg packages are installed (manifest mode creates vcpkg_installed directory)
VCPKG_INSTALLED="$PROJECT_ROOT/vcpkg_installed"
if [ ! -d "$VCPKG_INSTALLED" ]; then
    echo "[WARNING] vcpkg_installed directory not found."
    echo "[INFO] Packages may not be installed. Run setup_env.sh to install dependencies."
    echo "[INFO] Continuing anyway..."
fi

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake not found in PATH."
    echo "[INFO] Please install CMake and add it to your PATH."
    exit 1
fi

# Build directory
BUILD_DIR="$PROJECT_ROOT/build"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "[INFO] Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Detect platform for triplet
if [[ "$OSTYPE" == "darwin"* ]]; then
    TRIPLET="x64-osx"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TRIPLET="x64-linux"
else
    TRIPLET="x64-linux"  # Default
fi

# Configure CMake
echo "[INFO] Configuring CMake..."
echo "[INFO] Using vcpkg toolchain: $VCPKG_TOOLCHAIN"
echo "[INFO] Using triplet: $TRIPLET"
cd "$BUILD_DIR"
cmake .. -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET="$TRIPLET"
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed."
    echo "[INFO] Make sure vcpkg packages are installed by running setup_env.sh"
    exit 1
fi

# Build project
echo ""
echo "[INFO] Building project (Release configuration)..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed."
    exit 1
fi

echo ""
echo "[SUCCESS] Build complete!"
echo ""
echo "Executable location:"
if [ -f "$BUILD_DIR/bin/FantasySim" ]; then
    echo "  $BUILD_DIR/bin/FantasySim"
elif [ -f "$BUILD_DIR/FantasySim" ]; then
    echo "  $BUILD_DIR/FantasySim"
else
    echo "  (Check build directory for executable)"
fi
echo ""



