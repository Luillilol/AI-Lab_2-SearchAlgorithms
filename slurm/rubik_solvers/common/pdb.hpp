// =============================================================================
//  pdb.hpp  -  THE A* HEURISTIC: pattern databases (Korf, 1997)
// -----------------------------------------------------------------------------
//  h(state) = max( PDB_8corners(state), PDB_edgesA(state), PDB_edgesB(state) )
//
//  * A pattern database (PDB) stores, for every possible arrangement of a SUBSET of
//    the cubies (the "pattern"), the exact number of moves needed to solve that
//    subset while ignoring all the other cubies  ->  it never overestimates, so it
//    is an admissible heuristic. It is built once with a breadth-first search that
//    starts at the solved cube and uses the SAME 18 moves as the solvers.
//  * 8 corners : 8! * 3^7              = 88,179,840 entries
//  * 6 edges   : 12!/6! * 2^6          = 42,577,920 entries (two different sets of 6)
//    (option --edge-k 7: 12!/5! * 2^7  = 510,935,040 entries per table)
//  * The maximum of admissible heuristics is admissible (and consistent).
//  * We use one byte per entry (Korf used 4 bits) to keep the code simple.
//  * Tables are saved in a cache folder, so they are only built once.
// =============================================================================
#pragma once
#include <sys/stat.h>
#include <cmath>
#include <cstring>
#include "cube.hpp"
#include "stats.hpp"

namespace rc {

// ---- ranking of a k-permutation of n slots (mixed radix n, n-1, n-2, ...) ----
inline uint32_t rankKPerm(const uint8_t* p, int k, int n) {
    uint32_t r = 0;
    for (int i = 0; i < k; i++) {
        int d = p[i];
        for (int j = 0; j < i; j++) if (p[j] < p[i]) d--;
        r = r * (uint32_t)(n - i) + (uint32_t)d;
    }
    return r;
}
inline void unrankKPerm(uint32_t r, int k, int n, uint8_t* p) {
    int d[12];
    for (int i = k - 1; i >= 0; i--) { d[i] = (int)(r % (uint32_t)(n - i)); r /= (uint32_t)(n - i); }
    bool used[12] = {false};
    for (int i = 0; i < k; i++) {
        int c = d[i];
        for (int s = 0; s < n; s++) {
            if (used[s]) continue;
            if (c == 0) { p[i] = (uint8_t)s; used[s] = true; break; }
            c--;
        }
    }
}

// ---- breadth-first fill: dist[start] = 0, neighbours via next(index, move) ----
template <class Next>
inline void bfsFill(std::vector<uint8_t>& dist, size_t start, Next next) {
    const long long N = (long long)dist.size();
    std::fill(dist.begin(), dist.end(), (uint8_t)255);
    dist[start] = 0;
    for (int d = 0; d < 250; d++) {
        long long added = 0;
#pragma omp parallel for schedule(static) reduction(+ : added)
        for (long long i = 0; i < N; i++) {
            if (dist[i] != d) continue;
            for (int m = 0; m < NMOVES; m++) {
                size_t j = next((size_t)i, m);
                if (__atomic_load_n(&dist[j], __ATOMIC_RELAXED) == 255) {
                    __atomic_store_n(&dist[j], (uint8_t)(d + 1), __ATOMIC_RELAXED);
                    added++;
                }
            }
        }
        if (added == 0) break;
    }
}

struct PdbInfo {
    std::string name;
    uint64_t entries = 0;
    double avg = 0;
    int maxd = 0;
    std::vector<uint64_t> hist;
    double buildSec = 0;
    bool fromCache = false;
};

inline void computeStats(const std::vector<uint8_t>& dist, PdbInfo& in) {
    in.hist.assign(32, 0);
    double sum = 0;
    for (uint8_t v : dist) { in.hist[v < 32 ? v : 31]++; sum += v; }
    in.entries = dist.size();
    in.avg = sum / (double)dist.size();
    in.maxd = 0;
    for (int i = 0; i < 32; i++) if (in.hist[i]) in.maxd = i;
}

// ---- cache files -------------------------------------------------------------
struct PdbHeader {
    char magic[8];
    uint64_t layout, entries;
    double buildSec;
};
inline bool loadPdb(const std::string& path, std::vector<uint8_t>& dist, uint64_t entries, double& buildSec) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    PdbHeader h;
    bool ok = std::fread(&h, sizeof h, 1, f) == 1 && std::memcmp(h.magic, "RCPDB002", 8) == 0 && h.layout == G.layoutHash() && h.entries == entries;
    if (ok) {
        dist.resize(entries);
        ok = std::fread(dist.data(), 1, entries, f) == entries;
        buildSec = h.buildSec;
    }
    std::fclose(f);
    return ok;
}
inline void savePdb(const std::string& path, const std::vector<uint8_t>& dist, double buildSec) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::fprintf(stderr, "warning: cannot write %s (the table will be rebuilt next time)\n", path.c_str()); return; }
    PdbHeader h;
    std::memcpy(h.magic, "RCPDB002", 8);
    h.layout = G.layoutHash();
    h.entries = dist.size();
    h.buildSec = buildSec;
    std::fwrite(&h, sizeof h, 1, f);
    std::fwrite(dist.data(), 1, dist.size(), f);
    std::fclose(f);
}

