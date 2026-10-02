#include "GraphAlgorithms.h"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace GraphAlgorithms {

namespace {

void requireUser(const SocialGraph& graph, int id) {
    if (!graph.userExists(id)) {
        throw std::invalid_argument("user " + std::to_string(id) + " does not exist");
    }
}

// Follows parent links from target back to source and reverses the result.
std::vector<int> walkBack(const std::unordered_map<int, int>& parent, int source, int target) {
    std::vector<int> path{target};
    int current = target;
    while (current != source) {
        current = parent.at(current);
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

}  // namespace

std::vector<int> mutualFriends(const SocialGraph& graph, int a, int b) {
    const auto* smaller = &graph.neighbors(a);
    const auto* larger = &graph.neighbors(b);
    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    std::vector<int> result;
    for (const auto& entry : *smaller) {
        if (larger->count(entry.first) != 0) {
            result.push_back(entry.first);
        }
    }
    // Hash map iteration order is unspecified; sort so output is deterministic.
    std::sort(result.begin(), result.end());
    return result;
}

PathResult bfsShortestPath(const SocialGraph& graph, int source, int target) {
    requireUser(graph, source);
    requireUser(graph, target);

    PathResult result;
    if (source == target) {
        result.found = true;
        result.distance = 0;
        result.path = {source};
        return result;
    }

    // parent doubles as the visited set: a vertex is visited once it has a parent.
    std::unordered_map<int, int> parent;
    parent[source] = source;
    std::queue<int> frontier;
    frontier.push(source);

    while (!frontier.empty()) {
        int current = frontier.front();
        frontier.pop();
        ++result.expanded;

        for (const auto& edge : graph.neighbors(current)) {
            int next = edge.first;
            if (parent.count(next) != 0) {
                continue;
            }
            parent[next] = current;
            // BFS discovers vertices in order of distance, so the first time we
            // see the target we already have a shortest path; no need to keep going.
            if (next == target) {
                result.found = true;
                result.path = walkBack(parent, source, target);
                result.distance = static_cast<long long>(result.path.size()) - 1;
                return result;
            }
            frontier.push(next);
        }
    }
    return result;
}

}  // namespace GraphAlgorithms
