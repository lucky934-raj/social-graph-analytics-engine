#include "GraphAlgorithms.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <random>
#include <climits>
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

// Sum of edge weights along a path (assumes the path is valid).
long long pathCost(const SocialGraph& g, const std::vector<int>& path) {
    long long cost = 0;
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        cost += g.neighbors(path[i]).at(path[i + 1]);
    }
    return cost;
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

// ---------- bidirectional BFS ----------

TEST_CASE(bidir_simple_chain_even_and_odd_lengths) {
    SocialGraph g = makeGraph(5, {{1, 2}, {2, 3}, {3, 4}, {4, 5}});
    std::vector<int> odd{1, 2, 3, 4};
    std::vector<int> even{1, 2, 3, 4, 5};
    PathResult r3 = GraphAlgorithms::bidirectionalBfs(g, 1, 4);
    PathResult r4 = GraphAlgorithms::bidirectionalBfs(g, 1, 5);
    CHECK(r3.found && r3.distance == 3 && r3.path == odd);
    CHECK(r4.found && r4.distance == 4 && r4.path == even);
}

TEST_CASE(bidir_prefers_fewer_hops) {
    SocialGraph g = makeGraph(6, {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 6}, {6, 5}});
    PathResult r = GraphAlgorithms::bidirectionalBfs(g, 1, 5);
    std::vector<int> expected{1, 6, 5};
    CHECK(r.distance == 2 && r.path == expected);
}

TEST_CASE(bidir_direct_friends_and_same_user) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    PathResult direct = GraphAlgorithms::bidirectionalBfs(g, 1, 2);
    std::vector<int> expected{1, 2};
    CHECK(direct.found && direct.distance == 1 && direct.path == expected);

    PathResult self = GraphAlgorithms::bidirectionalBfs(g, 2, 2);
    std::vector<int> justOne{2};
    CHECK(self.found && self.distance == 0 && self.path == justOne);
}

TEST_CASE(bidir_unreachable_and_unknown) {
    SocialGraph g = makeGraph(5, {{1, 2}, {2, 3}, {4, 5}});
    PathResult r = GraphAlgorithms::bidirectionalBfs(g, 1, 5);
    CHECK(!r.found && r.distance == -1 && r.path.empty());
    CHECK(!GraphAlgorithms::bidirectionalBfs(g, 5, 1).found);
    CHECK_THROWS(GraphAlgorithms::bidirectionalBfs(g, 1, 99));
}

TEST_CASE(bidir_hub_with_unbalanced_frontiers) {
    // Source 1 is a hub with many friends; target 20 sits at the end of a
    // chain. The search should keep expanding the small (chain) side.
    SocialGraph g = makeGraph(20, {});
    for (int leaf = 2; leaf <= 15; ++leaf) {
        g.addFriendship(1, leaf);
    }
    g.addFriendship(15, 16);
    g.addFriendship(16, 17);
    g.addFriendship(17, 20);
    PathResult r = GraphAlgorithms::bidirectionalBfs(g, 1, 20);
    std::vector<int> expected{1, 15, 16, 17, 20};
    CHECK(r.distance == 4 && r.path == expected);
}

TEST_CASE(bidir_matches_bfs_on_random_graphs) {
    // Compares against plain BFS for every pair of users in a few hundred
    // random graphs of different densities. Both algorithms may pick different
    // paths when several are shortest, so we compare distances and check that
    // each path is valid and has the right length.
    std::mt19937 rng(12345);
    int mismatches = 0;
    int pairsChecked = 0;
    for (int round = 0; round < 300; ++round) {
        int n = 2 + static_cast<int>(rng() % 30);
        int edgeChance = 3 + static_cast<int>(rng() % 30);  // percent
        SocialGraph g = makeGraph(n, {});
        for (int a = 1; a <= n; ++a) {
            for (int b = a + 1; b <= n; ++b) {
                if (static_cast<int>(rng() % 100) < edgeChance) {
                    g.addFriendship(a, b);
                }
            }
        }
        for (int s = 1; s <= n; ++s) {
            for (int t = 1; t <= n; ++t) {
                PathResult plain = GraphAlgorithms::bfsShortestPath(g, s, t);
                PathResult bidir = GraphAlgorithms::bidirectionalBfs(g, s, t);
                ++pairsChecked;
                bool ok = plain.found == bidir.found && plain.distance == bidir.distance;
                if (ok && bidir.found) {
                    ok = isValidPath(g, bidir.path, s, t) &&
                         bidir.path.size() == static_cast<std::size_t>(bidir.distance) + 1;
                }
                if (!ok) {
                    ++mismatches;
                }
            }
        }
    }
    CHECK(pairsChecked > 10000);
    CHECK(mismatches == 0);
}

// ---------- Dijkstra ----------

