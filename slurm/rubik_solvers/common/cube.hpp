// =============================================================================
//  cube.hpp  -  Rubik's Cube model used by ALL the solvers
// -----------------------------------------------------------------------------
//  * The cube is stored by cubies (8 corners + 12 edges) instead of 54 stickers,
//    because a move is then only 20 table look-ups.
//  * The 54-sticker layout and the 12 quarter-turn permutations are EXACTLY the
//    ones of the Python version (aStar.py / Bidirectional.py). All the cubie
//    tables are derived from them at start-up, so both programs agree.
//  * 18 moves (half-turn metric):  U U2 U'  D D2 D'  L L2 L'  R R2 R'  F F2 F'  B B2 B'
//        move index m = face*3 + k   (face order U D L R F B,  k: 0 = turn, 1 = half turn, 2 = inverse)
// =============================================================================
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace rc {

constexpr int NMOVES = 18;
inline const char FACE_LETTER[7] = "UDLRFB";

inline int moveFace(int m) { return m / 3; }
inline int moveInv(int m) { return (m / 3) * 3 + (2 - m % 3); }
inline std::string moveName(int m) {
    std::string s(1, FACE_LETTER[m / 3]);
    if (m % 3 == 1) s += '2';
    else if (m % 3 == 2) s += '\'';
    return s;
}
// Redundant-move pruning used by the tree search (IDDFS): never turn the same face twice
// in a row, and for opposite faces (which commute) only allow one order.
inline bool allowedAfter(int lastFace, int face) {
    if (lastFace < 0) return true;
    if (face == lastFace) return false;
    if (face == (lastFace ^ 1) && face < lastFace) return false;
    return true;
}

// ----------------------------------------------------------------------------- cube state
struct Cube {
    uint8_t cp[8];   // cp[slot] = which corner cubie is in this slot
    uint8_t co[8];   // twist of that cubie (0,1,2)
    uint8_t ep[12];  // ep[slot] = which edge cubie is in this slot
    uint8_t eo[12];  // flip of that cubie (0,1)
};

inline Cube solvedCube() {
    Cube c;
    for (int i = 0; i < 8; i++) { c.cp[i] = (uint8_t)i; c.co[i] = 0; }
    for (int i = 0; i < 12; i++) { c.ep[i] = (uint8_t)i; c.eo[i] = 0; }
    return c;
}
inline bool sameCube(const Cube& a, const Cube& b) { return std::memcmp(&a, &b, sizeof(Cube)) == 0; }

// 128-bit packed key (a state is identified by 16 bytes: used in hash tables)
struct Key {
    uint64_t a, b;
    bool operator==(const Key& o) const { return a == o.a && b == o.b; }
};
inline Key pack(const Cube& c) {
    uint64_t a = 0, b = 0, eo = 0, co = 0;
    for (int i = 0; i < 12; i++) a |= (uint64_t)c.ep[i] << (4 * i);
    for (int i = 0; i < 11; i++) eo |= (uint64_t)c.eo[i] << i;  // last flip is implied
    a |= eo << 48;
    for (int i = 0; i < 8; i++) b |= (uint64_t)c.cp[i] << (3 * i);
    for (int i = 0; i < 7; i++) co |= (uint64_t)c.co[i] << (2 * i);  // last twist is implied
    b |= co << 24;
    return Key{a, b};
}
inline Cube unpack(const Key& k) {
    Cube c;
    int se = 0, sc = 0;
    for (int i = 0; i < 12; i++) c.ep[i] = (uint8_t)((k.a >> (4 * i)) & 15);
    for (int i = 0; i < 11; i++) { c.eo[i] = (uint8_t)((k.a >> (48 + i)) & 1); se ^= c.eo[i]; }
    c.eo[11] = (uint8_t)se;
    for (int i = 0; i < 8; i++) c.cp[i] = (uint8_t)((k.b >> (3 * i)) & 7);
    for (int i = 0; i < 7; i++) { c.co[i] = (uint8_t)((k.b >> (24 + 2 * i)) & 3); sc += c.co[i]; }
    c.co[7] = (uint8_t)((3 - sc % 3) % 3);
    return c;
}

// ----------------------------------------------------------------------------- tables
// Sticker layout (same as the Python code): 6 faces x 9 stickers, in blocks
//   0-8 Blue (this is the L face)   9-17 Red (U)   18-26 White (F)
//  27-35 Orange (D)   36-44 Green (R)   45-53 Yellow (B)
struct Tables {
    int perm[NMOVES][54];  // sticker model: new[i] = old[perm[m][i]]
    int inv[NMOVES][54];   // inverse permutation
    char goal[54];
    int cst[8][3];         // sticker positions of every corner slot (ordered, [0] = reference sticker)
    int est[12][2];        // sticker positions of every edge slot   (ordered, [0] = reference sticker)
    int cornerSlotOf[54], edgeSlotOf[54];
    char chc[8][3];        // home colours of every corner cubie, same order as cst
    char ehc[12][2];
    uint8_t csrc[NMOVES][8], cdel[NMOVES][8];    // new_cp[t] = cp[csrc[m][t]], new_co[t] = co[csrc]+cdel (mod 3)
    uint8_t esrc[NMOVES][12], edel[NMOVES][12];  // same for edges (flip = xor)

    Tables();
    uint64_t layoutHash() const {  // identifies the move tables (used to validate the PDB cache files)
        uint64_t h = 1469598103934665603ULL;
        auto mix = [&](uint8_t v) { h ^= v; h *= 1099511628211ULL; };
        for (int m = 0; m < NMOVES; m++) {
            for (int i = 0; i < 8; i++) { mix(csrc[m][i]); mix(cdel[m][i]); }
            for (int i = 0; i < 12; i++) { mix(esrc[m][i]); mix(edel[m][i]); }
        }
        return h;
    }
};

inline Tables::Tables() {
    static const int BASE[12][54] = {
        {0, 1, 27, 3, 4, 28, 6, 7, 29, 9, 10, 11, 12, 13, 14, 8, 5, 2, 24, 21, 18, 25, 22, 19, 26, 23, 20, 42, 39, 36, 30, 31, 32, 33, 34, 35, 15, 37, 38, 16, 40, 41, 17, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53},   // F
        {0, 1, 17, 3, 4, 16, 6, 7, 15, 9, 10, 11, 12, 13, 14, 36, 39, 42, 20, 23, 26, 19, 22, 25, 18, 21, 24, 2, 5, 8, 30, 31, 32, 33, 34, 35, 29, 37, 38, 28, 40, 41, 27, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53},   // F'
        {11, 1, 2, 10, 4, 5, 9, 7, 8, 38, 41, 44, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 0, 3, 6, 36, 37, 35, 39, 40, 34, 42, 43, 33, 51, 48, 45, 52, 49, 46, 53, 50, 47},  // B
        {33, 1, 2, 34, 4, 5, 35, 7, 8, 6, 3, 0, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 44, 41, 38, 36, 37, 9, 39, 40, 10, 42, 43, 11, 47, 50, 53, 46, 49, 52, 45, 48, 51},  // B'
        {6, 3, 0, 7, 4, 1, 8, 5, 2, 53, 10, 11, 50, 13, 14, 47, 16, 17, 9, 19, 20, 12, 22, 23, 15, 25, 26, 18, 28, 29, 21, 31, 32, 24, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 33, 48, 49, 30, 51, 52, 27},   // L
        {2, 5, 8, 1, 4, 7, 0, 3, 6, 18, 10, 11, 21, 13, 14, 24, 16, 17, 27, 19, 20, 30, 22, 23, 33, 25, 26, 53, 28, 29, 50, 31, 32, 47, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 15, 48, 49, 12, 51, 52, 9},   // L'
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 12, 13, 23, 15, 16, 26, 18, 19, 29, 21, 22, 32, 24, 25, 35, 27, 28, 51, 30, 31, 48, 33, 34, 45, 42, 39, 36, 43, 40, 37, 44, 41, 38, 17, 46, 47, 14, 49, 50, 11, 52, 53},  // R
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 51, 12, 13, 48, 15, 16, 45, 18, 19, 11, 21, 22, 14, 24, 25, 17, 27, 28, 20, 30, 31, 23, 33, 34, 26, 38, 41, 44, 37, 40, 43, 36, 39, 42, 35, 46, 47, 32, 49, 50, 29, 52, 53},  // R'
        {18, 19, 20, 3, 4, 5, 6, 7, 8, 15, 12, 9, 16, 13, 10, 17, 14, 11, 36, 37, 38, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 45, 46, 47, 39, 40, 41, 42, 43, 44, 0, 1, 2, 48, 49, 50, 51, 52, 53},  // U
        {45, 46, 47, 3, 4, 5, 6, 7, 8, 11, 14, 17, 10, 13, 16, 9, 12, 15, 0, 1, 2, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 18, 19, 20, 39, 40, 41, 42, 43, 44, 36, 37, 38, 48, 49, 50, 51, 52, 53},  // U'
        {0, 1, 2, 3, 4, 5, 51, 52, 53, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 6, 7, 8, 33, 30, 27, 34, 31, 28, 35, 32, 29, 36, 37, 38, 39, 40, 41, 24, 25, 26, 45, 46, 47, 48, 49, 50, 42, 43, 44},  // D
        {0, 1, 2, 3, 4, 5, 24, 25, 26, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 42, 43, 44, 29, 32, 35, 28, 31, 34, 27, 30, 33, 36, 37, 38, 39, 40, 41, 51, 52, 53, 45, 46, 47, 48, 49, 50, 6, 7, 8},  // D'
    };
    // my face order U D L R F B  ->  index of the letter in BASE (F,B,L,R,U,D) and sticker block of that face
    const int LETTER[6] = {4, 5, 2, 3, 0, 1};
    const int BLOCK[6] = {1, 3, 0, 4, 2, 5};
    auto fail = [](const char* msg) { std::fprintf(stderr, "Tables: %s\n", msg); std::exit(2); };

    for (int f = 0; f < 6; f++) {
        const int* q = BASE[2 * LETTER[f]];
        const int* qi = BASE[2 * LETTER[f] + 1];
        for (int i = 0; i < 54; i++) {
            perm[f * 3 + 0][i] = q[i];
            perm[f * 3 + 1][i] = q[q[i]];
            perm[f * 3 + 2][i] = qi[i];
        }
    }
    for (int m = 0; m < NMOVES; m++)
        for (int i = 0; i < 54; i++) inv[m][perm[m][i]] = i;
    const char COL[7] = "BRWOGY";
    for (int i = 0; i < 54; i++) goal[i] = COL[i / 9];

    // group stickers by cubie: a sticker belongs to the cubie defined by the set of faces that move it
    int mask[54] = {0};
    for (int f = 0; f < 6; f++)
        for (int i = 0; i < 54; i++)
            if (perm[f * 3][i] != i) mask[i] |= 1 << f;
    std::map<int, std::vector<int>> cg, eg;
    for (int i = 0; i < 54; i++) {
        int pc = __builtin_popcount(mask[i]);
        if (pc == 3) cg[mask[i]].push_back(i);
        else if (pc == 2) eg[mask[i]].push_back(i);
    }
    if (cg.size() != 8 || eg.size() != 12) fail("cubie grouping failed");
    for (int i = 0; i < 54; i++) cornerSlotOf[i] = edgeSlotOf[i] = -1;
    int nc = 0, ne = 0;
    for (auto& kv : cg) { for (int j = 0; j < 3; j++) cst[nc][j] = kv.second[j]; nc++; }
    for (auto& kv : eg) { for (int j = 0; j < 2; j++) est[ne][j] = kv.second[j]; ne++; }

    // reference sticker first. Corners: the sticker on the U or D face. Edges: on U/D, else on F/B.
    for (int s = 0; s < 8; s++) {
        int r = -1;
        for (int j = 0; j < 3; j++) { int b = cst[s][j] / 9; if (b == BLOCK[0] || b == BLOCK[1]) r = j; }
        if (r < 0) fail("corner without U/D sticker");
        std::swap(cst[s][0], cst[s][r]);
    }
    for (int s = 0; s < 12; s++) {
        int r = -1;
        for (int j = 0; j < 2; j++) { int b = est[s][j] / 9; if (b == BLOCK[0] || b == BLOCK[1]) r = j; }
        if (r < 0)
            for (int j = 0; j < 2; j++) { int b = est[s][j] / 9; if (b == BLOCK[4] || b == BLOCK[5]) r = j; }
        if (r < 0) fail("edge without reference sticker");
        std::swap(est[s][0], est[s][r]);
    }
    // fix the clockwise order of the 2nd/3rd sticker of every corner by propagating through the moves
    {
        auto slotOfPos = [&](int p) {  // temporary mapping position -> corner slot (before final order)
            for (int s = 0; s < 8; s++) for (int j = 0; j < 3; j++) if (cst[s][j] == p) return s;
            return -1;
        };
        int ord[8][3];
        bool done[8] = {false};
        for (int j = 0; j < 3; j++) ord[0][j] = cst[0][j];
        done[0] = true;
        std::vector<int> q = {0};
        for (size_t qi = 0; qi < q.size(); qi++) {
            int s = q[qi];
            for (int m = 0; m < NMOVES; m++) {
                int to[3];
                for (int j = 0; j < 3; j++) to[j] = inv[m][ord[s][j]];
                int t = slotOfPos(to[0]);
                if (t == s || done[t]) continue;
                int jr = -1;
                for (int j = 0; j < 3; j++) if (to[j] == cst[t][0]) jr = j;
                for (int j = 0; j < 3; j++) ord[t][j] = to[(jr + j) % 3];
                done[t] = true;
                q.push_back(t);
            }
        }
        for (int s = 0; s < 8; s++) {
            if (!done[s]) fail("corner order propagation failed");
            for (int j = 0; j < 3; j++) cst[s][j] = ord[s][j];
        }
    }
    for (int s = 0; s < 8; s++) for (int j = 0; j < 3; j++) cornerSlotOf[cst[s][j]] = s;
    for (int s = 0; s < 12; s++) for (int j = 0; j < 2; j++) edgeSlotOf[est[s][j]] = s;
    for (int s = 0; s < 8; s++) for (int j = 0; j < 3; j++) chc[s][j] = goal[cst[s][j]];
    for (int s = 0; s < 12; s++) for (int j = 0; j < 2; j++) ehc[s][j] = goal[est[s][j]];

    // cubie-level move tables (and a consistency check of the orientation convention)
    for (int m = 0; m < NMOVES; m++) {
        for (int t = 0; t < 8; t++) {
            int s = cornerSlotOf[perm[m][cst[t][0]]];
            csrc[m][t] = (uint8_t)s;
            int d = -1;
            for (int o = 0; o < 3; o++) {
                int q = inv[m][cst[s][o]];
                if (cornerSlotOf[q] != t) fail("corner move inconsistent");
                int j = 0;
                while (cst[t][j] != q) j++;
                int dd = (j - o + 3) % 3;
                if (d < 0) d = dd;
                else if (d != dd) fail("corner twist convention inconsistent");
            }
            cdel[m][t] = (uint8_t)d;
        }
        for (int t = 0; t < 12; t++) {
            int s = edgeSlotOf[perm[m][est[t][0]]];
            esrc[m][t] = (uint8_t)s;
            int d = -1;
            for (int o = 0; o < 2; o++) {
                int q = inv[m][est[s][o]];
                if (edgeSlotOf[q] != t) fail("edge move inconsistent");
                int j = (est[t][0] == q) ? 0 : 1;
                int dd = j ^ o;
                if (d < 0) d = dd;
                else if (d != dd) fail("edge flip convention inconsistent");
            }
            edel[m][t] = (uint8_t)d;
        }
    }
}

