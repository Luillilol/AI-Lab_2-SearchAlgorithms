// IDDFS - Iterative Deepening Depth-First Search (sequential version)
#include "common/iddfs_core.hpp"
using namespace rc;

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    SolveOut out;
    int completed = 0;
    if (isSolved(start)) { out.status = Status::SOLVED; out.extra.push_back({"depth_completed", 0}); return out; }
    for (int limit = 1; limit <= cfg.maxDepth; limit++) {
        Dfs d;
        d.ctl = &ctl;
        bool found = d.run(start, limit, -1);
        out.expanded += d.nodes;
        out.generated += d.nodes;
        if (found) { out.status = Status::SOLVED; out.moves = d.path; break; }
        if (d.timedOut || ctl.expired()) { out.status = Status::TIMEOUT; break; }
        completed = limit;
    }
    out.extra.push_back({"depth_completed", (double)completed});
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"IDDFS", "sequential", false, false, solve};
    return runMain(argc, argv, info);
}
