#include "CommandProcessor.h"
#include "TestFramework.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

// Runs a single command and returns what it printed.
std::string runCommand(CommandProcessor& cli, const std::string& line) {
    std::ostringstream out;
    cli.execute(line, out);
    return out.str();
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

TEST_CASE(cli_adds_users_with_multi_word_names) {
    CommandProcessor cli;
    CHECK(contains(runCommand(cli, "ADD_USER 1 Mary Jane Watson"), "Added user Mary Jane Watson(1)"));
    CHECK(cli.graph().findUser(1)->name == "Mary Jane Watson");
    CHECK(contains(runCommand(cli, "ADD_USER 1 Bob"), "already exists"));
}

TEST_CASE(cli_commands_are_case_insensitive) {
    CommandProcessor cli;
    runCommand(cli, "add_user 1 Alice");
    runCommand(cli, "Add_User 2 Bob");
    runCommand(cli, "add_friend 1 2");
    CHECK(cli.graph().areFriends(1, 2));
}

TEST_CASE(cli_friendship_commands) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1 2"), "Added friendship Alice(1) - Bob(2)"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1 3 5"), "(weight 5)"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 2 1"), "already friends"));
    CHECK(contains(runCommand(cli, "ARE_FRIENDS 2 1"), "Yes"));
    CHECK(contains(runCommand(cli, "ARE_FRIENDS 2 3"), "No"));
    CHECK(contains(runCommand(cli, "FRIENDS 1"), "Friends of Alice(1) (2): Bob(2) Charlie(3)"));
    CHECK(contains(runCommand(cli, "REMOVE_FRIEND 1 2"), "Removed friendship"));
    CHECK(contains(runCommand(cli, "REMOVE_FRIEND 1 2"), "are not friends"));
}

TEST_CASE(cli_reports_errors_without_crashing) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1 1"), "Error: a user cannot be friends with themselves"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1 9"), "Error: user 9 does not exist"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1 x"), "Error: expected an integer"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 1"), "Error: missing second user id"));
    CHECK(contains(runCommand(cli, "ADD_USER 2"), "Error: user name cannot be empty"));
    CHECK(contains(runCommand(cli, "ARE_FRIENDS 1 2 3"), "Error: too many arguments"));
    CHECK(contains(runCommand(cli, "ADD_USER 12abc Bob"), "Error: expected an integer"));
    CHECK(contains(runCommand(cli, "DANCE"), "Error: unknown command 'DANCE'"));
}

TEST_CASE(cli_remove_and_find_user) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    CHECK(contains(runCommand(cli, "FIND_USER 1"), "Alice(1), 0 friend(s)"));
    CHECK(contains(runCommand(cli, "REMOVE_USER 1"), "Removed user Alice(1)"));
    CHECK(contains(runCommand(cli, "FIND_USER 1"), "not found"));
    CHECK(contains(runCommand(cli, "REMOVE_USER 1"), "does not exist"));
}

TEST_CASE(cli_mutual_friends) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    runCommand(cli, "ADD_FRIEND 1 2");
    runCommand(cli, "ADD_FRIEND 2 3");
    CHECK(contains(runCommand(cli, "MUTUAL 1 3"),
                   "Mutual friends of Alice(1) and Charlie(3) (1): Bob(2)"));
    CHECK(contains(runCommand(cli, "MUTUAL 1 2"), "(0):"));
    CHECK(contains(runCommand(cli, "MUTUAL 1 7"), "Error: user 7 does not exist"));
}

TEST_CASE(cli_recommend_and_jaccard) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    runCommand(cli, "ADD_FRIEND 1 2");
    runCommand(cli, "ADD_FRIEND 2 3");
    std::string out = runCommand(cli, "RECOMMEND 1 5");
    CHECK(contains(out, "Recommendations for Alice(1):"));
    CHECK(contains(out, "1. Charlie(3)  jaccard=1.000 (1 mutual / 1 in union)"));
    CHECK(contains(runCommand(cli, "RECOMMEND 2"), "No recommendations"));
    CHECK(contains(runCommand(cli, "RECOMMEND 1 -1"), "Error: count must not be negative"));
    CHECK(contains(runCommand(cli, "JACCARD 1 3"), "= 1.000"));
}

TEST_CASE(cli_bfs) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    runCommand(cli, "ADD_USER 4 Dave");
    runCommand(cli, "ADD_FRIEND 1 2");
    runCommand(cli, "ADD_FRIEND 2 3");
    std::string out = runCommand(cli, "BFS 1 3");
    CHECK(contains(out, "Distance: 2"));
    CHECK(contains(out, "Path: Alice(1) -> Bob(2) -> Charlie(3)"));
    CHECK(contains(runCommand(cli, "BFS 1 4"), "No path from Alice(1) to Dave(4)"));
    CHECK(contains(runCommand(cli, "BFS 1 5"), "Error: user 5 does not exist"));

    std::string bidir = runCommand(cli, "BIDIR_BFS 3 1");
    CHECK(contains(bidir, "Distance: 2"));
    CHECK(contains(bidir, "Path: Charlie(3) -> Bob(2) -> Alice(1)"));
    CHECK(contains(runCommand(cli, "BIDIR_BFS 4 1"), "No path"));
}

