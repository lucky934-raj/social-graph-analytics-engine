#include "Connectivity.h"
#include "DisjointSetUnion.h"
#include "GraphAlgorithms.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <cmath>
#include <random>
#include <vector>

// ---------- DisjointSetUnion ----------

TEST_CASE(dsu_starts_with_singletons) {
    DisjointSetUnion dsu(4);
    CHECK(dsu.elementCount() == 4);
    CHECK(dsu.setCount() == 4);
    for (int i = 0; i < 4; ++i) {
        CHECK(dsu.find(i) == i);
        CHECK(dsu.setSize(i) == 1);
    }
    CHECK(!dsu.connected(0, 1));
}

TEST_CASE(dsu_unite_and_connected) {
    DisjointSetUnion dsu(6);
    CHECK(dsu.unite(0, 1));
    CHECK(dsu.unite(2, 3));
    CHECK(dsu.unite(1, 3));
    CHECK(!dsu.unite(0, 2));  // already in the same set
    CHECK(dsu.connected(0, 3));
    CHECK(dsu.connected(2, 1));
    CHECK(!dsu.connected(0, 4));
    CHECK(dsu.setCount() == 3);  // {0,1,2,3}, {4}, {5}
    CHECK(dsu.setSize(2) == 4);
    CHECK(dsu.setSize(5) == 1);
}

TEST_CASE(dsu_self_union_is_a_no_op) {
    DisjointSetUnion dsu(2);
    CHECK(!dsu.unite(1, 1));
    CHECK(dsu.setCount() == 2);
}

TEST_CASE(dsu_out_of_range_throws) {
    DisjointSetUnion dsu(3);
    CHECK_THROWS(dsu.find(3));
    CHECK_THROWS(dsu.find(-1));
    CHECK_THROWS(dsu.unite(0, 7));

    DisjointSetUnion empty(0);
    CHECK(empty.setCount() == 0);
    CHECK_THROWS(empty.find(0));
}

TEST_CASE(dsu_long_chain) {
    // Uniting neighbours of a long chain one by one; find is iterative, so no
    // risk of deep recursion even before compression flattens things.
    const int n = 200000;
    DisjointSetUnion dsu(n);
    for (int i = 0; i + 1 < n; ++i) {
        dsu.unite(i, i + 1);
    }
    CHECK(dsu.setCount() == 1);
    CHECK(dsu.connected(0, n - 1));
    CHECK(dsu.setSize(12345) == n);
}

// ---------- Connectivity on the social graph ----------

TEST_CASE(components_of_small_graph) {
    // {1,2,3}, {4,5}, {6}
    SocialGraph g = makeGraph(6, {{1, 2}, {2, 3}, {4, 5}});
    Connectivity c(g);
    CHECK(c.componentCount() == 3);
    CHECK(c.largestComponentSize() == 3);
    std::vector<std::vector<int>> expected{{1, 2, 3}, {4, 5}, {6}};
    CHECK(c.components() == expected);
    CHECK(c.connected(1, 3));
    CHECK(c.connected(5, 4));
    CHECK(!c.connected(3, 4));
    CHECK(c.connected(6, 6));
    CHECK_THROWS(c.connected(1, 100));
}

TEST_CASE(components_ordering_ties_by_smallest_id) {
    SocialGraph g = makeGraph(4, {{3, 4}, {1, 2}});
    std::vector<std::vector<int>> expected{{1, 2}, {3, 4}};
    CHECK(Connectivity(g).components() == expected);
}

TEST_CASE(components_with_sparse_and_negative_ids) {
    SocialGraph g;
    g.addUser(-10, "A");
    g.addUser(500, "B");
    g.addUser(7, "C");
    g.addFriendship(-10, 500);
    Connectivity c(g);
    CHECK(c.connected(-10, 500));
    CHECK(!c.connected(7, 500));
    std::vector<std::vector<int>> expected{{-10, 500}, {7}};
    CHECK(c.components() == expected);
}

TEST_CASE(components_empty_and_single_user_graph) {
    SocialGraph empty;
    Connectivity c0(empty);
    CHECK(c0.componentCount() == 0);
    CHECK(c0.largestComponentSize() == 0);
    CHECK(c0.components().empty());

    SocialGraph single = makeGraph(1, {});
    Connectivity c1(single);
    CHECK(c1.componentCount() == 1);
    CHECK(c1.largestComponentSize() == 1);
    CHECK(c1.connected(1, 1));
}

TEST_CASE(snapshot_reflects_graph_at_build_time) {
    SocialGraph g = makeGraph(3, {{1, 2}, {2, 3}});
    Connectivity before(g);
    std::uint64_t versionBefore = g.version();
    g.removeFriendship(2, 3);
    CHECK(g.version() != versionBefore);
    CHECK(before.connected(1, 3));  // stale snapshot still says connected
    Connectivity after(g);
    CHECK(!after.connected(1, 3));
    CHECK(after.componentCount() == 2);
}

TEST_CASE(version_changes_only_on_real_modifications) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    std::uint64_t v = g.version();
    g.addFriendship(1, 2);   // duplicate, no change
    g.removeUser(99);        // unknown, no change
    g.addUser(1, "Again");   // duplicate id, no change
    CHECK(g.version() == v);
    g.addUser(3, "New");
    CHECK(g.version() == v + 1);
}

TEST_CASE(connectivity_matches_bfs_reachability_on_random_graphs) {
    std::mt19937 rng(2024);
    int mismatches = 0;
    for (int round = 0; round < 100; ++round) {
        int n = 1 + static_cast<int>(rng() % 25);
        SocialGraph g = makeGraph(n, {});
        for (int a = 1; a <= n; ++a) {
            for (int b = a + 1; b <= n; ++b) {
                if (rng() % 100 < 8) {
                    g.addFriendship(a, b);
                }
            }
        }
        Connectivity c(g);
        for (int a = 1; a <= n; ++a) {
            for (int b = 1; b <= n; ++b) {
                if (c.connected(a, b) != GraphAlgorithms::bfsShortestPath(g, a, b).found) {
                    ++mismatches;
                }
            }
        }
    }
    CHECK(mismatches == 0);
}

// ---------- statistics ----------

TEST_CASE(stats_of_empty_graph) {
    SocialGraph g;
    GraphStats s = GraphAlgorithms::computeStats(g);
    CHECK(s.users == 0 && s.friendships == 0);
    CHECK(s.averageDegree == 0.0);
    CHECK(s.maxDegree == 0 && s.isolatedUsers == 0);
    CHECK(s.components == 0 && s.largestComponent == 0);
}

TEST_CASE(stats_of_small_graph) {
    // Star 1-{2,3,4}, edge 5-6, isolated 7.
    SocialGraph g = makeGraph(7, {{1, 2}, {1, 3}, {1, 4}, {5, 6}});
    GraphStats s = GraphAlgorithms::computeStats(g);
    CHECK(s.users == 7);
    CHECK(s.friendships == 4);
    CHECK(std::fabs(s.averageDegree - 8.0 / 7.0) < 1e-12);
    CHECK(s.maxDegree == 3);
    CHECK(s.isolatedUsers == 1);
    CHECK(s.components == 3);
    CHECK(s.largestComponent == 4);
}
