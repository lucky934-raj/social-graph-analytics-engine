#pragma once

#include "User.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Undirected social graph stored as an adjacency list.
//
// Every friendship is an edge with a positive weight (default 1). BFS-based
// algorithms ignore the weight; Dijkstra uses it. Keeping a single structure
// avoids maintaining a separate weighted copy of the graph.
//
// Error convention:
//   - a bool return value says whether the graph was changed
//     (e.g. adding an existing friendship returns false);
//   - std::invalid_argument is thrown when a request makes no sense
//     (unknown user, self-friendship, non-positive weight, empty name).
class SocialGraph {
public:
    using NeighborMap = std::unordered_map<int, int>;  // neighbor id -> edge weight

    bool addUser(int id, const std::string& name);
    bool removeUser(int id);
    const User* findUser(int id) const;  // nullptr if the user does not exist
    bool userExists(int id) const;

    bool addFriendship(int a, int b, int weight = 1);
    bool removeFriendship(int a, int b);
    bool areFriends(int a, int b) const;  // false if either user is unknown

    const NeighborMap& neighbors(int id) const;
    std::size_t degree(int id) const;

    std::size_t userCount() const { return nodes_.size(); }
    std::size_t friendshipCount() const { return edgeCount_; }
    std::vector<int> userIds() const;  // sorted ascending

    // Incremented on every change to the graph. Lets callers that cache
    // derived data (e.g. a connectivity snapshot) tell when it is stale.
    std::uint64_t version() const { return version_; }

private:
    struct Node {
        User user;
        NeighborMap friends;
    };

    Node& node(int id);
    const Node& node(int id) const;

    // One map holds both the user record and its adjacency list, so a user
    // can never exist without an adjacency list (or the other way round).
    std::unordered_map<int, Node> nodes_;
    std::size_t edgeCount_ = 0;
    std::uint64_t version_ = 0;
};
