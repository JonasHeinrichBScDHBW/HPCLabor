# the compiler: gcc for C program, define as g++ for C++
CC = icx # gcc | icx
CPPC = icpx # g++ | icpx

# compiler flags:
#  -g                     adds debugging information to the executable file
#  -Wall                  turns on most, but not all, compiler warnings
#  -Wno-format-truncation
ARCHITECTURE_DEFINITIONS = -DAVX_512 # -DAVX_2 | -DAVX_512
COMPILER_FLAGS           = -g -O3 -Wall -lc -lm -fopenmp -D _DEFAULT_SOURCE -march=skylake-avx512
COMPILER_FLAGS_C         = -std=c99
COMPILER_FLAGS_CPP       = -std=c++17

all: build-gol build-benchmark-cpp

# Source the intel environment - Prerequisite for running things compiled with icc
source-intel-env:
	source /opt/intel/oneapi/setvars.sh intel64

# Build pure C variante
build-gol: src/gameoflife.c
	$(CC) $(ARCHITECTURE_DEFINITIONS) src/gameoflife.c  $(COMPILER_FLAGS_C)   $(COMPILER_FLAGS) -o build/gameoflife

# Run pure C variant
run-gol: build-gol
	./build/gameoflife

# Build C++ Benchmark Wrapper
build-benchmark-cpp: src/benchmark.cpp
	$(CPPC) $(ARCHITECTURE_DEFINITIONS) src/benchmark.cpp $(COMPILER_FLAGS_CPP) $(COMPILER_FLAGS) -isystem google-benchmark/include -Lgoogle-benchmark/build/src -lbenchmark -lpthread -o build/benchmark

# Run C++ Benchmark Wrapper
run-benchmark-cpp: build-benchmark-cpp
	./build/benchmark --benchmark_report_aggregates_only=true --benchmark_repetitions=10

# Run Python Benchmark wrapper
run-benchmark: all
	python3 src/benchmark.py

# Build scratchpad
scratchpad: src/scratchpad.c
	$(CC) -o build/scratchpad src/scratchpad.c $(COMPILER_FLAGS) $(COMPILER_FLAGS_C)
	./build/scratchpad

# Remove builds / Cleanup
clean:
	find build ! -name '.gitignore' -type f -exec rm -f {} +
