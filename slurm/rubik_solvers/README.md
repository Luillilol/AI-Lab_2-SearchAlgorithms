# Rubik's Cube solvers (C++17, OpenMP)

Six programs, same command line, same CSV:

| Algorithm | Sequential | Parallel (OpenMP) |
|---|---|---|
| IDDFS (iterative deepening DFS) | `iddfs_seq` | `iddfs_par` |
| Bidirectional search | `bidir_seq` | `bidir_par` |
| A* (Korf pattern-database heuristic) | `astar_seq` | `astar_par` |

All of them use the 18 moves (`U U2 U' D D2 D' L L2 L' R R2 R' F F2 F' B B2 B'`, half-turn metric),
return a shortest solution (IDDFS, Bidirectional and A* are all optimal) and verify it independently.

## Build
    make            # all six        (make seq / make par)
Needs g++ >= 9 with OpenMP. `CXXFLAGS="-O3 -std=c++17"` if `-march=native` is a problem.

## Use
    # one cube: a scramble ...
    ./astar_par --input "F U B F' R U L D R B'"
    # ... or the 54 stickers (B R W O G Y, same order as the Python code)
    ./astar_par --input "YRGYBWRYWGBBBRORWOYGGRWOBRYRGOBORBYRWGOWGBBYGWOOWYOWGY"

    # many random scrambles (the same --seed gives the same cubes to every algorithm)
    ./astar_par --depths 1-19 --per-depth 10 --seed 2026 --timeout 600 --mem-gb 120 --csv astar_par.csv

    ./astar_par --selftest        # correctness checks
    ./astar_par --build-only      # only build the pattern databases (cached in ./pdb_cache)

Options: `--timeout SEC` (per solve), `--mem-gb GB` (per solve, search structures only, the 173 MB of
pattern databases are not counted), `--threads N`, `--edge-k 6|7`, `--pdb-dir DIR`, `--max-depth N` (IDDFS).
The first A* run builds the pattern databases (about 12 s with 2 threads; single thread a few minutes).
Run `./astar_par --build-only` once and the sequential version will just load the files.

## CSV columns (one row per cube)
algorithm, version, threads, test_id, scramble_depth, scramble, **status** (SOLVED / TIMEOUT / OUT_OF_MEMORY),
solved, verified, **solution_length**, solution, **time_s**, **mem_peak_mb**, mem_search_mb, nodes_expanded,
nodes_generated, timeout_s, mem_limit_gb, seed, setup_s, plus algorithm columns
(`depth_completed` for IDDFS, `nodes_stored` for Bidirectional/A*, `h_initial` for A*).
`mem_peak_mb` = peak resident memory of the process during the solve; `mem_search_mb` = peak minus memory before the solve.

## Adding a metric
`common/stats.hpp` explains it; in short, add `r.add("my_metric", value);` in `common/driver.hpp`
(marked with `>>> new metrics <<<`) or push `{"name", value}` to `out.extra` inside a solver.
The header of the CSV is generated from the first record.

## Layout
    common/cube.hpp      cube model, moves, parsing (scramble / 54 stickers), validation
    common/pdb.hpp       A* heuristic (pattern databases)
    common/table.hpp     hash tables (sharded, lock-free parallel inserts)
    common/stats.hpp     timing, memory, CSV module
    common/driver.hpp    command line, random scrambles, verification, CSV rows
    tools/compare_heuristics.cpp   average h of the old and the new heuristic
    report/              LaTeX section of the heuristic, IEEE references, figure
