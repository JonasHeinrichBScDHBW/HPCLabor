from typing import List, Tuple
from pathlib import Path
import subprocess
import os
from io import StringIO
import math
import pickle
import re
from datetime import timedelta

import numpy as np
import pandas as pd
from matplotlib import pyplot as plt
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D

DIR_SCRIPT = Path(__file__).parent
DIR_ROOT = DIR_SCRIPT.parent

DIR_BENCHMARKS = DIR_ROOT.joinpath("benchmarks")
DIR_BUILD = DIR_ROOT.joinpath("build")
DIR_OUTPUT = DIR_ROOT.joinpath("output")
DIR_PERFORATOR = DIR_ROOT.joinpath("perforator")
DIR_PLOTS = DIR_ROOT.joinpath("plots")

TEST_COMMAND = f"./{str(DIR_BUILD.joinpath('gameoflife'))}"
TEST_FUNCTION = "simulateSteps"

RUNS = 5
TIMESTEPS = 500  # determined to be >20s and <1min

FIGURE_SIZE = (15, 15)


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


def run_benchmark_perforator(benchmark: Benchmark) -> str:
    # NOTE: eg: sudo ./../perforator/perforator --csv -r simulateSteps ./gameoflife 1500 1000 1000

    if benchmark.threads != 1:
        raise ValueError("Perforator only supports exactly one thread.")

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
    env["PATH"] = str(DIR_PERFORATOR.resolve()) + ":" + env["PATH"]

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


