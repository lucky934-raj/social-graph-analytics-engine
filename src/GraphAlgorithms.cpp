#include "GraphAlgorithms.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

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

namespace {

// State of one direction of the bidirectional search.
struct SearchSide {
    std::unordered_map<int, int> parent;  // also the visited set
    std::vector<int> frontier;            // vertices at the deepest level so far

    explicit SearchSide(int start) : parent{{start, start}}, frontier{start} {}
};

// Expands every vertex in side.frontier by one level. Returns a vertex that
// is now reached by both searches, or -1 if the searches have not met yet.
//
// Why the first meeting is a shortest path: before this call, `side` has
// reached everything within distance D1 of its start and `other` everything
// within D2 of its start, with no vertex in common, so the shortest path has
// length L > D1 + D2. A meeting vertex v found now is D1 + 1 steps from this
// side's start and at most D2 from the other's, giving a path of length
// <= D1 + 1 + D2 <= L. No path is shorter than L, so it must equal L.
int expandLevel(const SocialGraph& graph, SearchSide& side, const SearchSide& other,
                std::size_t& expanded) {
    std::vector<int> nextFrontier;
    for (int current : side.frontier) {
        ++expanded;
        for (const auto& edge : graph.neighbors(current)) {
            int next = edge.first;
            if (side.parent.count(next) != 0) {
                continue;
            }
            side.parent[next] = current;
            if (other.parent.count(next) != 0) {
                return next;
            }
            nextFrontier.push_back(next);
        }
    }
    side.frontier = std::move(nextFrontier);
    return -1;
}

}  // namespace

PathResult bidirectionalBfs(const SocialGraph& graph, int source, int target) {
    requireUser(graph, source);
    requireUser(graph, target);

    PathResult result;
    if (source == target) {
        result.found = true;
        result.distance = 0;
        result.path = {source};
        return result;
    }

    SearchSide forward(source);
    SearchSide backward(target);
    int meeting = -1;

    // If either frontier runs empty, that side has explored its whole
    // component without meeting the other, so no path exists.
    while (meeting == -1 && !forward.frontier.empty() && !backward.frontier.empty()) {
        // Expanding the smaller frontier keeps the two search trees balanced,
        // which is where the b^(d/2) saving comes from.
        if (forward.frontier.size() <= backward.frontier.size()) {
            meeting = expandLevel(graph, forward, backward, result.expanded);
        } else {
            meeting = expandLevel(graph, backward, forward, result.expanded);
        }
    }
    if (meeting == -1) {
        return result;
    }

    // source ... meeting from the forward tree, then meeting ... target by
    // following the backward tree's parent links (they point towards target).
    result.path = walkBack(forward.parent, source, meeting);
    for (int v = meeting; v != target;) {
        v = backward.parent.at(v);
        result.path.push_back(v);
    }
    result.found = true;
    result.distance = static_cast<long long>(result.path.size()) - 1;
    return result;
}

PathResult dijkstra(const SocialGraph& graph, int source, int target) {
    requireUser(graph, source);
    requireUser(graph, target);

    // (distance, vertex); std::greater turns the default max-heap into a min-heap.
    using HeapEntry = std::pair<long long, int>;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>> heap;
    std::unordered_map<int, long long> dist;
    std::unordered_map<int, int> parent;

    dist[source] = 0;
    parent[source] = source;
    heap.push({0, source});

    PathResult result;
    while (!heap.empty()) {
        auto [d, current] = heap.top();
        heap.pop();
        // std::priority_queue has no decrease-key, so a vertex is pushed again
        // whenever its distance improves. Older entries are stale; skip them.
        if (d > dist.at(current)) {
            continue;
        }
        ++result.expanded;
        // With non-negative weights a vertex's distance is final once it is
        // popped, so we can stop as soon as the target comes off the heap.
        if (current == target) {
            break;
        }
        for (const auto& edge : graph.neighbors(current)) {
            int next = edge.first;
            long long candidate = d + edge.second;
            auto it = dist.find(next);
            if (it == dist.end() || candidate < it->second) {
                dist[next] = candidate;
                parent[next] = current;
                heap.push({candidate, next});
            }
        }
    }

    auto it = dist.find(target);
    if (it == dist.end()) {
        return result;
    }
    result.found = true;
    result.distance = it->second;
    result.path = walkBack(parent, source, target);
    return result;
}

}  // namespace GraphAlgorithms