TEST_CASE(cli_dijkstra) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    runCommand(cli, "ADD_FRIEND 1 2 10");
    runCommand(cli, "ADD_FRIEND 1 3 2");
    runCommand(cli, "ADD_FRIEND 3 2 3");
    std::string out = runCommand(cli, "DIJKSTRA 1 2");
    CHECK(contains(out, "Cost: 5"));
    CHECK(contains(out, "Path: Alice(1) -> Charlie(3) -> Bob(2)"));
    CHECK(contains(runCommand(cli, "ADD_FRIEND 2 3 -4"), "Error: friendship weight must be positive"));
}

TEST_CASE(cli_connectivity_and_stats) {
    CommandProcessor cli;
    runCommand(cli, "ADD_USER 1 Alice");
    runCommand(cli, "ADD_USER 2 Bob");
    runCommand(cli, "ADD_USER 3 Charlie");
    runCommand(cli, "ADD_FRIEND 1 2");
    CHECK(contains(runCommand(cli, "CONNECTED 1 2"), "Yes"));
    CHECK(contains(runCommand(cli, "CONNECTED 1 3"), "No"));

    std::string comps = runCommand(cli, "COMPONENTS");
    CHECK(contains(comps, "2 connected component(s)"));
    CHECK(contains(comps, "#1 (2 user(s)): Alice(1) Bob(2)"));
    CHECK(contains(comps, "#2 (1 user(s)): Charlie(3)"));

    // The cached snapshot must be rebuilt after the graph changes.
    runCommand(cli, "ADD_FRIEND 2 3");
    CHECK(contains(runCommand(cli, "CONNECTED 1 3"), "Yes"));
    runCommand(cli, "REMOVE_FRIEND 1 2");
    CHECK(contains(runCommand(cli, "CONNECTED 1 3"), "No"));

    std::string stats = runCommand(cli, "STATS");
    CHECK(contains(stats, "Users:              3"));
    CHECK(contains(stats, "Friendships:        1"));
    CHECK(contains(stats, "Average degree:     0.67"));
    CHECK(contains(stats, "Components:         2"));
    CHECK(contains(runCommand(cli, "CONNECTED 1 9"), "Error: user 9 does not exist"));
}

TEST_CASE(cli_save_and_load) {
    std::string dir = std::filesystem::temp_directory_path().string();
    std::string good = dir + "/sgae_cli_good.txt";
    std::string bad = dir + "/sgae_cli_bad.txt";

    CommandProcessor writer;
    runCommand(writer, "ADD_USER 1 Alice");
    runCommand(writer, "ADD_USER 2 Bob");
    runCommand(writer, "ADD_FRIEND 1 2 3");
    CHECK(contains(runCommand(writer, "SAVE " + good), "Saved 2 users and 1 friendships"));

    CommandProcessor reader;
    runCommand(reader, "ADD_USER 9 Zed");
    CHECK(contains(runCommand(reader, "CONNECTED 9 9"), "Yes"));  // builds a cached snapshot
    CHECK(contains(runCommand(reader, "LOAD " + good), "Loaded 2 users and 1 friendships"));
    CHECK(!reader.graph().userExists(9));
    CHECK(reader.graph().neighbors(1).at(2) == 3);
    CHECK(contains(runCommand(reader, "CONNECTED 1 2"), "Yes"));  // snapshot was rebuilt

    // A broken file must leave the current graph unchanged.
    {
        std::ofstream f(bad);
        f << "USER 1 Alice\nFRIEND 1 5\n";
    }
    CHECK(contains(runCommand(reader, "LOAD " + bad), "Error: line 2: user 5 does not exist"));
    CHECK(reader.graph().userCount() == 2 && reader.graph().areFriends(1, 2));

    CHECK(contains(runCommand(reader, "LOAD"), "Error: missing file path"));
    CHECK(contains(runCommand(reader, "LOAD " + dir + "/no_such_file.txt"), "Error: cannot open"));
    std::remove(good.c_str());
    std::remove(bad.c_str());
}

TEST_CASE(cli_exit_comments_and_blank_lines) {
    CommandProcessor cli;
    std::ostringstream out;
    CHECK(cli.execute("", out));
    CHECK(cli.execute("   ", out));
    CHECK(cli.execute("# just a comment", out));
    CHECK(out.str().empty());
    CHECK(!cli.execute("EXIT", out));
    CHECK(!cli.execute("quit", out));
}

TEST_CASE(cli_run_stops_at_exit) {
    CommandProcessor cli;
    std::istringstream in("ADD_USER 1 Alice\nEXIT\nADD_USER 2 Bob\n");
    std::ostringstream out;
    cli.run(in, out, /*echo=*/true, /*prompt=*/false);
    CHECK(cli.graph().userCount() == 1);
    CHECK(contains(out.str(), "> ADD_USER 1 Alice"));
}
