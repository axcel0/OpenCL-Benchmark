#!/usr/bin/env bash
# command line argument(s): device ID(s); if empty, it will benchmark all available devices

mkdir -p bin # create directory for executable
rm -f bin/OpenCL-Benchmark # prevent execution of old version if compiling fails

# Enhanced compiler flags for maximum performance:
# -O3: Maximum optimization level
# -march=native: Optimize for current CPU architecture
# -mtune=native: Fine-tune for current CPU
# -ffast-math: Enable aggressive floating-point optimizations
# -funroll-loops: Unroll loops when beneficial
# -flto: Link-time optimization
# -DNDEBUG: Disable debug assertions
CFLAGS="-std=c++17 -pthread -O3 -march=native -mtune=native -ffast-math -funroll-loops -flto -DNDEBUG -Wno-comment"

case "$(uname -a)" in # automatically detect operating system
	 Darwin*) g++ src/*.cpp -o bin/OpenCL-Benchmark $CFLAGS -I./src/OpenCL/include -framework OpenCL               ;; # macOS
	*Android) g++ src/*.cpp -o bin/OpenCL-Benchmark $CFLAGS -I./src/OpenCL/include -L/system/vendor/lib64 -lOpenCL ;; # Android
	*       ) g++ src/*.cpp -o bin/OpenCL-Benchmark $CFLAGS -I./src/OpenCL/include -L./src/OpenCL/lib -lOpenCL     ;; # Linux
esac

if [[ $? == 0 ]]; then 
	echo "✓ Compilation successful with optimized flags!"
	bin/OpenCL-Benchmark "$@"
fi # run executable only if last compilation was successful
