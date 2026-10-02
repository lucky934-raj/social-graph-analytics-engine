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

struct GraphStats {
    std::size_t users = 0;
    std::size_t friendships = 0;
    double averageDegree = 0.0;  // 2E / V, 0 for an empty graph
    std::size_t maxDegree = 0;
    std::size_t isolatedUsers = 0;  // users with no friends
    std::size_t components = 0;
    std::size_t largestComponent = 0;
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

// Same result as bfsShortestPath, but searches from both ends at once and
// stops when the two searches meet. Each round expands one whole level of the
// side with the smaller frontier. Worst case is still O(V + E), but with
// branching factor b and distance d it explores roughly 2 * b^(d/2) vertices
// instead of b^d. Throws if either user does not exist.
PathResult bidirectionalBfs(const SocialGraph& graph, int source, int target);

// Minimum total-weight path using Dijkstra's algorithm with a binary heap
// (std::priority_queue). Correct because SocialGraph only stores positive
// weights. O((V + E) log V) time, O(V + E) space for the heap in the worst
// case. Throws if either user does not exist.
PathResult dijkstra(const SocialGraph& graph, int source, int target);

// Summary statistics. Degrees are O(V); components use a DSU, O(V + E * alpha(V))
// (plus O(V log V) to sort user ids when building the snapshot).
GraphStats computeStats(const SocialGraph& graph);

}  // namespace GraphAlgorithms
