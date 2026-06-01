#!/bin/bash

# Exit immediately if any command fails
set -e

echo "======================================"
echo "  Building FakePs4 (Qt6 + CMake)      "
echo "======================================"

BUILD_DIR="build"
BIN_DIR="bin"
EXECUTABLE="$BIN_DIR/FakePs4"

# 1. Create the build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "📁 Creating build directory..."
    mkdir "$BUILD_DIR"
fi

# Enter the build directory
cd "$BUILD_DIR"

# 2. Run CMake (Forcing Release mode for maximum performance)
echo "⚙️  Configuring CMake (Release Mode)..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Compile the project
# Automatically detect the number of CPU cores to speed up the build
CORES=$(nproc 2>/dev/null || echo 4)
echo "🔨 Compiling using $CORES cores..."
make -j"$CORES"

# Go back to the project root
cd ..

echo "======================================"
echo "✅ Build Successful!                  "
echo "======================================"

# 4. Ask the user if they want to launch it immediately
read -p "🚀 Do you want to run FakePs4 now? (y/n) " -n 1 -r
echo    # Move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo "Starting application..."
    ./$EXECUTABLE
fi