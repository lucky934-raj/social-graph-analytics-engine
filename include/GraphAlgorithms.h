#pragma once

#include "SocialGraph.h"

#include <vector>

// Stateless graph algorithms. They only read the graph, so they take it by
// const reference instead of living inside SocialGraph.
namespace GraphAlgorithms {

// Friends shared by users a and b, sorted by id.
// Walks the smaller adjacency list and looks each id up in the larger one:
// O(min(deg a, deg b)) average lookups + O(m log m) to sort m results.
// Throws std::invalid_argument if either user does not exist.
std::vector<int> mutualFriends(const SocialGraph& graph, int a, int b);

}  // namespace GraphAlgorithms