inline Tables G;  // built once, before main()

// ----------------------------------------------------------------------------- moves
inline void applyMove(const Cube& a, int m, Cube& b) {
    const uint8_t* cs = G.csrc[m];
    const uint8_t* cd = G.cdel[m];
    const uint8_t* es = G.esrc[m];
    const uint8_t* ed = G.edel[m];
    for (int i = 0; i < 8; i++) {
        int s = cs[i];
        b.cp[i] = a.cp[s];
        uint8_t v = (uint8_t)(a.co[s] + cd[i]);
        b.co[i] = v >= 3 ? (uint8_t)(v - 3) : v;
    }
    for (int i = 0; i < 12; i++) {
        int s = es[i];
        b.ep[i] = a.ep[s];
        b.eo[i] = a.eo[s] ^ ed[i];
    }
}
inline Cube applyMoves(Cube c, const std::vector<int>& mv) {
    Cube n;
    for (int m : mv) { applyMove(c, m, n); c = n; }
    return c;
}
inline bool isSolved(const Cube& c) {
    static const Cube S = solvedCube();
    return sameCube(c, S);
}

// ----------------------------------------------------------------------------- text <-> moves
inline std::string movesToString(const std::vector<int>& mv) {
    std::string s;
    for (size_t i = 0; i < mv.size(); i++) { if (i) s += ' '; s += moveName(mv[i]); }
    return s;
}
inline bool parseMoves(const std::string& text, std::vector<int>& out, std::string& err) {
    out.clear();
    size_t i = 0, n = text.size();
    while (i < n) {
        char ch = text[i];
        if (ch == ' ' || ch == '\t' || ch == ',' || ch == '\n' || ch == '\r') { i++; continue; }
        char up = (char)std::toupper((unsigned char)ch);
        const char* p = std::strchr(FACE_LETTER, up);
        if (!p || up == 0) { err = std::string("unknown move letter '") + ch + "'"; return false; }
        int f = (int)(p - FACE_LETTER), k = 0;
        i++;
        while (i < n && (text[i] == '\'' || text[i] == '2' || text[i] == '3')) {
            if (text[i] == '2') k = 1;
            else if (text[i] == '\'' || text[i] == '3') k = (k == 1) ? 1 : 2;
            i++;
        }
        out.push_back(f * 3 + k);
    }
    return true;
}

