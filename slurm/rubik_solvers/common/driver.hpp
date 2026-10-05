// =============================================================================
//  driver.hpp  -  command line, test generation, verification and CSV output
//  (shared by the 6 programs; every program only has to provide a solve() function)
// =============================================================================
#pragma once
#include <atomic>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_set>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "cube.hpp"
#include "pdb.hpp"
#include "stats.hpp"
#include "table.hpp"

namespace rc {

// ----------------------------------------------------------------------------- solver interface
struct Config {
    double timeoutSec = 60;   // time limit of ONE solve
    double memGB = 100;       // memory limit of ONE solve (stay below the Slurm limit)
    int threads = 1;
    int edgeK = 6;            // size of the edge pattern databases (A*)
    std::string pdbDir = "pdb_cache";
    int maxDepth = 26;        // IDDFS depth cap
};
struct Ctl {  // what a solver needs to know to stop in time
    Clock::time_point end;
    size_t memLimit = 0;
    bool expired() const { return Clock::now() >= end; }
};
enum class Status { SOLVED, TIMEOUT, OUT_OF_MEMORY, NO_SOLUTION };
inline const char* statusName(Status s) {
    switch (s) {
        case Status::SOLVED: return "SOLVED";
        case Status::TIMEOUT: return "TIMEOUT";
        case Status::OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        default: return "NO_SOLUTION";
    }
}
struct SolveOut {
    Status status = Status::NO_SOLUTION;
    std::vector<int> moves;
    uint64_t expanded = 0, generated = 0;
    std::vector<std::pair<std::string, double>> extra;  // algorithm specific metrics (extra CSV columns)
};
using SolveFn = SolveOut (*)(const Cube&, const Config&, const Ctl&);
struct AlgoInfo {
    const char* name;      // "IDDFS", "Bidirectional", "A*"
    const char* version;   // "sequential" / "parallel"
    bool parallel;
    bool needsPdb;
    SolveFn solve;
};

// ----------------------------------------------------------------------------- scrambles
inline uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
// random scramble of exactly `depth` moves (no redundant move pairs) -> same seed = same cubes for all the algorithms
inline std::vector<int> randomScramble(int depth, uint64_t seed) {
    std::vector<int> mv;
    uint64_t x = seed;
    int last = -1;
    while ((int)mv.size() < depth) {
        int m = (int)(splitmix64(x) % NMOVES);
        if (!allowedAfter(last, m / 3)) continue;
        mv.push_back(m);
        last = m / 3;
    }
    return mv;
}

struct TestCase {
    int depth;
    int index;
    std::string scramble;
    Cube cube;
};

// ----------------------------------------------------------------------------- self test
inline int selfTest(const AlgoInfo& A, const Config& cfg) {
    (void)cfg;
    int bad = 0;
    auto check = [&](bool ok, const char* what) { std::printf("  [%s] %s\n", ok ? " OK " : "FAIL", what); if (!ok) bad++; };
    uint64_t seed = 99;
    // 1) cubie model == sticker model (the permutations of the Python code)
    bool ok1 = true, ok2 = true, ok3 = true;
    for (int t = 0; t < 500 && ok1; t++) {
        std::string st(G.goal, 54);
        Cube c = solvedCube(), n;
        int len = 1 + (int)(splitmix64(seed) % 40);
        for (int i = 0; i < len; i++) {
            int m = (int)(splitmix64(seed) % NMOVES);
            std::string ns(54, ' ');
            for (int j = 0; j < 54; j++) ns[j] = st[G.perm[m][j]];
            st = ns;
            applyMove(c, m, n);
            c = n;
        }
        ok1 = (cubeToStickers(c) == st);
        Cube d;
        std::string err;
        ok2 = ok2 && stickersToCube(st, d, err) && sameCube(c, d) && sameCube(unpack(pack(c)), c);
        Cube e;
        ok3 = ok3 && parseInput(st, e, err) && sameCube(e, c);
    }
    check(ok1, "cubie model agrees with the 54-sticker model of the Python code");
    check(ok2, "54-sticker input -> cube -> key -> cube round trip");
    check(ok3, "raw 54-sticker input is detected and parsed");
    // 2) every move followed by its inverse is the identity
    bool ok4 = true;
    for (int m = 0; m < NMOVES; m++) {
        Cube c = applyMoves(solvedCube(), {m, 3 * (m / 3) + (2 - m % 3)});
        ok4 = ok4 && isSolved(c);
    }
    check(ok4, "move followed by its inverse returns to the solved cube");
    // 3) number of different states at depth 1..4 must be the known numbers 18, 243, 3240, 43239
    struct H { size_t operator()(const Key& k) const { return (size_t)(hashKey(k)); } };
    std::unordered_set<Key, H> seen;
    std::vector<Key> fr{pack(solvedCube())};
    seen.insert(fr[0]);
    const size_t expect[5] = {1, 18, 243, 3240, 43239};
    bool ok5 = true;
    for (int d = 1; d <= 4; d++) {
        std::vector<Key> nx;
        for (auto& k : fr) { Cube c = unpack(k), n; for (int m = 0; m < NMOVES; m++) { applyMove(c, m, n); Key kk = pack(n); if (seen.insert(kk).second) nx.push_back(kk); } }
        ok5 = ok5 && nx.size() == expect[d];
        fr = nx;
    }
    check(ok5, "18 moves generate 18, 243, 3240, 43239 new states at depth 1..4 (known values)");
    // 4) heuristic: admissible (h <= scramble length) and consistent (|h(a)-h(b)| <= 1)
    if (A.needsPdb && HEUR) {
        bool adm = true, cons = true;
        for (int t = 0; t < 3000; t++) {
            int len = (int)(splitmix64(seed) % 14);
            Cube c = applyMoves(solvedCube(), randomScramble(len, splitmix64(seed))), n;
            int h = HEUR->eval(c);
            if (h > len) adm = false;
            for (int m = 0; m < NMOVES; m++) { applyMove(c, m, n); if (std::abs(HEUR->eval(n) - h) > 1) cons = false; }
        }
        check(adm, "heuristic never exceeds the length of the scramble (admissible)");
        check(cons, "heuristic changes by at most 1 per move (consistent)");
    }
    std::printf(bad ? "SELF-TEST FAILED\n" : "SELF-TEST PASSED\n");
    return bad ? 1 : 0;
}

// ----------------------------------------------------------------------------- main
inline void usage(const char* prog) {
    std::printf(
        "Usage: %s [options]\n"
        "  Solve ONE cube:\n"
        "    --input \"F U B' R2 ...\"      scramble (applied to the solved cube)  OR  54 sticker letters (B R W O G Y)\n"
        "  Test MANY random scrambles (statistics):\n"
        "    --depths 1-19                 scramble lengths to test (also: 7 or 5-12)\n"
        "    --per-depth N                 scrambles per length (default 5)\n"
        "    --seed S                      random seed (same seed = same cubes for every algorithm)\n"
        "  Limits and output:\n"
        "    --timeout SEC                 time limit of each solve (default 60)\n"
        "    --mem-gb GB                   memory limit of each solve (default 100)\n"
        "    --csv FILE                    append one row per test to this CSV file\n"
        "    --threads N                   (parallel programs) number of threads, default = all\n"
        "    --edge-k 6|7                  (A*) size of the edge pattern databases, default 6\n"
        "    --pdb-dir DIR                 (A*) folder where the pattern databases are cached\n"
        "    --max-depth N                 (IDDFS) deepest limit to try, default 26\n"
        "    --build-only                  (A*) only build/load the pattern databases and exit\n"
        "    --selftest                    run the correctness checks and exit\n"
        "    --quiet                       less console output\n",
        prog);
}

inline int runMain(int argc, char** argv, const AlgoInfo& A) {
    Config cfg;
    std::string input, csvPath;
    int dLo = -1, dHi = -1, perDepth = 5;
    uint64_t seed = 2026;
    bool doSelf = false, buildOnly = false, quiet = false, haveThreads = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value after %s\n", a.c_str()); std::exit(2); }
            return argv[++i];
        };
        if (a == "--input" || a == "--scramble" || a == "--state") input = next();
        else if (a == "--depths") {
            std::string v = next();
            size_t dash = v.find('-');
            if (dash == std::string::npos) dLo = dHi = std::atoi(v.c_str());
            else { dLo = std::atoi(v.substr(0, dash).c_str()); dHi = std::atoi(v.substr(dash + 1).c_str()); }
        }
        else if (a == "--per-depth") perDepth = std::atoi(next().c_str());
        else if (a == "--seed") seed = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--timeout") cfg.timeoutSec = std::atof(next().c_str());
        else if (a == "--mem-gb") cfg.memGB = std::atof(next().c_str());
        else if (a == "--csv") csvPath = next();
        else if (a == "--threads") { cfg.threads = std::atoi(next().c_str()); haveThreads = true; }
        else if (a == "--edge-k") cfg.edgeK = std::atoi(next().c_str());
        else if (a == "--pdb-dir") cfg.pdbDir = next();
        else if (a == "--max-depth") cfg.maxDepth = std::atoi(next().c_str());
        else if (a == "--selftest") doSelf = true;
        else if (a == "--build-only") buildOnly = true;
        else if (a == "--quiet") quiet = true;
        else if (a == "--help" || a == "-h") { usage(argv[0]); return 0; }
        else if (a[0] != '-' && input.empty()) input = a;
        else { std::fprintf(stderr, "unknown option %s (try --help)\n", a.c_str()); return 2; }
    }
