// Measures how good the heuristics are: average h over random cubes.
//   old  = first version of the project (5 tables of 4 cubies: 2 of corners, 3 of edges), rebuilt for the 18 moves
//   korf = 8 corners + 2 x 6 edges (the heuristic used by astar_seq / astar_par)
// Build:  g++ -O3 -std=c++17 -fopenmp -I.. tools/compare_heuristics.cpp -o compare_heuristics
#include "../common/driver.hpp"
using namespace rc;

// PDB of 4 corner cubies (cubies c0..c3): state = positions (3 bits each) + twists (2 bits each) -> 20 bits
struct Corner4 {
    int ids[4]; int jOf[8]; std::vector<uint8_t> dist;
    uint32_t index(const Cube& c) const {
        uint32_t pos = 0, ori = 0;
        for (int s = 0; s < 8; s++) { int j = jOf[c.cp[s]]; if (j >= 0) { pos |= (uint32_t)s << (3 * j); ori |= (uint32_t)c.co[s] << (2 * j); } }
        return pos | (ori << 12);
    }
    void build(int first) {
        for (int i = 0; i < 8; i++) jOf[i] = -1;
        for (int i = 0; i < 4; i++) { ids[i] = first + i; jOf[first + i] = i; }
        dist.assign(1u << 20, 255);
        uint32_t start = 0;
        for (int j = 0; j < 4; j++) start |= (uint32_t)ids[j] << (3 * j);
        dist[start] = 0;
        std::vector<uint32_t> fr{start};
        for (int d = 0; !fr.empty(); d++) {
            std::vector<uint32_t> nx;
            for (uint32_t st : fr)
                for (int m = 0; m < NMOVES; m++) {
                    uint32_t np = 0, no = 0;
                    for (int j = 0; j < 4; j++) {
                        int s = (st >> (3 * j)) & 7, o = (st >> (12 + 2 * j)) & 3;
                        int t = -1;
                        for (int x = 0; x < 8; x++) if (G.csrc[m][x] == s) t = x;
                        np |= (uint32_t)t << (3 * j);
                        no |= (uint32_t)((o + G.cdel[m][t]) % 3) << (12 + 2 * j);
                    }
                    uint32_t n = np | no;
                    if (dist[n] == 255) { dist[n] = (uint8_t)(d + 1); nx.push_back(n); }
                }
            fr.swap(nx);
        }
    }
    int get(const Cube& c) const { return dist[index(c)]; }
};

int main(int argc, char** argv) {
    int N = argc > 1 ? std::atoi(argv[1]) : 20000;
    Heuristic korf;
    korf.init(6, "pdb_cache", false);
    Corner4 c1, c2;
    c1.build(0); c2.build(4);
    EdgePDB e1, e2, e3;
    e1.init({0, 1, 2, 3}, "/tmp/pdb_old", false);
    e2.init({4, 5, 6, 7}, "/tmp/pdb_old", false);
    e3.init({8, 9, 10, 11}, "/tmp/pdb_old", false);
    auto oldH = [&](const Cube& c) { return std::max({c1.get(c), c2.get(c), e1.get(c), e2.get(c), e3.get(c)}); };
    uint64_t seed = 123;
    std::printf("%8s %10s %10s\n", "depth", "avg h old", "avg h korf");
    for (int depth : {5, 8, 10, 12, 14, 18, 30}) {
        double so = 0, sk = 0;
        for (int i = 0; i < N; i++) {
            Cube c = applyMoves(solvedCube(), randomScramble(depth, splitmix64(seed)));
            so += oldH(c); sk += korf.eval(c);
        }
        std::printf("%8d %10.3f %10.3f\n", depth, so / N, sk / N);
    }
    std::printf("table sizes: old = 2*%u + 3*%u = %u entries ; korf = %llu entries\n", 136080u, 190080u, 2 * 136080u + 3 * 190080u,
                (unsigned long long)(korf.corners.info.entries + korf.edgesA.info.entries + korf.edgesB.info.entries));
    return 0;
}
