#!/bin/bash
# Run script for Linux/Mac
# This script runs the built executable

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
EXE_PATH="$BUILD_DIR/bin/FantasySim"

# Check if executable exists in standard location
if [ ! -f "$EXE_PATH" ]; then
    EXE_PATH="$BUILD_DIR/FantasySim"
fi

# Check if executable exists
if [ ! -f "$EXE_PATH" ]; then
    echo "[ERROR] Executable not found."
    echo "[INFO] Expected locations:"
    echo "  $BUILD_DIR/bin/FantasySim"
    echo "  $BUILD_DIR/FantasySim"
    echo ""
    echo "[INFO] Please build the project first using build.sh"
    exit 1
fi

echo "[INFO] Running Fantasy Sim..."
echo "Executable: $EXE_PATH"
echo ""

cd "$PROJECT_ROOT"
"$EXE_PATH"



