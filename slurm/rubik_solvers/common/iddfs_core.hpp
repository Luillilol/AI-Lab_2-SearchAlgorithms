// =============================================================================
//  iddfs_core.hpp  -  depth-limited search used by IDDFS (sequential and parallel)
//  Iterative Deepening DFS: depth limit 1, 2, 3, ... ; the first limit that finds
//  the solved cube gives a shortest solution (like BFS) but only needs O(depth) memory.
// =============================================================================
#pragma once
#include <atomic>
#include "driver.hpp"

namespace rc {

struct Dfs {
    const Ctl* ctl;
    std::atomic<bool>* stop = nullptr;   // set by other threads (solution found / time over)
    uint64_t nodes = 0;
    uint64_t tick = 0;
    bool timedOut = false, stopped = false;
    std::vector<int> path;

    // looks for the solved cube exactly `left` moves from c
    bool run(const Cube& c, int left, int lastFace) {
        Cube n;
        if (left == 1) {
            for (int m = 0; m < NMOVES; m++) {
                if (!allowedAfter(lastFace, m / 3)) continue;
                applyMove(c, m, n);
                nodes++;
                if (isSolved(n)) { path.push_back(m); return true; }
            }
            return false;
        }
        for (int m = 0; m < NMOVES; m++) {
            int f = m / 3;
            if (!allowedAfter(lastFace, f)) continue;
            applyMove(c, m, n);
            nodes++;
            if ((++tick & 0x3FFF) == 0) {
                if (ctl->expired()) { timedOut = true; return false; }
                if (stop && stop->load(std::memory_order_relaxed)) { stopped = true; return false; }
            }
            path.push_back(m);
            if (run(n, left - 1, f)) return true;
            path.pop_back();
            if (timedOut || stopped) return false;
        }
        return false;
    }
};

}  // namespace rc
