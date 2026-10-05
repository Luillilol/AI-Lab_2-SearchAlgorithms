// =============================================================================
//  astar_core.hpp  -  shared pieces of A*
//  f(n) = g(n) + h(n).  g = moves done so far, h = pattern-database heuristic.
//  Because every move costs 1, f is a small integer, so the "open list" is an array
//  of buckets indexed by f (instead of a heap): pushing and popping is O(1).
//  The heuristic is consistent, so a state is final the first time it is expanded.
// =============================================================================
#pragma once
#include "driver.hpp"

namespace rc {

constexpr int FMAX = 24;   // no cube needs more than 20 moves, so f > 24 can never be useful

struct ANode {
    Key key;
    uint32_t parent;
    uint8_t g, h, move, pad;
};

inline std::vector<int> aPath(Sharded<ANode>& tab, uint32_t gid) {
    std::vector<int> mv;
    for (; tab.node(gid).parent != NONE32; gid = tab.node(gid).parent) mv.push_back(tab.node(gid).move);
    std::reverse(mv.begin(), mv.end());
    return mv;
}

}  // namespace rc
