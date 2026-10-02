#include "GraphAlgorithms.h"

#include <algorithm>

namespace GraphAlgorithms {

std::vector<int> mutualFriends(const SocialGraph& graph, int a, int b) {
    const auto* smaller = &graph.neighbors(a);
    const auto* larger = &graph.neighbors(b);
    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    std::vector<int> result;
    for (const auto& entry : *smaller) {
        if (larger->count(entry.first) != 0) {
            result.push_back(entry.first);
        }
    }
    // Hash map iteration order is unspecified; sort so output is deterministic.
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace GraphAlgorithms
