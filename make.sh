#!/bin/bash
# ============================================================================
# OpenCL-Benchmark v2.0 - Optimized Build Script
# ============================================================================

set -e  # Exit on error

echo "🔨 Building OpenCL-Benchmark v2.0..."
echo ""

# Detect number of CPU cores for parallel LTO
NCORES=$(nproc 2>/dev/null || echo 4)

# Compiler flags for maximum performance
# -flto=auto uses GNU make's job server or auto-detects CPU cores
CFLAGS="-std=c++17 -O3 -march=native -mtune=native -ffast-math -funroll-loops -flto=auto -DNDEBUG"
LIBS="-lOpenCL -Lsrc/OpenCL/lib -Isrc/OpenCL/include"

echo "🧵 Using $NCORES cores for parallel compilation..."

# Create bin directory if it doesn't exist
mkdir -p bin

# Compile the optimized version
echo "📦 Compiling with optimizations..."
g++ $CFLAGS src/main_clean.cpp src/kernel.cpp -o bin/OpenCL-Benchmark $LIBS

# Make executable
chmod +x bin/OpenCL-Benchmark

echo ""
echo "✅ BUILD SUCCESSFUL!"
echo ""
echo "🚀 Executable: bin/OpenCL-Benchmark"
echo ""
echo "📋 Usage:"
echo "  Interactive mode:  ./bin/OpenCL-Benchmark"
echo "  CLI mode:          ./bin/OpenCL-Benchmark [device_id]"
echo ""
echo "🎨 Features:"
echo "  • Interactive menu with arrow key navigation"
echo "  • 3DMark-style performance scoring"
echo "  • Real-time progress bars with visual feedback"
echo "  • Colored output with performance grades"
echo "  • JSON export for benchmark results"
echo ""
