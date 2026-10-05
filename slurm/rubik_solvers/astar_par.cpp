// A* search with the Korf pattern-database heuristic (parallel version, OpenMP)
// Nodes with the smallest f are expanded together (they can be expanded in any order):
//   1. select a chunk of nodes with minimum f (each shard of the hash table is owned by one thread)
//   2. phase 1: threads generate the successors and evaluate the heuristic (the expensive part)
//   3. phase 2: each shard inserts / updates its own nodes, no locks needed
// The search stops when the solved cube is selected, so the solution is optimal (same as sequential A*).
#include <omp.h>
#include "common/astar_core.hpp"
using namespace rc;

struct Rec { Key key; uint32_t parent; uint8_t g, h, move, pad; };

static SolveOut solve(const Cube& start, const Config& cfg, const Ctl& ctl) {
    SolveOut out;
    const Heuristic& H = *HEUR;
    const int T = cfg.threads;
    const int h0 = H.eval(start);
    out.extra.push_back({"h_initial", (double)h0});
    if (isSolved(start)) { out.status = Status::SOLVED; out.extra.push_back({"nodes_stored", 1}); return out; }

    int sb = 0;
    while ((1 << sb) < 2 * T && sb < 6) sb++;
    if (sb == 0) sb = 1;
    const size_t S = (size_t)1 << sb;
    Sharded<ANode> tab(sb);
    auto finish = [&]() { out.extra.push_back({"nodes_stored", (double)tab.count()}); };
    std::vector<std::vector<std::vector<uint32_t>>> open(S, std::vector<std::vector<uint32_t>>(FMAX + 1));  // open[shard][f] = local ids
    const Key goalKey = pack(solvedCube());
    {
        Key sk = pack(start);
        uint64_t h = hashKey(sk);
        size_t s = tab.shardOf(h);
        uint32_t l = tab.sh[s]->insert(ANode{sk, NONE32, 0, (uint8_t)h0, 255, 0}, h);
        open[s][h0].push_back(l);
    }
    size_t CH = (size_t)1 << 21;
    {
        size_t byMem = ctl.memLimit / 8000;
        if (byMem < CH) CH = std::max<size_t>(byMem, 4096);
    }
    std::vector<std::vector<Rec>> buf((size_t)T * S);
    std::vector<std::vector<uint32_t>> sel(S);
    std::vector<uint32_t> flat;

    while (true) {
        // lowest f with open nodes
        int f = FMAX + 1;
        for (size_t s = 0; s < S; s++)
            for (int ff = 0; ff < f; ff++) if (!open[s][ff].empty()) { f = ff; break; }
        if (f > FMAX) { out.status = Status::NO_SOLUTION; break; }
        if (ctl.expired()) { out.status = Status::TIMEOUT; break; }
        {
            size_t openBytes = 0;
            for (auto& os : open) for (auto& v : os) openBytes += v.capacity() * 4;
            size_t used = tab.bytes() + openBytes + CH * NMOVES * (sizeof(Rec) + 40);
            if (used > ctl.memLimit || tab.idSpaceNearlyFull()) { out.status = Status::OUT_OF_MEMORY; break; }
        }
        // ---- select a chunk of nodes with f minimal
        bool goalSel = false;
        uint32_t goalGid = 0;
        const size_t cap = std::max<size_t>(1, CH / S);
#pragma omp parallel for schedule(dynamic, 1) num_threads(T)
        for (long s = 0; s < (long)S; s++) {
            auto& b = open[s][f];
            auto& o = sel[s];
            o.clear();
            while (!b.empty() && o.size() < cap) {
                uint32_t l = b.back();
                b.pop_back();
                const ANode& nd = tab.sh[s]->nodes[l];
                if (nd.g + nd.h != f) continue;   // old entry
                uint32_t gid = tab.gid((size_t)s, l);
                o.push_back(gid);
                if (nd.key == goalKey) {
#pragma omp critical(goal)
                    { goalSel = true; goalGid = gid; }
                }
            }
        }
        if (goalSel) { out.moves = aPath(tab, goalGid); out.status = Status::SOLVED; break; }
        flat.clear();
        for (auto& o : sel) flat.insert(flat.end(), o.begin(), o.end());
        if (flat.empty()) continue;

        // ---- phase 1: expand
        uint64_t gen = 0;
#pragma omp parallel for schedule(dynamic, 128) reduction(+ : gen) num_threads(T)
        for (long i = 0; i < (long)flat.size(); i++) {
            const int tid = omp_get_thread_num();
            const uint32_t gid = flat[i];
            const ANode nd = tab.node(gid);
            Cube c = unpack(nd.key);
            const int lastFace = nd.move == 255 ? -1 : nd.move / 3;
            const int ng = nd.g + 1;
            for (int m = 0; m < NMOVES; m++) {
                if (m / 3 == lastFace) continue;
                Cube n;
                applyMove(c, m, n);
                Key k = pack(n);
                uint64_t hh = hashKey(k);
                gen++;
                size_t s = tab.shardOf(hh);
                uint32_t e = tab.sh[s]->find(k, hh);
                int hv;
                if (e != NONE32) {
                    const ANode& en = tab.sh[s]->nodes[e];
                    if (en.g <= ng) continue;
                    hv = en.h;
                } else hv = H.eval(n);
                if (ng + hv > FMAX) continue;
                buf[(size_t)tid * S + s].push_back(Rec{k, gid, (uint8_t)ng, (uint8_t)hv, (uint8_t)m, 0});
            }
        }
        out.expanded += flat.size();
        out.generated += gen;

        // ---- phase 2: insert / update, one thread per shard
#pragma omp parallel for schedule(dynamic, 1) num_threads(T)
        for (long s = 0; s < (long)S; s++) {
            Shard<ANode>& sh = *tab.sh[s];
            for (int t = 0; t < T; t++) {
                auto& b = buf[(size_t)t * S + s];
                for (const Rec& r : b) {
                    uint64_t hh = hashKey(r.key);
                    uint32_t e = sh.find(r.key, hh);
                    if (e == NONE32) {
                        uint32_t l = sh.insert(ANode{r.key, r.parent, r.g, r.h, r.move, 0}, hh);
                        open[s][r.g + r.h].push_back(l);
                    } else if (r.g < sh.nodes[e].g) {
                        ANode& en = sh.nodes[e];
                        en.g = r.g; en.parent = r.parent; en.move = r.move;
                        open[s][r.g + en.h].push_back(e);
                    }
                }
                b.clear();
            }
        }
    }
    finish();
    return out;
}

int main(int argc, char** argv) {
    static AlgoInfo info{"A*", "parallel", true, true, solve};
    return runMain(argc, argv, info);
}