// =============================================================================
//  PDB of the 8 corners
// =============================================================================
class CornerPDB {
public:
    static constexpr uint32_t NP = 40320, NO = 2187;
    std::vector<uint8_t> dist;
    PdbInfo info;

    static uint32_t index(const Cube& c) {
        uint32_t o = 0;
        for (int i = 6; i >= 0; i--) o = o * 3 + c.co[i];
        return rankKPerm(c.cp, 8, 8) * NO + o;
    }
    int get(const Cube& c) const { return dist[index(c)]; }

    void init(const std::string& dir, bool verbose) {
        info.name = "8 corners";
        std::string path = dir + "/corners8.pdb";
        auto t0 = Clock::now();
        if (loadPdb(path, dist, (uint64_t)NP * NO, info.buildSec)) {
            info.fromCache = true;
        } else {
            build();
            info.buildSec = secondsSince(t0);
            savePdb(path, dist, info.buildSec);
        }
        computeStats(dist, info);
        if (verbose) report(info, secondsSince(t0));
    }
    static void report(const PdbInfo& i, double sec);

private:
    void build() {
        std::vector<uint16_t> pm((size_t)NMOVES * NP), om((size_t)NMOVES * NO);
#pragma omp parallel for schedule(static)
        for (long long p = 0; p < NP; p++) {
            uint8_t cp[8], nc[8];
            unrankKPerm((uint32_t)p, 8, 8, cp);
            for (int m = 0; m < NMOVES; m++) {
                for (int i = 0; i < 8; i++) nc[i] = cp[G.csrc[m][i]];
                pm[(size_t)m * NP + p] = (uint16_t)rankKPerm(nc, 8, 8);
            }
        }
        for (uint32_t o = 0; o < NO; o++) {
            int co[8], sum = 0, x = (int)o;
            for (int i = 0; i < 7; i++) { co[i] = x % 3; x /= 3; sum += co[i]; }
            co[7] = (3 - sum % 3) % 3;
            for (int m = 0; m < NMOVES; m++) {
                uint32_t r = 0;
                int nco[8];
                for (int i = 0; i < 8; i++) nco[i] = (co[G.csrc[m][i]] + G.cdel[m][i]) % 3;
                for (int i = 6; i >= 0; i--) r = r * 3 + (uint32_t)nco[i];
                om[(size_t)m * NO + o] = (uint16_t)r;
            }
        }
        dist.assign((size_t)NP * NO, 255);
        bfsFill(dist, 0, [&](size_t i, int m) -> size_t {
            return (size_t)pm[(size_t)m * NP + i / NO] * NO + om[(size_t)m * NO + i % NO];
        });
    }
};

// =============================================================================
//  PDB of k edges (cubies lo .. lo+k-1)
// =============================================================================
class EdgePDB {
public:
    int k = 6;
    int ids[8] = {0, 0, 0, 0, 0, 0, 0, 0};   // the tracked edge cubies
    int jOf[12];                              // cubie id -> index in ids (or -1)
    uint32_t NP = 0;
    std::vector<uint8_t> dist;
    PdbInfo info;

