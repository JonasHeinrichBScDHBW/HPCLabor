# High Performance Computing Labor - Game of Life

## General Remarks

- Project compiles in both C and C++
  - C++ version with Google Benchmark for easy single threaded comparison (algorithm optimisation)
  - C version for actual benchmark
- Implementation with SIMD (AVX2 / AVX512 for fun)

## Project Structure

- `archive`: task material provided by the lecturer
- `benchmarks`: benchmark result cache
- `build`: compiled binaries
- `google-benchmark`: [Google Benchmark](https://github.com/google/benchmark) as a git submodule
- `output`: program output (mainly `.vtk` files)
- `perforator`: [Perforator](https://github.com/zyedidia/perforator) as a git submodule (`perf` events for single threaded code regions)
- `plots`: benchmark plots
- `src`: Source files
  - `benchmark.cpp`: Google Benchmark C++ wrapper for GOL
  - `benchmark.py`: Python benchmark wrapper and plotting
  - `gameoflife.c`: Entry point for C version
  - `gol_field.h`: Definitions and utilities regarding a GOL field used by other implementations
  - `gol_mpi.h`: GOL implementation that uses MPI
  - `gol_omp.h`: GOL implementation that uses OpenMP
  - `gol_plain_utils.h`: Utils for a plain gol implementation
  - `gol_vanilla.h`: GOL implementation that uses no framework (single threaded)
  - `scratchpad.c`: Scratchpad file for testing random things

## Uebung 1

### Aufgabe 3 - Performanceanalyse

#### 3a

*Berechnen Sie, wie viel Speicherplatz im Arbeitsspeicher und auf der Festplatte für den Gebietsgrößentest benötigt wird.*

- h: Segmenthöhe
- w: Segmentbreite

- Arbeitsspeicher: h * w * sizeof(char) (mindestens)
- Festplatte: h * w * sizeof(float) + sizeof(long) + xml_overhead * sizeof(char)

*Vergleichen Sie ihre berechneten Werte des Festplattenplatzes mit den tatsächlichen Größen der Outputdateien.*

- Segmentgöße: 8x15
- Arbeitsspeicher:       8 * 15
- Festplatte:            898 = 8 * 15 * 4 + 8 + 410 * 1
- Festplatte (gemessen): 898 (`ls -lah output/gol_mtp_00001_0_0.vti`)

Der berechnete und gemessene Wert auf der Festplatte ist gleich. Die Konstante `410` wurde im Code Editor durch
Selektion des XML-Tags exclusive `append-data` gemessen (analog aus C-Code zu berechnen). 

*Wie ist das Verhältnis der benötigten Größe auf dem Arbeitsspeicher zu der Output-Dateigröße? Notieren Sie Ihre Beobachtungen in kurzer Form.*

- Beispiel: 898 / 130 = 6.9
- Relativ (ohne bias): 520 / 130 = 4
    - Umwandeln von char zu float

## Uebung 3

### Aufgabe 1g

*Welche Probleme ergeben sich im Zusammenhang miteiner 2D Gebietszerlegung und welche Vorraussetzungen müssen an den Aufrufer des Programms gestellt werden?*

- erst ab 4 Prozessen (wirklich) sinnvoll
- gerade Anzahl von Prozessen oder 1
- Kommunikationsaufwand steigt mit 2 bzw. 6 Kommunikationspartnern
- Fuer periodisch: Torus Netzwerk vorteilhaft
- Je nach Eingabeformat und Segmentaufteilung muss die gesamte Feldgroesse durch die Anzahl der Prozesse teilbar sein

### Aufgabe 2

*