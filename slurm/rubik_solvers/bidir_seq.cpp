// Bidirectional search (sequential version)
#include "common/bidir_core.hpp"
using namespace rc;

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    (void)cfg;
    SolveOut out;
    auto finish = [&](Sharded<BNode>& F, Sharded<BNode>& B) {
        out.extra.push_back({"nodes_stored", (double)(F.count() + B.count())});
    };
    Sharded<BNode> F(0), B(0);
    if (isSolved(start)) { out.status = Status::SOLVED; finish(F, B); return out; }
    Sharded<BNode>* T[2] = {&F, &B};
    Key sk = pack(start), gk = pack(solvedCube());
    std::vector<uint32_t> fr[2];
    fr[0].push_back(F.sh[0]->insert(BNode{sk, NONE32, 255, 0, 0}, hashKey(sk)));
    fr[1].push_back(B.sh[0]->insert(BNode{gk, NONE32, 255, 0, 0}, hashKey(gk)));
    uint64_t tick = 0;

    while (true) {
        int side = fr[0].size() <= fr[1].size() ? 0 : 1;
        if (fr[side].empty()) { out.status = Status::NO_SOLUTION; break; }
        Shard<BNode>& own = *T[side]->sh[0];
        Shard<BNode>& oth = *T[1 - side]->sh[0];
        std::vector<uint32_t> nxt;
        for (uint32_t id : fr[side]) {
            BNode nd = own.nodes[id];
            Cube c = unpack(nd.key);
            int lastFace = nd.move == 255 ? -1 : nd.move / 3;
            out.expanded++;
            for (int m = 0; m < NMOVES; m++) {
                if (m / 3 == lastFace) continue;
                Cube n;
                applyMove(c, m, n);
                Key k = pack(n);
                uint64_t h = hashKey(k);
                out.generated++;
                if (own.find(k, h) != NONE32) continue;
                uint32_t nid = own.insert(BNode{k, id, (uint8_t)m, (uint8_t)(nd.depth + 1), 0}, h);
                nxt.push_back(nid);
                uint32_t o = oth.find(k, h);
                if (o != NONE32) {   // the two searches met
                    out.moves = side == 0 ? buildPath(F, nid, B, o) : buildPath(F, o, B, nid);
                    out.status = Status::SOLVED;
                    finish(F, B);
                    return out;
                }
            }
            if ((++tick & 1023) == 0) {
                size_t used = F.bytes() + B.bytes() + (nxt.capacity() + fr[0].capacity() + fr[1].capacity()) * 4;
                if (ctl.expired()) { out.status = Status::TIMEOUT; finish(F, B); return out; }
                if (used > ctl.memLimit || T[side]->idSpaceNearlyFull()) { out.status = Status::OUT_OF_MEMORY; finish(F, B); return out; }
            }
        }
        fr[side].swap(nxt);
    }
    finish(F, B);
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"Bidirectional", "sequential", false, false, solve};
    return runMain(argc, argv, info);
}