def run_benchmark(benchmark: Benchmark) -> str:
    # NOTE: eg: /usr/bin/time -v ./gameoflife 1500 1000 1000

    command = [
        "/usr/bin/time",
        "-v",
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
    env["PATH"] = str(DIR_PERFORATOR.resolve()) + ":" + env["PATH"]

    retry = True
    while retry:
        try:
            lines = subprocess.check_output(command, env=env, stderr=subprocess.STDOUT,).decode("utf-8").split("\n")
            data = {}
            for line in lines:
                parts = line.split(": ")
                if len(parts) < 2:
                    continue

                data[parts[0].strip()] = [":".join(parts[1:]).strip(),]

            df = pd.DataFrame.from_dict(data)
            retry = False
        except subprocess.CalledProcessError as e:
            print("Detected an error in the called process - retrying!")
            print(e)

    if benchmark.data is not None:
        benchmark.data = benchmark.data.append(df.iloc[0], ignore_index=True)
    else:
        benchmark.data = df


def calculate_segments(threads: int) -> List[Tuple[int]]:
    # None, None is kept to let our program figure it out
    factors = [(None, None), (1, threads), ]

    if threads != 1:
        factors.append((threads, 1))

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
                print("Saving benchmark: " + str(benchmark))
                benchmark.save(DIR_BENCHMARKS)

        # NOTE: Keep for easy debugging
        #         break
        #     break
        # break


def load_benchmarks() -> List[Benchmark]:
    benchmarks = []
    for file_path in list(DIR_BENCHMARKS.glob("*.csv")):
        benchmarks.append(Benchmark.load(file_path))
    return benchmarks


#
# Plots
#


PARSE_TIME_REGEX_PERFORATOR = re.compile(
    r'((?P<hours>\d+?)hr)?((?P<minutes>\d+?)m)?((?P<seconds>\d+\.\d+?)s)?((?P<milliseconds>\d+\.\d+?)ms)?')


def parse_time_perforator(time_str):
    parts = PARSE_TIME_REGEX_PERFORATOR.match(time_str)
    if not parts:
        print("Found not match! Error!")
        return
    parts = parts.groupdict()
    time_params = {}
    for key in parts:
        value = parts[key]
        if value:
            time_params[key] = float(value)
    return timedelta(**time_params)


def parse_time(time_str):
    parts = time_str.split(":")
    if len(parts) == 2:
        return timedelta(hours=int(parts[0]), minutes=float(parts[1]))
    else:
        raise ValueError(f"Found an unkown format: {time_str=}")


def wrong_log(number, basis):
    if number == 0:
        return 0
    else:
        return math.log(number, basis)


def plot_3d_thread_size_time(benchmarks: List[Benchmark], z_metric="Elapsed (wall clock) time (h:mm:ss or m:ss)", z_label="$\log_4($Elapsed Wall Clock$)$", show=False):
    """
    3D Plot: Threads vs Size vs Time
    ================================

    - x: Board Size (1024, 2048, 4096)
    - y: number of threads (segments = None, None)
    - z: time elapsed
    """
    x = []
    y = []
    z = []

    for benchmark in benchmarks:
        if benchmark.segments_x != calculate_segments(benchmark.threads)[-1][0]:
            continue

        x.append(wrong_log(benchmark.width * benchmark.height, 4))
        y.append(wrong_log(benchmark.threads, 2))

        metrics = []
        for metric in benchmark.data[z_metric]:
            if ":" in metric:
                parsed_time = parse_time(metric)
                if parsed_time is not None:
                    metrics.append(parsed_time.total_seconds())
            else:
                metrics.append(metric)
        z.append(wrong_log(pd.Series(metrics).mean(), 4))

    X = np.array(x)
    Y = np.array(y)
    Z = np.array(z)

    fig = plt.figure(figsize=FIGURE_SIZE)
    ax = fig.add_subplot(111, projection='3d')
    ax.plot_trisurf(X, Y, Z, cmap=cm.coolwarm)

    ax.set_xlabel("$\log_4($Board Area$)$")
    ax.set_ylabel("$\log_2($Number of Threads$)$")
    ax.set_zlabel(z_label)
    plt.title(
        f"Game of Life Benchmark: Number of Threads vs Board Area vs {z_metric} (10 runs)", fontsize=14)
    ax.view_init(15, 135)

    plt.savefig(str(DIR_PLOTS.joinpath(f"thread_size_{z_metric}_3d.png")))
    if show:
        plt.show()


def plot_2d_segments_time(benchmarks: List[Benchmark], board_size=1024, y_metric="Elapsed (wall clock) time (h:mm:ss or m:ss)", y_label="$\log_4($Elapsed Wall Clock$)$", show=False):
    """
    2D Plot: Threads vs Size e
    ================================

    - x: segments
    - y: wall clock time
    """
    x = []
    y = []
    y_err = []

    for benchmark in benchmarks:
        if benchmark.width != board_size:
            continue

        x.append("x: " + str(benchmark.segments_x) +
                 "\ny: " + str(benchmark.segments_y) +
                 "\nt: " + str(benchmark.threads))

        metrics = []
        for metric in benchmark.data[y_metric]:
            if ":" in metric:
                parsed_time = parse_time(metric)
                if parsed_time is not None:
                    metrics.append(parsed_time.total_seconds())
            else:
                metrics.append(metric)
        
        series = pd.Series(metrics)
        y.append(wrong_log(series.mean(), 4))
        y_err.append(wrong_log(series.std(), 4))

    X = np.array(x)
    Y = np.array(y)
    index = np.argsort(Y)
    Y_sort = Y[index]
    X_sort = X[index]

    fig = plt.figure(figsize=FIGURE_SIZE)
    ax = fig.add_subplot(111)
    ax.bar(np.arange(len(Y_sort)), Y_sort, yerr=y_err, capsize=10)
    plt.xticks(range(len(Y_sort)), X_sort)

    ax.set_xlabel("Segments")
    ax.set_ylabel(y_label)
    plt.title(
        f"Game of Life Benchmark: Segments vs {y_metric} (10 runs | Board Size {board_size})", fontsize=14)
    ax.set_ylabel(y_label)

    plt.savefig(str(DIR_PLOTS.joinpath(f"segments_{y_metric}_2d.png")))
    if show:
        plt.show()


def visualize_benchmarks(benchmarks: List[Benchmark]):
    # Plots
    # =====
    #
    # - Time elapsed
    #     - x: number of threads
    #     - y: time elapsed
    # - Time elapsed
    #     - x: width (1024, 2048, 4096)
    #     - y: time elapsed

    plot_3d_thread_size_time(benchmarks)
    plot_2d_segments_time(benchmarks, board_size=1024, show=True)


def main():
    build()
    run_benchmarks()

    # benchmarks = load_benchmarks()
    # visualize_benchmarks(benchmarks)


if __name__ == "__main__":
    main()