// ----------------------------------------------------------------------------- 54 stickers <-> cube
inline std::string cubeToStickers(const Cube& c) {
    std::string s(G.goal, 54);
    for (int t = 0; t < 8; t++) {
        int cub = c.cp[t], o = c.co[t];
        for (int j = 0; j < 3; j++) s[G.cst[t][(j + o) % 3]] = G.chc[cub][j];
    }
    for (int t = 0; t < 12; t++) {
        int cub = c.ep[t], o = c.eo[t];
        for (int j = 0; j < 2; j++) s[G.est[t][j ^ o]] = G.ehc[cub][j];
    }
    return s;
}
inline int permParity(const uint8_t* p, int n) {
    bool seen[12] = {false};
    int par = 0;
    for (int i = 0; i < n; i++) {
        if (seen[i]) continue;
        int len = 0;
        for (int j = i; !seen[j]; j = p[j]) { seen[j] = true; len++; }
        par ^= (len - 1) & 1;
    }
    return par;
}
inline bool stickersToCube(const std::string& in, Cube& c, std::string& err) {
    std::string s;
    for (char ch : in) if (ch != ' ' && ch != ',' && ch != '\t' && ch != '\n' && ch != '\r') s += (char)std::toupper((unsigned char)ch);
    if (s.size() != 54) { err = "a raw state needs exactly 54 stickers (got " + std::to_string(s.size()) + ")"; return false; }
    int cnt[256] = {0};
    for (char ch : s) cnt[(unsigned char)ch]++;
    for (const char* p = "BRWOGY"; *p; p++)
        if (cnt[(unsigned char)*p] != 9) { err = std::string("colour ") + *p + " must appear exactly 9 times"; return false; }
    for (int i = 0; i < 54; i += 9)  // centres are fixed in this model
        if (s[i + 4] != G.goal[i + 4]) { err = "centre stickers must keep the solved layout (B,R,W,O,G,Y)"; return false; }
    bool usedC[8] = {false}, usedE[12] = {false};
    int sc = 0, se = 0;
    for (int t = 0; t < 8; t++) {
        char col[3];
        for (int j = 0; j < 3; j++) col[j] = s[G.cst[t][j]];
        int found = -1;
        for (int cub = 0; cub < 8 && found < 0; cub++) {
            bool ok = true;
            for (int j = 0; j < 3; j++) { bool in = false; for (int k = 0; k < 3; k++) if (G.chc[cub][k] == col[j]) in = true; ok = ok && in; }
            if (ok && col[0] != col[1] && col[1] != col[2] && col[0] != col[2]) found = cub;
        }
        if (found < 0) { err = "corner at slot " + std::to_string(t) + " has impossible colours"; return false; }
        int o = 0;
        while (col[o] != G.chc[found][0]) o++;
        for (int j = 0; j < 3; j++)
            if (col[(j + o) % 3] != G.chc[found][j]) { err = "corner at slot " + std::to_string(t) + " is mirrored (impossible cube)"; return false; }
        if (usedC[found]) { err = "the same corner appears twice"; return false; }
        usedC[found] = true;
        c.cp[t] = (uint8_t)found; c.co[t] = (uint8_t)o; sc += o;
    }
    for (int t = 0; t < 12; t++) {
        char col[2] = {s[G.est[t][0]], s[G.est[t][1]]};
        int found = -1;
        for (int cub = 0; cub < 12 && found < 0; cub++)
            if ((col[0] == G.ehc[cub][0] && col[1] == G.ehc[cub][1]) || (col[0] == G.ehc[cub][1] && col[1] == G.ehc[cub][0])) found = cub;
        if (found < 0) { err = "edge at slot " + std::to_string(t) + " has impossible colours"; return false; }
        int o = (col[0] == G.ehc[found][0]) ? 0 : 1;
        if (usedE[found]) { err = "the same edge appears twice"; return false; }
        usedE[found] = true;
        c.ep[t] = (uint8_t)found; c.eo[t] = (uint8_t)o; se += o;
    }
    if (sc % 3 != 0) { err = "unsolvable cube: a corner is twisted"; return false; }
    if (se % 2 != 0) { err = "unsolvable cube: an edge is flipped"; return false; }
    if (permParity(c.cp, 8) != permParity(c.ep, 12)) { err = "unsolvable cube: two pieces are swapped"; return false; }
    return true;
}

// Reads the user input: either a scramble ("F U B' R2 ...") or a raw 54-sticker state.
inline bool parseInput(const std::string& text, Cube& out, std::string& err) {
    bool looksLikeState = false;
    for (char ch : text) { char u = (char)std::toupper((unsigned char)ch); if (u == 'W' || u == 'O' || u == 'G' || u == 'Y') looksLikeState = true; }
    if (looksLikeState) return stickersToCube(text, out, err);
    std::vector<int> mv;
    if (!parseMoves(text, mv, err)) return false;
    out = applyMoves(solvedCube(), mv);
    return true;
}

}  // namespace rc
