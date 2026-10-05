// A* search with the Korf pattern-database heuristic (sequential version)
#include "common/astar_core.hpp"
using namespace rc;

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    (void)cfg;
    SolveOut out;
    const Heuristic& H = *HEUR;
    const int h0 = H.eval(start);
    out.extra.push_back({"h_initial", (double)h0});
    if (isSolved(start)) { out.status = Status::SOLVED; out.extra.push_back({"nodes_stored", 1}); return out; }

    Sharded<ANode> tab(0);
    Shard<ANode>& T = *tab.sh[0];
    std::vector<std::vector<uint32_t>> open(FMAX + 1);
    const Key goalKey = pack(solvedCube());
    Key sk = pack(start);
    open[h0].push_back(T.insert(ANode{sk, NONE32, 0, (uint8_t)h0, 255, 0}, hashKey(sk)));
    int curF = h0;
    uint64_t tick = 0;
    auto finish = [&]() { out.extra.push_back({"nodes_stored", (double)T.nodes.size()}); };

    while (true) {
        while (curF <= FMAX && open[curF].empty()) curF++;
        if (curF > FMAX) { out.status = Status::NO_SOLUTION; break; }
        uint32_t id = open[curF].back();     // LIFO inside the same f: prefers deeper nodes
        open[curF].pop_back();
        ANode nd = T.nodes[id];
        if (nd.g + nd.h != curF) continue;   // old entry (the node was reached again with a smaller g)
        if (nd.key == goalKey) { out.moves = aPath(tab, id); out.status = Status::SOLVED; break; }

        Cube c = unpack(nd.key);
        const int lastFace = nd.move == 255 ? -1 : nd.move / 3;
        out.expanded++;
        for (int m = 0; m < NMOVES; m++) {
            if (m / 3 == lastFace) continue;          // never turn the same face twice in a row
            Cube n;
            applyMove(c, m, n);
            Key k = pack(n);
            uint64_t hh = hashKey(k);
            const int ng = nd.g + 1;
            out.generated++;
            uint32_t e = T.find(k, hh);
            if (e == NONE32) {
                int hv = H.eval(n);
                if (ng + hv > FMAX) continue;
                uint32_t nid = T.insert(ANode{k, id, (uint8_t)ng, (uint8_t)hv, (uint8_t)m, 0}, hh);
                open[ng + hv].push_back(nid);
            } else if (ng < T.nodes[e].g) {           // found a shorter way to a known state
                ANode& en = T.nodes[e];
                en.g = (uint8_t)ng; en.parent = id; en.move = (uint8_t)m;
                int f = ng + en.h;
                open[f].push_back(e);
                if (f < curF) curF = f;
            }
        }
        if ((++tick & 1023) == 0) {
            if (ctl.expired()) { out.status = Status::TIMEOUT; break; }
            size_t openBytes = 0;
            for (auto& v : open) openBytes += v.capacity() * 4;
            if (T.bytes() + openBytes > ctl.memLimit || tab.idSpaceNearlyFull()) { out.status = Status::OUT_OF_MEMORY; break; }
        }
    }
    finish();
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"A*", "sequential", false, true, solve};
    return runMain(argc, argv, info);
}
