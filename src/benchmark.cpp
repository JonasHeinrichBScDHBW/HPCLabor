#include <benchmark/benchmark.h>
#include <omp.h>

#include "gol_field.h"
#include "gol_vanilla.h"
#include "gol_omp.h"
#include "gol_mpi.h"

static void BM_SimulateStep(benchmark::State &state, simulate_func simulateFunc)
{
    int boardSize = state.range(0);

    struct Field field1;
    struct Field field2;

    struct Field *currentFieldPtr = &field1;
    struct Field *newFieldPtr = &field2;
    initializeFields(currentFieldPtr, newFieldPtr, boardSize, boardSize, 0, 0);

    fillRandom(currentFieldPtr);

    omp_set_dynamic(0);
    omp_set_num_threads(1);

    struct Field *temp;
    int timestep = 0;
    for (auto _ : state)
    {
        simulateFunc(currentFieldPtr, newFieldPtr, timestep);
        timestep++;

        temp = currentFieldPtr;
        currentFieldPtr = newFieldPtr;
        newFieldPtr = temp;

        benchmark::DoNotOptimize(currentFieldPtr);
        benchmark::DoNotOptimize(newFieldPtr);
        benchmark::DoNotOptimize(timestep);
        benchmark::ClobberMemory(); // Force write to memory
    }
    // Number of processed cells
    state.SetItemsProcessed(boardSize * boardSize * state.iterations());

    free(field1.field);
    free(field2.field);
}

// #define GOL_BENCHMARK_BOARD_SIZES \
//     {                             \
//         1 << 10, 1 << 11, 1 << 12 \
//     }

#define GOL_BENCHMARK_BOARD_SIZES \
    {                             \
        (1 << 10) + 2             \
    }
BENCHMARK_CAPTURE(BM_SimulateStep, Vanilla_Plain, &simulateStepVanillaPlain)->Args(GOL_BENCHMARK_BOARD_SIZES);
BENCHMARK_CAPTURE(BM_SimulateStep, Vanilla_SIMD, &simulateStepVanillaSIMD)->Args(GOL_BENCHMARK_BOARD_SIZES);
BENCHMARK_CAPTURE(BM_SimulateStep, OMP_Plain, &simulateStepOMPPlain)->Args(GOL_BENCHMARK_BOARD_SIZES);

#undef BenchmarkRange

BENCHMARK_MAIN();
