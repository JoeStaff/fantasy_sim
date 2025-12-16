#!/bin/bash
# Setup environment script for Linux/Mac
# This script sets up vcpkg and installs dependencies

set -e  # Exit on error

echo "========================================"
echo "Fantasy Sim - Environment Setup"
echo "========================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VCPKG_DIR="$PROJECT_ROOT/vcpkg"

# Check if vcpkg directory exists
if [ -d "$VCPKG_DIR" ]; then
    echo "[INFO] vcpkg directory found at: $VCPKG_DIR"
else
    echo "[INFO] vcpkg not found. Cloning vcpkg..."
    cd "$PROJECT_ROOT"
    git clone https://github.com/Microsoft/vcpkg.git
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to clone vcpkg. Make sure git is installed."
        exit 1
    fi
    echo "[SUCCESS] vcpkg cloned successfully"
fi

# Check if vcpkg executable exists
VCPKG_EXE="$VCPKG_DIR/vcpkg"
if [ ! -f "$VCPKG_EXE" ]; then
    echo "[INFO] Bootstrapping vcpkg..."
    cd "$VCPKG_DIR"
    ./bootstrap-vcpkg.sh
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to bootstrap vcpkg."
        exit 1
    fi
    echo "[SUCCESS] vcpkg bootstrapped successfully"
else
    echo "[INFO] vcpkg already bootstrapped"
fi

# Integrate vcpkg with CMake (one-time setup)
echo "[INFO] Integrating vcpkg with CMake..."
cd "$VCPKG_DIR"
./vcpkg integrate install
if [ $? -ne 0 ]; then
    echo "[WARNING] vcpkg integration may have failed, but continuing..."
else
    echo "[SUCCESS] vcpkg integrated with CMake"
fi

# Detect platform for triplet
if [[ "$OSTYPE" == "darwin"* ]]; then
    TRIPLET="x64-osx"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    TRIPLET="x64-linux"
else
    TRIPLET="x64-linux"  # Default
    echo "[WARNING] Unknown OS type, using x64-linux triplet"
fi

# Install dependencies
echo ""
echo "[INFO] Installing dependencies (this may take a while)..."
echo "Installing packages from vcpkg.json: sdl2 sdl2-image sdl2-ttf nlohmann-json"
echo "Using manifest mode (packages defined in vcpkg.json)"
echo "Triplet: $TRIPLET"
cd "$PROJECT_ROOT"
"$VCPKG_DIR/vcpkg" install --triplet "$TRIPLET"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to install dependencies."
    exit 1
fi

echo ""
echo "[SUCCESS] Environment setup complete!"
echo ""
echo "vcpkg location: $VCPKG_DIR"
echo "vcpkg toolchain: $VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
echo ""
echo "You can now run build.sh to build the project."
echo ""



