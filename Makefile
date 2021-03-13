IMPLEMENTATION_VERSION = 'MPI'

ifeq ($(IMPLEMENTATION_VERSION), 'MPI')
CC = mpicc # gcc | mpicc | icx
CPPC = mpic++ # g++ | mpic++ | icpx
ADDITIONAL_DEFS = -DGOL_VERSION_MPI
else
CC = gcc # gcc | mpicc | icx
CPPC = g++ # g++ | mpic++ | icpx
ADDITIONAL_DEFS = -DGOL_VERSION_OPENMP
endif


DEBUG_FLAGS = -DNO_DEBUG
# DEBUG_FLAGS = -DDEBUG -DVTK_OUTPUT

# compiler flags:
#  -g                     adds debugging information to the executable file
#  -Wall                  turns on most, but not all, compiler warnings
#  -Wno-format-truncation
ARCHITECTURE_DEFINITIONS = -DAVX_2 # -DAVX_2 | -DAVX_512
COMPILER_FLAGS           = -g -O0 -Wall -lc -lm -fopenmp -D _DEFAULT_SOURCE -march=native $(DEBUG_FLAGS) # -march=native | -march=skylake-avx512
COMPILER_FLAGS_C         = -std=c99
COMPILER_FLAGS_CPP       = -std=c++17

all: build-gol build-benchmark-cpp

# Source the intel environment - Prerequisite for running things compiled with icc
source-intel-env:
	source /opt/intel/oneapi/setvars.sh intel64

# Build pure C variante
build-gol: src/gameoflife.c
	$(CC) $(ARCHITECTURE_DEFINITIONS) $(ADDITIONAL_DEFS) src/gameoflife.c  $(COMPILER_FLAGS_C) $(COMPILER_FLAGS) -o build/gameoflife

# Run pure C variant
ifeq ($(IMPLEMENTATION_VERSION), 'MPI')
run-gol: build-gol
	mpirun -n 1 ./build/gameoflife 100 1024 1024
else
run-gol: build-gol
	./build/gameoflife
endif

# Build C++ Benchmark Wrapper
build-benchmark-cpp: src/benchmark.cpp
	$(CPPC) $(ARCHITECTURE_DEFINITIONS) src/benchmark.cpp $(COMPILER_FLAGS_CPP) $(COMPILER_FLAGS) -isystem google-benchmark/include -Lgoogle-benchmark/build/src -lbenchmark -lpthread -o build/benchmark

# Run C++ Benchmark Wrapper
run-benchmark-cpp: build-benchmark-cpp
	./build/benchmark --benchmark_report_aggregates_only=true --benchmark_repetitions=10 --benchmark_out_format=csv --benchmark_out=benchmarks/_cpp_benchmark.csv

# Run Python Benchmark wrapper
run-benchmark: all
	python3 src/benchmark.py

# Build scratchpad
scratchpad: src/scratchpad.c
	$(CC) -o build/scratchpad src/scratchpad.c $(COMPILER_FLAGS) $(COMPILER_FLAGS_C)
	mpirun -n 4 ./build/scratchpad

# Remove builds / Cleanup
clean:
	find output ! -name '.gitignore' -type f -exec rm -f {} +
	find build ! -name '.gitignore' -type f -exec rm -f {} +
