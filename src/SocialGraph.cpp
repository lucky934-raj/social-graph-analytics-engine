#include "SocialGraph.h"

#include <algorithm>
#include <stdexcept>

bool SocialGraph::addUser(int id, const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("user name cannot be empty");
    }
    if (nodes_.count(id) != 0) {
        return false;
    }
    nodes_.emplace(id, Node{User{id, name}, {}});
    ++version_;
    return true;
}

bool SocialGraph::removeUser(int id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        return false;
    }
    // Edges are stored in both directions, so each neighbour still points back
    // to this user. Only the inner maps are modified here, which keeps `it` valid.
    for (const auto& entry : it->second.friends) {
        nodes_.at(entry.first).friends.erase(id);
    }
    edgeCount_ -= it->second.friends.size();
    nodes_.erase(it);
    ++version_;
    return true;
}

const User* SocialGraph::findUser(int id) const {
    auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second.user;
}

bool SocialGraph::userExists(int id) const {
    return nodes_.count(id) != 0;
}

bool SocialGraph::addFriendship(int a, int b, int weight) {
    if (a == b) {
        throw std::invalid_argument("a user cannot be friends with themselves");
    }
    if (weight <= 0) {
        throw std::invalid_argument("friendship weight must be positive");
    }
    Node& first = node(a);
    Node& second = node(b);
    if (first.friends.count(b) != 0) {
        return false;
    }
    first.friends[b] = weight;
    second.friends[a] = weight;
    ++edgeCount_;
    ++version_;
    return true;
}

bool SocialGraph::removeFriendship(int a, int b) {
    Node& first = node(a);
    Node& second = node(b);
    if (first.friends.erase(b) == 0) {
        return false;
    }
    second.friends.erase(a);
    --edgeCount_;
    ++version_;
    return true;
}

bool SocialGraph::areFriends(int a, int b) const {
    auto it = nodes_.find(a);
    return it != nodes_.end() && it->second.friends.count(b) != 0;
}

const SocialGraph::NeighborMap& SocialGraph::neighbors(int id) const {
    return node(id).friends;
}

std::size_t SocialGraph::degree(int id) const {
    return node(id).friends.size();
}

std::vector<int> SocialGraph::userIds() const {
    std::vector<int> ids;
    ids.reserve(nodes_.size());
    for (const auto& entry : nodes_) {
        ids.push_back(entry.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

SocialGraph::Node& SocialGraph::node(int id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        throw std::invalid_argument("user " + std::to_string(id) + " does not exist");
    }
    return it->second;
}

const SocialGraph::Node& SocialGraph::node(int id) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        throw std::invalid_argument("user " + std::to_string(id) + " does not exist");
    }
    return it->second;
}
