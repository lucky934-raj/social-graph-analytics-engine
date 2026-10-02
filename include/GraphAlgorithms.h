#pragma once

#include "SocialGraph.h"

#include <cstddef>
#include <vector>

struct PathResult {
    bool found = false;
    long long distance = -1;  // edges for BFS, total weight for Dijkstra; -1 if not found
    std::vector<int> path;    // source ... target; empty if not found
    std::size_t expanded = 0; // vertices whose adjacency lists were scanned (search effort)
};

// Stateless graph algorithms. They only read the graph, so they take it by
// const reference instead of living inside SocialGraph.
namespace GraphAlgorithms {

// Friends shared by users a and b, sorted by id.
// Walks the smaller adjacency list and looks each id up in the larger one:
// O(min(deg a, deg b)) average lookups + O(m log m) to sort m results.
// Throws std::invalid_argument if either user does not exist.
std::vector<int> mutualFriends(const SocialGraph& graph, int a, int b);

// Fewest-hops path from source to target, ignoring edge weights.
// O(V + E) time, O(V) extra space. Throws if either user does not exist.
PathResult bfsShortestPath(const SocialGraph& graph, int source, int target);

}  // namespace GraphAlgorithms
