// =============================================================================
//  table.hpp  -  memory-friendly hash tables for the graph searches
// -----------------------------------------------------------------------------
//  ChunkedVec : growing array made of fixed blocks (never copies old data, so
//               memory does not double when it grows)
//  Shard      : nodes (in a ChunkedVec) + open-addressing index (32-bit ids)
//  Sharded    : several shards selected by the hash of the state. The parallel
//               solvers give every shard to ONE thread at a time, so no locks
//               are needed. The sequential solvers use a single shard.
//  A node id is 32 bits: [ shard bits | local index ].
// =============================================================================
#pragma once
#include <cstdlib>
#include <memory>
#include <new>
#include "cube.hpp"

namespace rc {

constexpr uint32_t NONE32 = 0xFFFFFFFFu;

inline uint64_t hashKey(const Key& k) {
    uint64_t x = k.a * 0x9E3779B97F4A7C15ULL;
    x ^= (k.b + 0x632BE59BD9B4E019ULL) * 0xC2B2AE3D27D4EB4FULL;
    x ^= x >> 32;
    x *= 0xD6E8FEB86659FD93ULL;
    x ^= x >> 32;
    return x;
}

template <class T>
class ChunkedVec {
    static constexpr int B = 20;
    static constexpr size_t SZ = size_t(1) << B, M = SZ - 1;
    std::vector<T*> blocks_;
    size_t n_ = 0;

public:
    ChunkedVec() = default;
    ChunkedVec(const ChunkedVec&) = delete;
    ChunkedVec& operator=(const ChunkedVec&) = delete;
    ~ChunkedVec() { for (T* p : blocks_) std::free(p); }
    size_t size() const { return n_; }
    T& operator[](size_t i) { return blocks_[i >> B][i & M]; }
    const T& operator[](size_t i) const { return blocks_[i >> B][i & M]; }
    size_t push_back(const T& v) {
        if ((n_ & M) == 0) {
            T* p = (T*)std::malloc(SZ * sizeof(T));
            if (!p) throw std::bad_alloc();
            blocks_.push_back(p);
        }
        blocks_[n_ >> B][n_ & M] = v;
        return n_++;
    }
};

template <class Node>  // Node must have a member  Key key;
class Shard {
public:
    ChunkedVec<Node> nodes;
    std::vector<uint32_t> idx;
    uint64_t mask;

    Shard() { idx.assign(size_t(1) << 10, NONE32); mask = idx.size() - 1; }
    uint32_t find(const Key& k, uint64_t h) const {
        for (uint64_t p = h & mask;; p = (p + 1) & mask) {
            uint32_t v = idx[p];
            if (v == NONE32) return NONE32;
            if (nodes[v].key == k) return v;
        }
    }
    // precondition: the key is not in the table
    uint32_t insert(const Node& nd, uint64_t h) {
        if ((nodes.size() + 1) * 2 > idx.size()) grow();
        uint32_t id = (uint32_t)nodes.push_back(nd);
        place(id, h);
        return id;
    }
    size_t bytes() const { return nodes.size() * sizeof(Node) + idx.size() * sizeof(uint32_t); }

private:
    void place(uint32_t id, uint64_t h) {
        uint64_t p = h & mask;
        while (idx[p] != NONE32) p = (p + 1) & mask;
        idx[p] = id;
    }
    void grow() {
        std::vector<uint32_t> bigger(idx.size() * 2, NONE32);
        idx.swap(bigger);
        mask = idx.size() - 1;
        for (size_t i = 0; i < nodes.size(); i++) place((uint32_t)i, hashKey(nodes[i].key));
    }
};

template <class Node>
class Sharded {
public:
    int sbits;
    uint32_t lbits;
    std::vector<std::unique_ptr<Shard<Node>>> sh;

    explicit Sharded(int bits) : sbits(bits), lbits(32 - bits) {
        sh.resize(size_t(1) << bits);
        for (auto& p : sh) p.reset(new Shard<Node>());
    }
    size_t numShards() const { return sh.size(); }
    size_t shardOf(uint64_t h) const { return sbits ? (size_t)(h >> (64 - sbits)) : 0; }
    uint32_t gid(size_t s, uint32_t local) const { return sbits ? (uint32_t)((uint32_t)s << lbits | local) : local; }
    size_t shardOfId(uint32_t g) const { return sbits ? (size_t)(g >> lbits) : 0; }
    uint32_t localOfId(uint32_t g) const { return sbits ? (g & ((1u << lbits) - 1)) : g; }
    Node& node(uint32_t g) { return sh[shardOfId(g)]->nodes[localOfId(g)]; }
    size_t bytes() const { size_t b = 0; for (auto& p : sh) b += p->bytes(); return b; }
    size_t count() const { size_t b = 0; for (auto& p : sh) b += p->nodes.size(); return b; }
    // true when one shard is about to run out of 32-bit ids
    bool idSpaceNearlyFull() const {
        uint64_t lim = (lbits >= 32) ? 0xFFFFFF00ull : ((uint64_t(1) << lbits) - (1u << 20));
        for (auto& p : sh) if (p->nodes.size() > lim) return true;
        return false;
    }
};

}  // namespace rc
