#!/bin/bash

# Fork Eater Build Script
# This script automates the build process for the GLFW-based shader editor

set -e  # Exit on any error

PROJECT_DIR=$(dirname "$(realpath "$0")")
cd "$PROJECT_DIR"

echo "🚀 Building Fork Eater..."
echo "Project directory: $PROJECT_DIR"

# Check if external/imgui exists
if [ ! -d "external/imgui" ]; then
    echo "📦 Dear ImGui not found, cloning..."
    mkdir -p external
    cd external
    git clone https://github.com/ocornut/imgui.git
    cd ..
else
    echo "✅ Dear ImGui found"
fi

# Create build directory
echo "📁 Setting up build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring with CMake..."
cmake ..

# Build
echo "🔨 Building..."
make -j$(nproc)

echo "✅ Build complete!"
echo ""
echo "To run the shader editor:"
echo "  cd build && ./fork-eater"
echo ""
echo "Or from the project root:"
echo "  ./run.sh"