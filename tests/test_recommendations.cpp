#include "RecommendationEngine.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <cmath>
#include <vector>

namespace {

std::vector<int> ids(const std::vector<Recommendation>& recs) {
    std::vector<int> result;
    for (const auto& r : recs) {
        result.push_back(r.userId);
    }
    return result;
}

bool near(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

// User 1 has friends {2, 3, 4}.
//   5: friends {2, 3}  -> mutual 2, union 3, J = 2/3
//   6: friends {2}     -> mutual 1, union 3, J = 1/3
//   7: friends {4, 8}  -> mutual 1, union 4, J = 1/4
//   8: friend of 7 only, two hops from 1's friends -> not a candidate
SocialGraph rankingGraph() {
    return makeGraph(8, {{1, 2}, {1, 3}, {1, 4}, {5, 2}, {5, 3}, {6, 2}, {7, 4}, {7, 8}});
}

}  // namespace

TEST_CASE(recommend_ranks_by_jaccard) {
    SocialGraph g = rankingGraph();
    auto recs = RecommendationEngine::recommendFriends(g, 1, 10);
    std::vector<int> expected{5, 6, 7};
    CHECK(ids(recs) == expected);
    CHECK(recs.size() == 3 && recs[0].mutualCount == 2 && recs[0].unionSize == 3);
    CHECK(recs.size() == 3 && near(recs[0].jaccard(), 2.0 / 3.0));
    CHECK(recs.size() == 3 && near(recs[2].jaccard(), 0.25));
}

TEST_CASE(recommend_never_suggests_self_or_existing_friends) {
    SocialGraph g = rankingGraph();
    for (int id : ids(RecommendationEngine::recommendFriends(g, 1, 100))) {
        CHECK(id != 1);
        CHECK(!g.areFriends(1, id));
    }
}

TEST_CASE(recommend_top_k_truncates) {
    SocialGraph g = rankingGraph();
    std::vector<int> top1{5};
    std::vector<int> top2{5, 6};
    CHECK(ids(RecommendationEngine::recommendFriends(g, 1, 1)) == top1);
    CHECK(ids(RecommendationEngine::recommendFriends(g, 1, 2)) == top2);
    CHECK(RecommendationEngine::recommendFriends(g, 1, 0).empty());
}

TEST_CASE(recommend_tie_on_jaccard_broken_by_mutual_count) {
    // User 1 has friends {2, 3}.
    //   4: friends {2}          -> J = 1/2 with 1 mutual
    //   5: friends {2, 3, 6, 7} -> J = 2/4 with 2 mutual (same Jaccard, more mutual)
    SocialGraph g = makeGraph(7, {{1, 2}, {1, 3}, {4, 2}, {5, 2}, {5, 3}, {5, 6}, {5, 7}});
    auto recs = RecommendationEngine::recommendFriends(g, 1, 10);
    std::vector<int> expected{5, 4};
    CHECK(ids(recs) == expected);
    CHECK(recs.size() == 2 && near(recs[0].jaccard(), recs[1].jaccard()));
}

TEST_CASE(recommend_full_tie_broken_by_user_id) {
    // 3, 5 and 9 are identical from user 1's point of view.
    SocialGraph g = makeGraph(9, {{1, 2}, {2, 9}, {2, 5}, {2, 3}});
    std::vector<int> expected{3, 5, 9};
    CHECK(ids(RecommendationEngine::recommendFriends(g, 1, 10)) == expected);
}

TEST_CASE(recommend_with_no_friends_or_no_candidates) {
    SocialGraph g = makeGraph(3, {{2, 3}});
    CHECK(RecommendationEngine::recommendFriends(g, 1, 5).empty());  // isolated user

    SocialGraph triangle = makeGraph(3, {{1, 2}, {2, 3}, {1, 3}});
    CHECK(RecommendationEngine::recommendFriends(triangle, 1, 5).empty());  // already friends with everyone
}

TEST_CASE(recommend_unknown_user_throws) {
    SocialGraph g = makeGraph(2, {{1, 2}});
    CHECK_THROWS(RecommendationEngine::recommendFriends(g, 42, 5));
}

TEST_CASE(jaccard_similarity_values) {
    SocialGraph g = makeGraph(6, {{1, 3}, {1, 4}, {2, 3}, {2, 4}, {5, 6}});
    CHECK(near(RecommendationEngine::jaccardSimilarity(g, 1, 2), 1.0));  // same friends
    CHECK(near(RecommendationEngine::jaccardSimilarity(g, 1, 5), 0.0));  // disjoint
    CHECK(near(RecommendationEngine::jaccardSimilarity(g, 3, 1), 0.0));
    CHECK(near(RecommendationEngine::jaccardSimilarity(g, 3, 4), 1.0));  // both {1, 2}

    SocialGraph empty = makeGraph(2, {});
    CHECK(near(RecommendationEngine::jaccardSimilarity(empty, 1, 2), 0.0));
}