#ifdef _OPENMP
    if (A.parallel) {
        if (!haveThreads || cfg.threads <= 0) cfg.threads = omp_get_max_threads();
        omp_set_num_threads(cfg.threads);
    } else cfg.threads = 1;
#else
    cfg.threads = 1;
    (void)haveThreads;
    if (A.parallel) std::fprintf(stderr, "warning: compiled without OpenMP, running with 1 thread\n");
#endif
    if (cfg.edgeK < 5 || cfg.edgeK > 8) { std::fprintf(stderr, "--edge-k must be between 5 and 8\n"); return 2; }

    std::printf("== %s (%s, %d thread%s) ==\n", A.name, A.version, cfg.threads, cfg.threads == 1 ? "" : "s");

    // ---- collect the tests
    std::vector<TestCase> tests;
    if (doSelf || buildOnly) {
        // nothing to collect
    } else if (!input.empty()) {
        TestCase t;
        std::string err;
        if (!parseInput(input, t.cube, err)) { std::fprintf(stderr, "invalid input: %s\n", err.c_str()); return 2; }
        t.depth = -1;
        t.index = 0;
        t.scramble = input;
        tests.push_back(t);
    } else if (dLo >= 0) {
        for (int d = dLo; d <= dHi; d++)
            for (int k = 0; k < perDepth; k++) {
                TestCase t;
                std::vector<int> mv = randomScramble(d, seed * 1000003ULL + (uint64_t)d * 10007ULL + (uint64_t)k);
                t.depth = d; t.index = k; t.scramble = movesToString(mv);
                t.cube = applyMoves(solvedCube(), mv);
                tests.push_back(t);
            }
        if (csvPath.empty()) csvPath = std::string("results_") + (A.name[0] == 'A' ? "astar" : A.name[0] == 'B' ? "bidirectional" : "iddfs") + "_" + (A.parallel ? "par" : "seq") + ".csv";
    } else { usage(argv[0]); return 1; }

    Heuristic heur;
    double setupSec = 0;
    if (A.needsPdb) {
        heur.init(cfg.edgeK, cfg.pdbDir, true);
        HEUR = &heur;
        setupSec = heur.setupSeconds;
        if (buildOnly) return 0;
    }
    if (doSelf) return selfTest(A, cfg);

    CsvWriter csv;
    if (!csvPath.empty()) {
        if (!csv.open(csvPath)) { std::fprintf(stderr, "cannot open %s\n", csvPath.c_str()); return 2; }
        std::printf("CSV: %s\n", csvPath.c_str());
    }
    const bool canResetPeak = resetPeakRss();
    if (!canResetPeak) std::fprintf(stderr, "note: cannot reset the peak-memory counter, mem_peak_mb will be the maximum since the program started\n");

    struct Agg { int n = 0, solved = 0; double t = 0, mv = 0; };
    std::map<int, Agg> agg;
    int id = 0;
    for (const TestCase& tc : tests) {
        Ctl ctl;
        ctl.memLimit = (size_t)(cfg.memGB * 1024.0 * 1024.0 * 1024.0);
        resetPeakRss();
        const double base = currentRssMB();
        auto t0 = Clock::now();
        ctl.end = t0 + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(cfg.timeoutSec));
        SolveOut out = A.solve(tc.cube, cfg, ctl);
        const double sec = secondsSince(t0);
        const double peak = peakRssMB();

        bool verified = false;
        if (out.status == Status::SOLVED) verified = isSolved(applyMoves(tc.cube, out.moves));
        const bool solved = out.status == Status::SOLVED && verified;
        std::string sol = movesToString(out.moves);

        Record r;
        r.add("algorithm", A.name).add("version", A.version).add("threads", cfg.threads);
        r.add("test_id", id).add("scramble_depth", tc.depth).add("scramble", tc.scramble);
        r.add("status", statusName(out.status)).add("solved", solved).add("verified", verified);
        r.add("solution_length", out.status == Status::SOLVED ? (int)out.moves.size() : -1).add("solution", sol);
        r.add("time_s", sec).add("mem_peak_mb", peak, 1).add("mem_search_mb", peak > base ? peak - base : 0.0, 1);
        r.add("nodes_expanded", (unsigned long long)out.expanded).add("nodes_generated", (unsigned long long)out.generated);
        r.add("timeout_s", cfg.timeoutSec, 1).add("mem_limit_gb", cfg.memGB, 1).add("seed", (unsigned long long)seed).add("setup_s", setupSec, 3);
        for (auto& e : out.extra) r.add(e.first, e.second, 3);   // algorithm specific columns
        // >>> new metrics: add them here with  r.add("name", value);  <<<
        if (csv.isOpen()) csv.write(r);

        Agg& g = agg[tc.depth];
        g.n++;
        if (solved) { g.solved++; g.t += sec; g.mv += (double)out.moves.size(); }

        if (!quiet || tests.size() == 1) {
            std::printf("[%d] depth %2d  %-13s %s  %2s moves  %9.3fs  peak %8.1f MB  expanded %llu\n", id, tc.depth, statusName(out.status),
                        solved ? "verified" : "        ", solved ? std::to_string(out.moves.size()).c_str() : "-", sec, peak,
                        (unsigned long long)out.expanded);
        }
        if (tests.size() == 1) {
            if (out.status == Status::SOLVED) std::printf("Solution (%zu moves): %s\n", out.moves.size(), sol.empty() ? "(already solved)" : sol.c_str());
            else std::printf("No solution returned: %s\n", statusName(out.status));
        }
        id++;
    }
    if (tests.size() > 1) {
        std::printf("\n depth | tests | solved | avg time (s, solved) | avg moves (solved)\n");
        for (auto& kv : agg)
            std::printf(" %5d | %5d | %6d | %20.4f | %.2f\n", kv.first, kv.second.n, kv.second.solved,
                        kv.second.solved ? kv.second.t / kv.second.solved : 0.0, kv.second.solved ? kv.second.mv / kv.second.solved : 0.0);
    }
    return 0;
}

}  // namespace rc
