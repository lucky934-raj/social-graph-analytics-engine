#include "GraphAlgorithms.h"
#include "TestFramework.h"

#include <string>
#include <utility>
#include <vector>

namespace {

// Users 1..userCount named U1, U2, ..., plus the given friendships.
SocialGraph makeGraph(int userCount, const std::vector<std::pair<int, int>>& edges) {
    SocialGraph g;
    for (int id = 1; id <= userCount; ++id) {
        g.addUser(id, "U" + std::to_string(id));
    }
    for (const auto& e : edges) {
        g.addFriendship(e.first, e.second);
    }
    return g;
}

}  // namespace

// ---------- mutual friends ----------

TEST_CASE(mutual_friends_basic) {
    // 1 and 4 share friends 2 and 3; 5 is only a friend of 1.
    SocialGraph g = makeGraph(5, {{1, 2}, {1, 3}, {1, 5}, {4, 2}, {4, 3}});
    std::vector<int> expected{2, 3};
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 4) == expected);
    CHECK(GraphAlgorithms::mutualFriends(g, 4, 1) == expected);
}

TEST_CASE(mutual_friends_none) {
    SocialGraph g = makeGraph(4, {{1, 2}, {3, 4}});
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 3).empty());
}

TEST_CASE(mutual_friends_excludes_the_pair_itself) {
    // 1 and 2 are friends with each other and share 3; neither 1 nor 2 is "mutual".
    SocialGraph g = makeGraph(3, {{1, 2}, {1, 3}, {2, 3}});
    std::vector<int> expected{3};
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 2) == expected);
}

TEST_CASE(mutual_friends_isolated_users) {
    SocialGraph g = makeGraph(2, {});
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 2).empty());
}

TEST_CASE(mutual_friends_unknown_user_throws) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    CHECK_THROWS(GraphAlgorithms::mutualFriends(g, 1, 99));
}