    uint32_t index(const Cube& c) const {
        uint8_t pos[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        uint32_t ori = 0;
        for (int s = 0; s < 12; s++) {
            int j = jOf[c.ep[s]];
            if (j >= 0) { pos[j] = (uint8_t)s; ori |= (uint32_t)c.eo[s] << j; }
        }
        return (rankKPerm(pos, k, 12) << k) | ori;
    }
    int get(const Cube& c) const { return dist[index(c)]; }

    void init(const std::vector<int>& list, const std::string& dir, bool verbose) {
        k = (int)list.size();
        for (int i = 0; i < 12; i++) jOf[i] = -1;
        std::string tag;
        for (int i = 0; i < k; i++) { ids[i] = list[i]; jOf[list[i]] = i; tag += (i ? "-" : "") + std::to_string(list[i]); }
        NP = 1;
        for (int i = 0; i < k; i++) NP *= (uint32_t)(12 - i);
        info.name = std::to_string(k) + " edges (" + tag + ")";
        std::string path = dir + "/edges" + std::to_string(k) + "_" + tag + ".pdb";
        auto t0 = Clock::now();
        if (loadPdb(path, dist, (uint64_t)NP << k, info.buildSec)) {
            info.fromCache = true;
        } else {
            build();
            info.buildSec = secondsSince(t0);
            savePdb(path, dist, info.buildSec);
        }
        computeStats(dist, info);
        if (verbose) CornerPDB::report(info, secondsSince(t0));
    }

private:
    void build() {
        std::vector<uint32_t> pm((size_t)NMOVES * NP);
        std::vector<uint8_t> mk((size_t)NMOVES * NP);
        uint8_t dest[NMOVES][12];
        for (int m = 0; m < NMOVES; m++) for (int t = 0; t < 12; t++) dest[m][G.esrc[m][t]] = (uint8_t)t;
#pragma omp parallel for schedule(static)
        for (long long r = 0; r < (long long)NP; r++) {
            uint8_t pos[8] = {0}, np[8] = {0};
            unrankKPerm((uint32_t)r, k, 12, pos);
            for (int m = 0; m < NMOVES; m++) {
                uint32_t mask = 0;
                for (int i = 0; i < k; i++) {
                    np[i] = dest[m][pos[i]];
                    mask |= (uint32_t)G.edel[m][np[i]] << i;
                }
                pm[(size_t)m * NP + r] = rankKPerm(np, k, 12);
                mk[(size_t)m * NP + r] = (uint8_t)mask;
            }
        }
        uint8_t goalPos[8];
        for (int i = 0; i < k && i < 8; i++) goalPos[i] = (uint8_t)ids[i];
        size_t start = (size_t)rankKPerm(goalPos, k, 12) << k;
        dist.assign((size_t)NP << k, 255);
        const int kk = k;
        const uint32_t om = (1u << k) - 1;
        bfsFill(dist, start, [&](size_t i, int m) -> size_t {
            size_t r = i >> kk;
            uint32_t o = (uint32_t)(i & om);
            return ((size_t)pm[(size_t)m * NP + r] << kk) | (o ^ mk[(size_t)m * NP + r]);
        });
    }
};

inline void CornerPDB::report(const PdbInfo& i, double sec) {
    std::printf("  PDB %-28s %12llu entries, average h = %.3f, max = %d, %s in %.1fs\n", i.name.c_str(), (unsigned long long)i.entries,
                i.avg, i.maxd, i.fromCache ? "loaded" : "built", sec);
}

// =============================================================================
//  The heuristic
// =============================================================================
class Heuristic {
public:
    CornerPDB corners;
    EdgePDB edgesA, edgesB;
    double setupSeconds = 0;

    // Which edge cubies each table tracks. Edge cubie ids are the slots of the solved cube (sorted:
    // UL DL UR DR UF DF LF RF UB DB LB RB). Table A = the first k ids, table B = the last k ids
    // (for k = 6 they are disjoint, like in Korf's heuristic; for k = 7 they overlap in 2 edges, which is fine for max()).
    static std::vector<int> edgeSet(int which, int k) {
        std::vector<int> v;
        for (int i = 0; i < k; i++) v.push_back(which == 0 ? i : 12 - k + i);
        return v;
    }
    void init(int edgeK, const std::string& dir, bool verbose = true) {
        ::mkdir(dir.c_str(), 0755);
        auto t0 = Clock::now();
        if (verbose) std::printf("Pattern databases (cache folder: %s)\n", dir.c_str());
        corners.init(dir, verbose);
        edgesA.init(edgeSet(0, edgeK), dir, verbose);
        edgesB.init(edgeSet(1, edgeK), dir, verbose);
        setupSeconds = secondsSince(t0);
        if (verbose) {
            std::printf("  depth histograms (number of entries per distance):\n");
            for (const PdbInfo* p : {&corners.info, &edgesA.info, &edgesB.info}) {
                std::printf("   %-28s", p->name.c_str());
                for (int d = 0; d <= p->maxd; d++) std::printf(" %d:%llu", d, (unsigned long long)p->hist[d]);
                std::printf("\n");
            }
            std::printf("  heuristic ready in %.1fs\n", setupSeconds);
        }
    }
    // admissible and consistent: the maximum of exact sub-problem distances
    int eval(const Cube& c) const {
        int h = corners.get(c);
        int a = edgesA.get(c);
        if (a > h) h = a;
        int b = edgesB.get(c);
        if (b > h) h = b;
        return h;
    }
};

inline Heuristic* HEUR = nullptr;

}  // namespace rc
