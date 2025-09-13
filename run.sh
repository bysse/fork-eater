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
cd build
./fork-eater