TEST_CASE(dijkstra_prefers_cheaper_path_with_more_hops) {
    // Direct edge 1-2 costs 10; going through 3 costs 1 + 1.
    SocialGraph g = makeGraph(3, {});
    g.addFriendship(1, 2, 10);
    g.addFriendship(1, 3, 1);
    g.addFriendship(3, 2, 1);
    PathResult r = GraphAlgorithms::dijkstra(g, 1, 2);
    std::vector<int> expected{1, 3, 2};
    CHECK(r.found && r.distance == 2 && r.path == expected);

    // BFS on the same graph counts hops and takes the direct edge.
    CHECK(GraphAlgorithms::bfsShortestPath(g, 1, 2).distance == 1);
}

TEST_CASE(dijkstra_updates_distance_after_first_discovery) {
    // 4 is first reached via 2 with cost 50, later improved to 3 via 3-5.
    // Exercises the stale-heap-entry skip.
    SocialGraph g = makeGraph(5, {});
    g.addFriendship(1, 2, 1);
    g.addFriendship(2, 4, 49);
    g.addFriendship(1, 3, 1);
    g.addFriendship(3, 5, 1);
    g.addFriendship(5, 4, 1);
    PathResult r = GraphAlgorithms::dijkstra(g, 1, 4);
    std::vector<int> expected{1, 3, 5, 4};
    CHECK(r.distance == 3 && r.path == expected);
}

TEST_CASE(dijkstra_same_user_unreachable_and_unknown) {
    SocialGraph g = makeGraph(4, {{1, 2}, {3, 4}});
    PathResult self = GraphAlgorithms::dijkstra(g, 1, 1);
    std::vector<int> justOne{1};
    CHECK(self.found && self.distance == 0 && self.path == justOne);

    PathResult none = GraphAlgorithms::dijkstra(g, 1, 4);
    CHECK(!none.found && none.distance == -1 && none.path.empty());

    CHECK_THROWS(GraphAlgorithms::dijkstra(g, 1, 77));
}

TEST_CASE(dijkstra_large_weights_do_not_overflow) {
    // Each weight fits in an int, but the total does not.
    SocialGraph g = makeGraph(4, {});
    g.addFriendship(1, 2, INT_MAX);
    g.addFriendship(2, 3, INT_MAX);
    g.addFriendship(3, 4, INT_MAX);
    PathResult r = GraphAlgorithms::dijkstra(g, 1, 4);
    CHECK(r.found && r.distance == 3LL * INT_MAX);
}

TEST_CASE(dijkstra_matches_floyd_warshall_on_random_weighted_graphs) {
    std::mt19937 rng(777);
    const long long kInf = LLONG_MAX / 4;
    int mismatches = 0;
    for (int round = 0; round < 200; ++round) {
        int n = 2 + static_cast<int>(rng() % 20);
        int edgeChance = 5 + static_cast<int>(rng() % 40);
        SocialGraph g = makeGraph(n, {});
        // Floyd-Warshall reference, 1-indexed.
        std::vector<std::vector<long long>> ref(n + 1, std::vector<long long>(n + 1, kInf));
        for (int v = 1; v <= n; ++v) {
            ref[v][v] = 0;
        }
        for (int a = 1; a <= n; ++a) {
            for (int b = a + 1; b <= n; ++b) {
                if (static_cast<int>(rng() % 100) < edgeChance) {
                    int w = 1 + static_cast<int>(rng() % 20);
                    g.addFriendship(a, b, w);
                    ref[a][b] = ref[b][a] = w;
                }
            }
        }
        for (int k = 1; k <= n; ++k) {
            for (int i = 1; i <= n; ++i) {
                for (int j = 1; j <= n; ++j) {
                    if (ref[i][k] + ref[k][j] < ref[i][j]) {
                        ref[i][j] = ref[i][k] + ref[k][j];
                    }
                }
            }
        }
        for (int s = 1; s <= n; ++s) {
            for (int t = 1; t <= n; ++t) {
                PathResult r = GraphAlgorithms::dijkstra(g, s, t);
                bool ok;
                if (ref[s][t] == kInf) {
                    ok = !r.found;
                } else {
                    ok = r.found && r.distance == ref[s][t] && isValidPath(g, r.path, s, t) &&
                         pathCost(g, r.path) == r.distance;
                }
                if (!ok) {
                    ++mismatches;
                }
            }
        }
    }
    CHECK(mismatches == 0);
}

TEST_CASE(dijkstra_with_unit_weights_matches_bfs_distance) {
    SocialGraph g = makeGraph(8, {{1, 2}, {2, 3}, {3, 8}, {1, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}});
    for (int t = 1; t <= 8; ++t) {
        CHECK(GraphAlgorithms::dijkstra(g, 1, t).distance ==
              GraphAlgorithms::bfsShortestPath(g, 1, t).distance);
    }
}
