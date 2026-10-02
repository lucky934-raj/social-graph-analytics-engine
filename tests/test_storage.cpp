#include "GraphStorage.h"
#include "TestFramework.h"
#include "TestGraphs.h"

#include <cstdio>
#include <filesystem>
#include <sstream>
#include <string>

namespace {

// True if both graphs have the same users (ids and names) and the same
// friendships with the same weights.
bool sameGraph(const SocialGraph& x, const SocialGraph& y) {
    if (x.userIds() != y.userIds() || x.friendshipCount() != y.friendshipCount()) {
        return false;
    }
    for (int id : x.userIds()) {
        if (x.findUser(id)->name != y.findUser(id)->name || x.neighbors(id) != y.neighbors(id)) {
            return false;
        }
    }
    return true;
}

SocialGraph loadText(const std::string& text) {
    std::istringstream in(text);
    return GraphStorage::load(in);
}

// Returns the error message from loading `text`, or "" if it loaded fine.
std::string loadError(const std::string& text) {
    try {
        loadText(text);
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    return "";
}

}  // namespace

TEST_CASE(storage_round_trip) {
    SocialGraph g;
    g.addUser(3, "Mary Jane");
    g.addUser(-7, "Bob");
    g.addUser(10, "Charlie");
    g.addFriendship(3, -7, 4);
    g.addFriendship(10, 3);

    std::ostringstream out;
    GraphStorage::save(g, out);
    SocialGraph loaded = loadText(out.str());
    CHECK(sameGraph(g, loaded));
    CHECK(loaded.findUser(3)->name == "Mary Jane");
    CHECK(loaded.neighbors(-7).at(3) == 4);
}

TEST_CASE(storage_output_is_sorted_and_lists_each_edge_once) {
    SocialGraph g = makeGraph(3, {{3, 1}, {2, 1}});
    std::ostringstream out;
    GraphStorage::save(g, out);
    std::string expected =
        "# Social graph: 3 users, 2 friendships\n"
        "USER 1 U1\nUSER 2 U2\nUSER 3 U3\n"
        "FRIEND 1 2 1\nFRIEND 1 3 1\n";
    CHECK(out.str() == expected);
}

TEST_CASE(storage_empty_graph) {
    SocialGraph empty;
    std::ostringstream out;
    GraphStorage::save(empty, out);
    SocialGraph loaded = loadText(out.str());
    CHECK(loaded.userCount() == 0 && loaded.friendshipCount() == 0);
    CHECK(loadText("").userCount() == 0);
}

TEST_CASE(storage_accepts_comments_blank_lines_and_default_weight) {
    SocialGraph g = loadText(
        "# header\n"
        "\n"
        "USER 1   Alice Smith   \n"
        "   USER 2 Bob\n"
        "FRIEND 1 2\n");
    CHECK(g.findUser(1)->name == "Alice Smith");
    CHECK(g.areFriends(1, 2));
    CHECK(g.neighbors(1).at(2) == 1);
}

TEST_CASE(storage_reports_line_numbers_for_bad_input) {
    CHECK(loadError("USER 1 A\nUSER 1 B\n") == "line 2: duplicate user 1");
    CHECK(loadError("USER 1 A\nFRIEND 1 2\n") == "line 2: user 2 does not exist");
    CHECK(loadError("USER 1 A\nUSER 2 B\nFRIEND 1 2\nFRIEND 2 1\n") ==
          "line 4: duplicate friendship 2 - 1");
    CHECK(loadError("USER 1 A\nFRIEND 1 1\n") ==
          "line 2: a user cannot be friends with themselves");
    CHECK(loadError("USER 1 A\nUSER 2 B\nFRIEND 1 2 0\n") ==
          "line 3: friendship weight must be positive");
    CHECK(loadError("USER x A\n") == "line 1: expected an integer for user id, got 'x'");
    CHECK(loadError("USER 5\n") == "line 1: user name cannot be empty");
    CHECK(loadError("# ok\nEDGE 1 2\n") == "line 2: unknown record type 'EDGE'");
    CHECK(loadError("USER 1 A\nUSER 2 B\nFRIEND 1 2 3 4\n") == "line 3: too many arguments");
}

TEST_CASE(storage_file_round_trip_and_missing_file) {
    SocialGraph g = makeGraph(4, {{1, 2}, {3, 4}});
    std::string path =
        (std::filesystem::temp_directory_path() / "sgae_storage_test.txt").string();
    GraphStorage::saveToFile(g, path);
    SocialGraph loaded = GraphStorage::loadFromFile(path);
    std::remove(path.c_str());
    CHECK(sameGraph(g, loaded));

    CHECK_THROWS(GraphStorage::loadFromFile(path));  // removed above
    CHECK_THROWS(GraphStorage::saveToFile(g, "/nonexistent-dir/graph.txt"));
}

TEST_CASE(storage_sample_graph_loads) {
    SocialGraph g = GraphStorage::loadFromFile(SAMPLE_GRAPH_PATH);
    CHECK(g.userCount() == 13);
    CHECK(g.friendshipCount() == 15);
    CHECK(g.findUser(1)->name == "Aarav");
    CHECK(g.degree(13) == 0);
}
