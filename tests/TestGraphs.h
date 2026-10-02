#pragma once

#include "SocialGraph.h"

#include <string>
#include <utility>
#include <vector>

// Builds a graph with users 1..userCount (named U1, U2, ...) and the given
// friendships, all with weight 1.
inline SocialGraph makeGraph(int userCount, const std::vector<std::pair<int, int>>& edges) {
    SocialGraph g;
    for (int id = 1; id <= userCount; ++id) {
        g.addUser(id, "U" + std::to_string(id));
    }
    for (const auto& e : edges) {
        g.addFriendship(e.first, e.second);
    }
    return g;
}
