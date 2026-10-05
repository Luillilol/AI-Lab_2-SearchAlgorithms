// Bidirectional search (parallel version, OpenMP)
// Each level is expanded in chunks with two parallel phases:
//   phase 1: threads generate the successors of the frontier (read-only on the tables)
//   phase 2: every hash-table shard is updated by exactly one thread (no locks)
#include <omp.h>
#include "common/bidir_core.hpp"
using namespace rc;

struct Rec { Key key; uint32_t parent; uint8_t move, depth; uint16_t pad; };

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    SolveOut out;
    const int T = cfg.threads;
    int sb = 0;
    while ((1 << sb) < 2 * T && sb < 6) sb++;
    if (sb == 0) sb = 1;
    const size_t S = (size_t)1 << sb;
    Sharded<BNode> F(sb), B(sb);
    auto finish = [&]() { out.extra.push_back({"nodes_stored", (double)(F.count() + B.count())}); };
    if (isSolved(start)) { out.status = Status::SOLVED; finish(); return out; }
    Sharded<BNode>* TAB[2] = {&F, &B};
    Key sk = pack(start), gk = pack(solvedCube());
    std::vector<uint32_t> fr[2];
    {
        uint64_t h = hashKey(sk); size_t s = F.shardOf(h);
        fr[0].push_back(F.gid(s, F.sh[s]->insert(BNode{sk, NONE32, 255, 0, 0}, h)));
        h = hashKey(gk); s = B.shardOf(h);
        fr[1].push_back(B.gid(s, B.sh[s]->insert(BNode{gk, NONE32, 255, 0, 0}, h)));
    }
    // chunk size: keeps the temporary buffers small
    size_t CH = (size_t)1 << 21;
    {
        size_t byMem = ctl.memLimit / 8000;
        if (byMem < CH) CH = std::max<size_t>(byMem, 4096);
    }
    std::vector<std::vector<Rec>> buf((size_t)T * S);
    int depth[2] = {0, 0};

    while (true) {
        int side = fr[0].size() <= fr[1].size() ? 0 : 1;
        if (fr[side].empty()) { out.status = Status::NO_SOLUTION; break; }
        Sharded<BNode>& own = *TAB[side];
        Sharded<BNode>& oth = *TAB[1 - side];
        std::vector<std::vector<uint32_t>> nextS(S);
        bool met = false;
        uint32_t metOwn = 0, metOth = 0;
        const uint8_t newDepth = (uint8_t)(depth[side] + 1);

        for (size_t off = 0; off < fr[side].size() && !met; off += CH) {
            const size_t end = std::min(off + CH, fr[side].size());
            size_t nextCount = 0;
            for (auto& v : nextS) nextCount += v.size();
            size_t used = F.bytes() + B.bytes() + (fr[0].size() + fr[1].size() + nextCount) * 4 + CH * NMOVES * (sizeof(Rec) + 40);
            if (ctl.expired()) { out.status = Status::TIMEOUT; finish(); return out; }
            if (used > ctl.memLimit || own.idSpaceNearlyFull()) { out.status = Status::OUT_OF_MEMORY; finish(); return out; }

            uint64_t gen = 0, expd = 0;
            // ---- phase 1: generate successors
#pragma omp parallel for schedule(dynamic, 256) reduction(+ : gen, expd) num_threads(T)
            for (long i = (long)off; i < (long)end; i++) {
                const int tid = omp_get_thread_num();
                uint32_t id = fr[side][i];
                BNode nd = own.node(id);
                Cube c = unpack(nd.key);
                int lastFace = nd.move == 255 ? -1 : nd.move / 3;
                expd++;
                for (int m = 0; m < NMOVES; m++) {
                    if (m / 3 == lastFace) continue;
                    Cube n;
                    applyMove(c, m, n);
                    Key k = pack(n);
                    uint64_t h = hashKey(k);
                    gen++;
                    size_t s = own.shardOf(h);
                    if (own.sh[s]->find(k, h) != NONE32) continue;
                    buf[(size_t)tid * S + s].push_back(Rec{k, id, (uint8_t)m, newDepth, 0});
                }
            }
            out.expanded += expd; out.generated += gen;
            // ---- phase 2: every shard is owned by one thread
#pragma omp parallel for schedule(dynamic, 1) num_threads(T)
            for (long s = 0; s < (long)S; s++) {
                Shard<BNode>& sh = *own.sh[s];
                for (int t = 0; t < T; t++) {
                    auto& b = buf[(size_t)t * S + s];
                    for (const Rec& r : b) {
                        uint64_t h = hashKey(r.key);
                        if (sh.find(r.key, h) != NONE32) continue;
                        uint32_t local = sh.insert(BNode{r.key, r.parent, r.move, r.depth, 0}, h);
                        uint32_t gid = own.gid((size_t)s, local);
                        nextS[s].push_back(gid);
                        size_t so = oth.shardOf(h);
                        uint32_t o = oth.sh[so]->find(r.key, h);
                        if (o != NONE32) {
#pragma omp critical(meet)
                            {
                                if (!met) { met = true; metOwn = gid; metOth = oth.gid(so, o); }
                            }
                        }
                    }
                    b.clear();
                }
            }
        }
        if (met) {
            out.moves = side == 0 ? buildPath(F, metOwn, B, metOth) : buildPath(F, metOth, B, metOwn);
            out.status = Status::SOLVED;
            finish();
            return out;
        }
        std::vector<uint32_t> nf;
        size_t total = 0;
        for (auto& v : nextS) total += v.size();
        nf.reserve(total);
        for (auto& v : nextS) nf.insert(nf.end(), v.begin(), v.end());
        fr[side].swap(nf);
        depth[side]++;
    }
    finish();
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"Bidirectional", "parallel", true, false, solve};
    return runMain(argc, argv, info);
}
