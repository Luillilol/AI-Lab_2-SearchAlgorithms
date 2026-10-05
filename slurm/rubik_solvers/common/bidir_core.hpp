// =============================================================================
//  bidir_core.hpp  -  shared pieces of the bidirectional search
//  Two breadth-first searches: one from the scrambled cube (forward) and one from
//  the solved cube (backward). Every level is finished before switching sides, and
//  the side with the smaller frontier is expanded. The first time a state appears in
//  BOTH searches the path is complete, and it is a shortest one (the two searches
//  have visited all the states up to their depth, so no shorter meeting exists).
//  Instead of storing a whole path per state (like the Python version) every state
//  stores only (parent, move, depth): 24 bytes per state.
// =============================================================================
#pragma once
#include "driver.hpp"

namespace rc {

struct BNode {
    Key key;
    uint32_t parent;
    uint8_t move, depth;
    uint16_t pad;
};

// forward chain: start -> meeting state ; backward chain: meeting state -> solved (inverse moves)
inline std::vector<int> buildPath(Sharded<BNode>& fwd, uint32_t fid, Sharded<BNode>& bwd, uint32_t bid) {
    std::vector<int> a;
    for (uint32_t id = fid; fwd.node(id).parent != NONE32; id = fwd.node(id).parent) a.push_back(fwd.node(id).move);
    std::reverse(a.begin(), a.end());
    for (uint32_t id = bid; bwd.node(id).parent != NONE32; id = bwd.node(id).parent) a.push_back(moveInv(bwd.node(id).move));
    return a;
}

}  // namespace rc
