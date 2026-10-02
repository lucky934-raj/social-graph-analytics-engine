#include "SocialGraph.h"
#include "TestFramework.h"

TEST_CASE(empty_graph_has_no_users_or_friendships) {
    SocialGraph g;
    CHECK(g.userCount() == 0);
    CHECK(g.friendshipCount() == 0);
    CHECK(g.userIds().empty());
    CHECK(g.findUser(1) == nullptr);
    CHECK(!g.userExists(1));
}

TEST_CASE(add_and_find_user) {
    SocialGraph g;
    CHECK(g.addUser(1, "Alice"));
    CHECK(g.userExists(1));
    const User* alice = g.findUser(1);
    CHECK(alice != nullptr);
    CHECK(alice != nullptr && alice->name == "Alice" && alice->id == 1);
    CHECK(g.userCount() == 1);
    CHECK(g.degree(1) == 0);
}

TEST_CASE(duplicate_user_is_rejected) {
    SocialGraph g;
    CHECK(g.addUser(1, "Alice"));
    CHECK(!g.addUser(1, "Someone Else"));
    CHECK(g.findUser(1)->name == "Alice");
    CHECK(g.userCount() == 1);
}

TEST_CASE(empty_name_throws) {
    SocialGraph g;
    CHECK_THROWS(g.addUser(1, ""));
    CHECK(g.userCount() == 0);
}

TEST_CASE(add_friendship_is_undirected) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    CHECK(g.addFriendship(1, 2));
    CHECK(g.areFriends(1, 2));
    CHECK(g.areFriends(2, 1));
    CHECK(g.friendshipCount() == 1);
    CHECK(g.degree(1) == 1 && g.degree(2) == 1);
}

TEST_CASE(duplicate_friendship_is_rejected) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    CHECK(g.addFriendship(1, 2));
    CHECK(!g.addFriendship(1, 2));
    CHECK(!g.addFriendship(2, 1));
    CHECK(g.friendshipCount() == 1);
}

TEST_CASE(invalid_friendships_throw) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    CHECK_THROWS(g.addFriendship(1, 1));     // self-friendship
    CHECK_THROWS(g.addFriendship(1, 99));    // unknown user
    CHECK_THROWS(g.addFriendship(1, 2, 0));  // non-positive weight
    CHECK_THROWS(g.addFriendship(1, 2, -3));
    CHECK(g.friendshipCount() == 0);
    CHECK(!g.areFriends(1, 2));
}

TEST_CASE(weight_is_stored_in_both_directions) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    g.addFriendship(1, 2, 7);
    CHECK(g.neighbors(1).at(2) == 7);
    CHECK(g.neighbors(2).at(1) == 7);
}

TEST_CASE(remove_friendship) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    g.addUser(3, "Charlie");
    g.addFriendship(1, 2);
    CHECK(g.removeFriendship(2, 1));
    CHECK(!g.areFriends(1, 2));
    CHECK(g.friendshipCount() == 0);
    CHECK(!g.removeFriendship(1, 2));  // already removed
    CHECK(!g.removeFriendship(1, 3));  // never existed
    CHECK_THROWS(g.removeFriendship(1, 42));
}

TEST_CASE(remove_user_cleans_up_neighbours) {
    SocialGraph g;
    g.addUser(1, "Alice");
    g.addUser(2, "Bob");
    g.addUser(3, "Charlie");
    g.addFriendship(1, 2);
    g.addFriendship(1, 3);
    g.addFriendship(2, 3);

    CHECK(g.removeUser(1));
    CHECK(!g.userExists(1));
    CHECK(g.userCount() == 2);
    CHECK(g.friendshipCount() == 1);
    CHECK(g.neighbors(2).count(1) == 0);
    CHECK(g.neighbors(3).count(1) == 0);
    CHECK(g.areFriends(2, 3));
}

TEST_CASE(remove_nonexistent_user_returns_false) {
    SocialGraph g;
    CHECK(!g.removeUser(5));
    g.addUser(5, "Eve");
    CHECK(g.removeUser(5));
    CHECK(!g.removeUser(5));
}

TEST_CASE(are_friends_with_unknown_user_is_false) {
    SocialGraph g;
    g.addUser(1, "Alice");
    CHECK(!g.areFriends(1, 2));
    CHECK(!g.areFriends(7, 8));
}

TEST_CASE(unknown_user_queries_throw) {
    SocialGraph g;
    CHECK_THROWS(g.neighbors(3));
    CHECK_THROWS(g.degree(3));
}

TEST_CASE(user_ids_are_sorted_and_ids_can_be_sparse) {
    SocialGraph g;
    g.addUser(900, "C");
    g.addUser(-5, "A");
    g.addUser(42, "B");
    std::vector<int> expected{-5, 42, 900};
    CHECK(g.userIds() == expected);
}
