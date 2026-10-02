#pragma once

#include "DisjointSetUnion.h"
#include "SocialGraph.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Snapshot of which users are connected (directly or through friends of
// friends), built with a DSU.
//
// A DSU can merge sets but cannot split them, so removing a friendship cannot
// be applied to an existing snapshot. Instead the snapshot records the graph
// version it was built from; callers rebuild it when the graph has changed.
class Connectivity {
public:
    // O(V + E * alpha(V)): one unite per friendship.
    explicit Connectivity(const SocialGraph& graph);

    // O(alpha(V)). Throws std::invalid_argument for users not in the snapshot.
    bool connected(int a, int b);

    std::size_t componentCount() const { return dsu_.setCount(); }
    std::size_t largestComponentSize();

    // Components as sorted lists of user ids, largest first (ties: smallest id first).
    std::vector<std::vector<int>> components();

    std::uint64_t graphVersion() const { return graphVersion_; }

private:
    int indexOf(int userId) const;

    std::vector<int> idByIndex_;              // DSU index -> user id (sorted)
    std::unordered_map<int, int> indexById_;  // user id -> DSU index
    DisjointSetUnion dsu_;
    std::uint64_t graphVersion_;
};
