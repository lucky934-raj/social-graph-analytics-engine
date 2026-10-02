#include "RecommendationEngine.h"

#include <algorithm>
#include <unordered_map>

namespace RecommendationEngine {

namespace {

// Strict ordering used for ranking: true if x should come before y.
bool rankedBefore(const Recommendation& x, const Recommendation& y) {
    // Compare x.mutual / x.union with y.mutual / y.union by cross-multiplying,
    // which is exact (no floating point rounding) since both denominators are > 0.
    long long lhs = static_cast<long long>(x.mutualCount) * y.unionSize;
    long long rhs = static_cast<long long>(y.mutualCount) * x.unionSize;
    if (lhs != rhs) {
        return lhs > rhs;
    }
    if (x.mutualCount != y.mutualCount) {
        return x.mutualCount > y.mutualCount;
    }
    return x.userId < y.userId;
}

}  // namespace

double jaccardSimilarity(const SocialGraph& graph, int a, int b) {
    const auto* smaller = &graph.neighbors(a);
    const auto* larger = &graph.neighbors(b);
    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    std::size_t shared = 0;
    for (const auto& entry : *smaller) {
        shared += larger->count(entry.first);
    }
    std::size_t unionSize = smaller->size() + larger->size() - shared;
    if (unionSize == 0) {
        return 0.0;
    }
    return static_cast<double>(shared) / static_cast<double>(unionSize);
}

std::vector<Recommendation> recommendFriends(const SocialGraph& graph, int userId,
                                             std::size_t k) {
    const SocialGraph::NeighborMap& myFriends = graph.neighbors(userId);

    // A candidate w is reached once through every friend f that w shares with
    // the user, so counting visits gives |Friends(user) ∩ Friends(w)| directly.
    std::unordered_map<int, int> mutualCounts;
    for (const auto& f : myFriends) {
        for (const auto& w : graph.neighbors(f.first)) {
            int candidate = w.first;
            if (candidate == userId || myFriends.count(candidate) != 0) {
                continue;
            }
            ++mutualCounts[candidate];
        }
    }

    const int myDegree = static_cast<int>(myFriends.size());
    std::vector<Recommendation> result;
    result.reserve(mutualCounts.size());
    for (const auto& entry : mutualCounts) {
        int candidate = entry.first;
        int mutual = entry.second;
        int candidateDegree = static_cast<int>(graph.degree(candidate));
        // Inclusion-exclusion: |A ∪ B| = |A| + |B| - |A ∩ B|.
        result.push_back({candidate, mutual, myDegree + candidateDegree - mutual});
    }

    if (k < result.size()) {
        std::partial_sort(result.begin(), result.begin() + static_cast<std::ptrdiff_t>(k),
                          result.end(), rankedBefore);
        result.resize(k);
    } else {
        std::sort(result.begin(), result.end(), rankedBefore);
    }
    return result;
}

}  // namespace RecommendationEngine
