#!/bin/bash

# Shader Editor Run Script
# This script launches the shader editor from the project root

PROJECT_DIR=$(dirname "$(realpath "$0")")
cd "$PROJECT_DIR"

if [ ! -f "build/fork-eater" ]; then
    echo "❌ fork-eater not found. Please build first with:"
    echo "   ./build.sh"
    exit 1
fi

echo "🎨 Launching Fork Eater..."
echo "Tip: Press ESC or use File->Exit to exit gracefully"

# Convert relative paths to absolute paths before changing directory
ARGS=()
for arg in "$@"; do
    if [[ -d "$arg" ]]; then
        # If argument is a directory, convert to absolute path
        ARGS+=("$(realpath "$arg")")
    else
        # Keep other arguments as-is
        ARGS+=("$arg")
    fi
done

cd build
./fork-eater "${ARGS[@]}"
