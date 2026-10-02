#include "GraphAlgorithms.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <vector>

namespace {

// A path is valid if it starts at source, ends at target and every consecutive
// pair of users are friends.
bool isValidPath(const SocialGraph& g, const std::vector<int>& path, int source, int target) {
    if (path.empty() || path.front() != source || path.back() != target) {
        return false;
    }
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        if (!g.areFriends(path[i], path[i + 1])) {
            return false;
        }
    }
    return true;
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

// ---------- BFS ----------

TEST_CASE(bfs_simple_chain) {
    SocialGraph g = makeGraph(4, {{1, 2}, {2, 3}, {3, 4}});
    PathResult r = GraphAlgorithms::bfsShortestPath(g, 1, 4);
    std::vector<int> expected{1, 2, 3, 4};
    CHECK(r.found);
    CHECK(r.distance == 3);
    CHECK(r.path == expected);
}

TEST_CASE(bfs_prefers_fewer_hops) {
    // Long route 1-2-3-4-5 and a shortcut 1-6-5.
    SocialGraph g = makeGraph(6, {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 6}, {6, 5}});
    PathResult r = GraphAlgorithms::bfsShortestPath(g, 1, 5);
    std::vector<int> expected{1, 6, 5};
    CHECK(r.distance == 2);
    CHECK(r.path == expected);
}

TEST_CASE(bfs_tied_paths_return_any_valid_shortest_path) {
    // Diamond: 1-2-4 and 1-3-4 are both shortest. Which one is returned depends
    // on hash map iteration order, so only check that it is valid and shortest.
    SocialGraph g = makeGraph(4, {{1, 2}, {1, 3}, {2, 4}, {3, 4}});
    PathResult r = GraphAlgorithms::bfsShortestPath(g, 1, 4);
    CHECK(r.found && r.distance == 2);
    CHECK(r.path.size() == 3);
    CHECK(isValidPath(g, r.path, 1, 4));
}

TEST_CASE(bfs_direct_friends_and_same_user) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    PathResult direct = GraphAlgorithms::bfsShortestPath(g, 2, 1);
    std::vector<int> expected{2, 1};
    CHECK(direct.distance == 1 && direct.path == expected);

    PathResult self = GraphAlgorithms::bfsShortestPath(g, 1, 1);
    std::vector<int> justOne{1};
    CHECK(self.found && self.distance == 0 && self.path == justOne);
}

TEST_CASE(bfs_unreachable) {
    SocialGraph g = makeGraph(4, {{1, 2}, {3, 4}});
    PathResult r = GraphAlgorithms::bfsShortestPath(g, 1, 4);
    CHECK(!r.found);
    CHECK(r.distance == -1);
    CHECK(r.path.empty());

    SocialGraph isolated = makeGraph(2, {});
    CHECK(!GraphAlgorithms::bfsShortestPath(isolated, 1, 2).found);
}

TEST_CASE(bfs_ignores_weights) {
    SocialGraph g = makeGraph(3, {{1, 3}, {3, 2}});
    g.addFriendship(1, 2, 100);
    PathResult r = GraphAlgorithms::bfsShortestPath(g, 1, 2);
    std::vector<int> expected{1, 2};
    CHECK(r.distance == 1 && r.path == expected);
}

TEST_CASE(bfs_unknown_user_throws) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    CHECK_THROWS(GraphAlgorithms::bfsShortestPath(g, 1, 9));
    CHECK_THROWS(GraphAlgorithms::bfsShortestPath(g, 9, 1));
}
