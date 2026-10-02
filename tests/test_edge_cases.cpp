#include "Connectivity.h"
#include "GraphAlgorithms.h"
#include "RecommendationEngine.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <climits>
#include <vector>

// Edge cases that cut across several components.

TEST_CASE(edge_empty_graph_rejects_queries) {
    SocialGraph g;
    CHECK_THROWS(GraphAlgorithms::bfsShortestPath(g, 1, 2));
    CHECK_THROWS(GraphAlgorithms::bidirectionalBfs(g, 1, 1));
    CHECK_THROWS(GraphAlgorithms::dijkstra(g, 1, 1));
    CHECK_THROWS(GraphAlgorithms::mutualFriends(g, 1, 2));
    CHECK_THROWS(RecommendationEngine::recommendFriends(g, 1, 3));
    CHECK(!g.areFriends(1, 2));
    CHECK(!g.removeUser(1));
}

TEST_CASE(edge_single_user_graph) {
    SocialGraph g = makeGraph(1, {});
    std::vector<int> justOne{1};
    CHECK(GraphAlgorithms::bfsShortestPath(g, 1, 1).path == justOne);
    CHECK(GraphAlgorithms::bidirectionalBfs(g, 1, 1).path == justOne);
    CHECK(GraphAlgorithms::dijkstra(g, 1, 1).path == justOne);
    CHECK(RecommendationEngine::recommendFriends(g, 1, 5).empty());
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 1).empty());
    GraphStats s = GraphAlgorithms::computeStats(g);
    CHECK(s.users == 1 && s.components == 1 && s.isolatedUsers == 1 && s.averageDegree == 0.0);
}

TEST_CASE(edge_removed_user_is_gone_from_every_algorithm) {
    // 1-2-3 chain plus 1-4-3. Removing 2 leaves only the route through 4;
    // removing 4 as well disconnects 1 from 3.
    SocialGraph g = makeGraph(4, {{1, 2}, {2, 3}, {1, 4}, {4, 3}});
    g.removeUser(2);
    std::vector<int> viaFour{1, 4, 3};
    CHECK(GraphAlgorithms::bfsShortestPath(g, 1, 3).path == viaFour);
    CHECK(GraphAlgorithms::bidirectionalBfs(g, 1, 3).path == viaFour);
    CHECK(GraphAlgorithms::dijkstra(g, 1, 3).path == viaFour);
    CHECK_THROWS(GraphAlgorithms::bfsShortestPath(g, 1, 2));

    std::vector<int> mutual{4};
    CHECK(GraphAlgorithms::mutualFriends(g, 1, 3) == mutual);
    for (const auto& rec : RecommendationEngine::recommendFriends(g, 1, 10)) {
        CHECK(rec.userId != 2);
    }

    g.removeUser(4);
    CHECK(!GraphAlgorithms::bfsShortestPath(g, 1, 3).found);
    CHECK(!Connectivity(g).connected(1, 3));
    CHECK(g.friendshipCount() == 0);
}

TEST_CASE(edge_reused_id_starts_without_old_friendships) {
    SocialGraph g = makeGraph(3, {{1, 2}, {1, 3}});
    g.removeUser(1);
    CHECK(g.addUser(1, "New One"));
    CHECK(g.degree(1) == 0);
    CHECK(!g.areFriends(1, 2));
    CHECK(g.degree(2) == 0 && g.degree(3) == 0);
}

TEST_CASE(edge_extreme_user_ids) {
    SocialGraph g;
    g.addUser(INT_MIN, "Min");
    g.addUser(0, "Zero");
    g.addUser(INT_MAX, "Max");
    g.addFriendship(INT_MIN, 0);
    g.addFriendship(0, INT_MAX);
    PathResult r = GraphAlgorithms::bidirectionalBfs(g, INT_MIN, INT_MAX);
    std::vector<int> expected{INT_MIN, 0, INT_MAX};
    CHECK(r.found && r.path == expected);
    CHECK(Connectivity(g).connected(INT_MIN, INT_MAX));
    auto recs = RecommendationEngine::recommendFriends(g, INT_MIN, 5);
    CHECK(recs.size() == 1 && recs[0].userId == INT_MAX);
}

TEST_CASE(edge_recommendations_on_large_star) {
    // Hub 1 with 20000 leaves. Leaf 2 sees every other leaf as a candidate with
    // exactly one mutual friend (the hub) and the same union size, so ranking
    // falls through to user id.
    const int leaves = 20000;
    SocialGraph g = makeGraph(leaves + 1, {});
    for (int leaf = 2; leaf <= leaves + 1; ++leaf) {
        g.addFriendship(1, leaf);
    }
    auto recs = RecommendationEngine::recommendFriends(g, 2, 3);
    CHECK(recs.size() == 3);
    CHECK(recs.size() == 3 && recs[0].userId == 3 && recs[1].userId == 4 && recs[2].userId == 5);
    CHECK(recs.size() == 3 && recs[0].mutualCount == 1 && recs[0].unionSize == 1);
    CHECK(RecommendationEngine::recommendFriends(g, 1, 3).empty());  // hub knows everyone
}

TEST_CASE(edge_long_path_graph) {
    // A 50000-vertex path: deep searches must not recurse or overflow.
    const int n = 50000;
    SocialGraph g = makeGraph(n, {});
    for (int i = 1; i < n; ++i) {
        g.addFriendship(i, i + 1);
    }
    CHECK(GraphAlgorithms::bfsShortestPath(g, 1, n).distance == n - 1);
    CHECK(GraphAlgorithms::bidirectionalBfs(g, 1, n).distance == n - 1);
    CHECK(GraphAlgorithms::dijkstra(g, 1, n).distance == n - 1);
    CHECK(GraphAlgorithms::bidirectionalBfs(g, 1, n).path.size() == static_cast<std::size_t>(n));
}
