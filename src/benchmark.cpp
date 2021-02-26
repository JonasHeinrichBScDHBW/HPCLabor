#include <benchmark/benchmark.h>
#include <omp.h>

#include "gol_field.h"
#include "gol_vanilla.h"
#include "gol_omp.h"
#include "gol_mpi.h"

static void BM_SimulateStep(benchmark::State &state, simulate_func simulateFunc)
{
    int boardSize = state.range(0);
    int threads = state.range(1);

    struct Field currentField;
    struct Field newField;
    initializeFields(&currentField, &newField, boardSize, boardSize, 0, 0);

    fillRandom(&currentField);

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    int timestep = 0;
    for (auto _ : state)
    {
        simulateFunc(&currentField, &newField, timestep);
        timestep++;

        benchmark::DoNotOptimize(currentField);
        benchmark::DoNotOptimize(newField);
        benchmark::ClobberMemory(); // Force write to memory
    }
    // Number of processed cells
    state.SetItemsProcessed(boardSize * boardSize * state.iterations());

    free(currentField.field);
    free(newField.field);
}

#define GOL_BENCHMARK_BOARD_SIZES \
    {                             \
        1 << 10, 1 << 11, 1 << 12 \
    }
#define GOL_BENCHMARK_THREADS  \
    {                          \
        1, 2, 3, 4, 5, 6, 7, 8 \
    }
#define GOL_BENCHMARK_RANGE(Threads) ArgsProduct({GOL_BENCHMARK_BOARD_SIZES, Threads})

BENCHMARK_CAPTURE(BM_SimulateStep, Vanilla_Plain, &simulateStepVanillaPlain)->GOL_BENCHMARK_RANGE({1});
BENCHMARK_CAPTURE(BM_SimulateStep, OMP_Plain, &simulateStepOMPPlain)->GOL_BENCHMARK_RANGE(GOL_BENCHMARK_THREADS);

#undef BenchmarkRange

BENCHMARK_MAIN();
