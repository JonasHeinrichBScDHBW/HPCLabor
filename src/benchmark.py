from typing import List, Tuple
from pathlib import Path
import subprocess
import os
from io import StringIO
import math
import pickle

import pandas as pd

CURRENT_DIRECTORY = Path(__file__).parent
BENCHMARK_DIRECTORY = CURRENT_DIRECTORY.joinpath("benchmarks")
PERFORATOR_PATH = CURRENT_DIRECTORY.joinpath("../perforator")

TEST_COMMAND = "./gameoflife"
TEST_FUNCTION = "simulateSteps"

RUNS = 10
TIMESTEPS = 1500 # determined to be >20s and <1min

class Benchmark:
    def __init__(self, threads, timesteps, width, height, segments_x=None, segments_y=None):
        self.threads: int = threads
        self.timesteps: int = timesteps
        self.width: int = width
        self.height: int = height

        self.segments_x: int = segments_x
        self.segments_y: int = segments_y

        self.data: pd.DataFrame = None
    
    @property
    def runs(self):
        if self.data is not None:
            return self.data.shape[0]
        else:
            return 0

    def __str__(self):
        return "<Benchmark threads={} timesteps={} width={} height={} segmentsx={} segmentsy={} runs={}>".format(
            self.threads, self.timesteps, self.width, self.height,
            self.segments_x, self.segments_y, self.runs)

    def save(self, path: Path):
        path_name = "benchmark_{}_{}_{}_{}_{}_{}.csv".format(
            self.threads, self.timesteps, self.width, self.height,
            self.segments_x, self.segments_y, self.runs)

        with open(str(path.joinpath(path_name)), "w") as f:
            f.write(self.data.to_csv())

    @staticmethod
    def load(file_path: Path) -> object:
        parts = file_path.name.split("_")
        if len(parts) != 7:
            raise ValueError("The file path does not conform to the standard")
        
        parts[-1] = parts[-1].split(".")[0]

        benchmark = Benchmark(
            threads=int(parts[1]),
            timesteps=int(parts[2]),
            width=int(parts[3]),
            height=int(parts[4]),
            segments_x=int(parts[5]) if parts[5] != "None" else None,
            segments_y=int(parts[6]) if parts[6] != "None" else None,
        )
        benchmark.data = pd.read_csv(str(file_path), index_col=0)
        return benchmark


def build():
    subprocess.call(["make"])


def run_benchmark(benchmark: Benchmark) -> str:
    # NOTE: eg: sudo ./../perforator/perforator --csv -r simulateSteps ./gameoflife 1500 1000 1000
    
    command = [
        "perforator",
        "--csv",
        "-r", TEST_FUNCTION,
        TEST_COMMAND,
        str(benchmark.timesteps),
        str(benchmark.width),
        str(benchmark.height)
    ]
    if benchmark.segments_x is not None and benchmark.segments_y is not None:
        command.append(str(benchmark.segments_x))
        command.append(str(benchmark.segments_y))

    env = dict(os.environ)
    env.update({"OMP_NUM_THREADS": str(benchmark.threads)})
    env["PATH"] = str(PERFORATOR_PATH.resolve()) + ":" + env["PATH"]

    retry = True
    while retry:
        try:
            output = subprocess.check_output(command, env=env).decode("utf-8")
            df = pd.read_csv(StringIO(output), index_col=False, header=None)
            retry = False
        except subprocess.CalledProcessError as e:
            print("Detected an error on the called process - retrying!")
            print(e)

    # Orient and mark header correctly
    df = df.transpose()
    df.columns = df.iloc[0]
    df.drop(df.index[0], inplace=True)

    if benchmark.data is not None:
        benchmark.data = benchmark.data.append(df.iloc[0], ignore_index=True)
    else:
        benchmark.data = df


def calculate_segments(threads: int) -> List[Tuple[int]]:
    # None, None is kept to let our program figure it out
    factors = [(None, None), (1, threads)]

    for factor1 in range(2, int(math.sqrt(threads)) + 1):
        if threads % factor1 == 0:
            factor2 = int(threads / factor1)
            factors.append((factor1, factor2))
            if factor1 != factor2:
                factors.append((factor2, factor1))

    return factors


def run_benchmarks():
    for size in [1024, 2048, 4096]:
        for threads in range(1, 9):
            for segments in calculate_segments(threads):
                benchmark = Benchmark(
                    threads=threads,
                    timesteps=TIMESTEPS,
                    width=size,
                    height=size,
                    segments_x=segments[0],
                    segments_y=segments[1])
                for _ in range(RUNS):
                    print("Running benchmark: " + str(benchmark))
                    run_benchmark(benchmark)
                benchmark.save(BENCHMARK_DIRECTORY)

        # NOTE: Keep for easy debugging
        #         break
        #     break
        # break


def load_benchmarks() -> List[Benchmark]:
    benchmarks = []
    for file_path in list(BENCHMARK_DIRECTORY.glob("*.csv")):
        benchmarks.append(Benchmark.load(file_path))
    return benchmarks


def visualize_benchmarks(benchmarks: List[Benchmark]):
    pass


def main():
    build()
    run_benchmarks()

    benchmarks = load_benchmarks()
    visualize_benchmarks(benchmarks)


if __name__ == "__main__":
    main()
