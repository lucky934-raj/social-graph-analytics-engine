#pragma once

#include "SocialGraph.h"

#include <cstddef>
#include <vector>

struct Recommendation {
    int userId = 0;
    int mutualCount = 0;  // |Friends(user) ∩ Friends(candidate)|
    int unionSize = 0;    // |Friends(user) ∪ Friends(candidate)|

    double jaccard() const { return static_cast<double>(mutualCount) / unionSize; }
};

namespace RecommendationEngine {

// Jaccard similarity of two users' friend sets: |A ∩ B| / |A ∪ B|.
// Defined as 0 when both users have no friends. O(min(deg a, deg b)).
double jaccardSimilarity(const SocialGraph& graph, int a, int b);

// Top-k friend suggestions for `userId`, ranked by Jaccard similarity, then
// number of mutual friends, then smaller user id (so the order is deterministic).
//
// Only friends-of-friends are considered: anyone else shares no friends with
// the user and therefore has similarity 0. The user and their existing
// friends are never suggested.
//
// Time: O(sum of deg(f) over the user's friends f + C log k), where C is the
// number of candidates. Throws std::invalid_argument for an unknown user.
std::vector<Recommendation> recommendFriends(const SocialGraph& graph, int userId,
                                             std::size_t k);

}  // namespace RecommendationEngine
