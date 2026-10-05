// IDDFS - Iterative Deepening Depth-First Search (parallel version, OpenMP)
// Every depth limit is split in independent tasks: one per sequence of the first 3 moves.
#include <omp.h>
#include "common/iddfs_core.hpp"
using namespace rc;

struct Task { Cube c; int lastFace; int mv[3]; };

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    SolveOut out;
    int completed = 0;
    if (isSolved(start)) { out.status = Status::SOLVED; out.extra.push_back({"depth_completed", 0}); return out; }

    // all the (non redundant) sequences of 3 moves
    std::vector<Task> tasks;
    for (int a = 0; a < NMOVES; a++) {
        Cube c1; applyMove(start, a, c1);
        for (int b = 0; b < NMOVES; b++) {
            if (!allowedAfter(a / 3, b / 3)) continue;
            Cube c2; applyMove(c1, b, c2);
            for (int c = 0; c < NMOVES; c++) {
                if (!allowedAfter(b / 3, c / 3)) continue;
                Task t; applyMove(c2, c, t.c);
                t.lastFace = c / 3; t.mv[0] = a; t.mv[1] = b; t.mv[2] = c;
                tasks.push_back(t);
            }
        }
    }

    for (int limit = 1; limit <= cfg.maxDepth; limit++) {
        if (limit <= 3) {   // tiny: no need for threads
            Dfs d; d.ctl = &ctl;
            bool found = d.run(start, limit, -1);
            out.expanded += d.nodes; out.generated += d.nodes;
            if (found) { out.status = Status::SOLVED; out.moves = d.path; break; }
            completed = limit;
            continue;
        }
        std::atomic<bool> stop{false};
        bool found = false, timedOut = false;
        std::vector<int> solution;
        uint64_t total = 0;
#pragma omp parallel for schedule(dynamic, 1) reduction(+ : total) num_threads(cfg.threads)
        for (long i = 0; i < (long)tasks.size(); i++) {
            if (stop.load(std::memory_order_relaxed)) continue;
            Dfs d; d.ctl = &ctl; d.stop = &stop;
            bool ok = d.run(tasks[i].c, limit - 3, tasks[i].lastFace);
            total += d.nodes;
            if (ok) {
#pragma omp critical(sol)
                {
                    if (!found) {
                        found = true;
                        solution.assign(tasks[i].mv, tasks[i].mv + 3);
                        solution.insert(solution.end(), d.path.begin(), d.path.end());
                    }
                }
                stop.store(true);
            } else if (d.timedOut) {
#pragma omp critical(sol)
                timedOut = true;
                stop.store(true);
            }
        }
        out.expanded += total; out.generated += total;
        if (found) { out.status = Status::SOLVED; out.moves = solution; break; }   // any solution found at this limit is a shortest one
        if (timedOut || ctl.expired()) { out.status = Status::TIMEOUT; break; }
        completed = limit;
    }
    out.extra.push_back({"depth_completed", (double)completed});
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"IDDFS", "parallel", true, false, solve};
    return runMain(argc, argv, info);
}
