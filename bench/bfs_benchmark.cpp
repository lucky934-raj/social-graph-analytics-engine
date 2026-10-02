// Compares BFS and bidirectional BFS on generated graphs.
//
// Graphs come from std::mt19937 with fixed seeds. mt19937's output sequence is
// fixed by the C++ standard (unlike std::uniform_int_distribution), so every
// platform generates the same graphs and the same queries.
//
// For each graph the same random (source, target) pairs are answered by both
// algorithms. We report average time per query and average number of vertices
// expanded. Results depend heavily on graph shape: bidirectional search helps
// most when the number of vertices within distance d grows quickly with d.

#include "GraphAlgorithms.h"
#include "SocialGraph.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

int randomBelow(std::mt19937& rng, int n) {
    return static_cast<int>(rng() % static_cast<std::uint32_t>(n));
}

SocialGraph usersOnly(int n) {
    SocialGraph g;
    for (int id = 0; id < n; ++id) {
        g.addUser(id, "user" + std::to_string(id));
    }
    return g;
}

// n users with `edges` distinct random friendships (Erdos-Renyi G(n, m)).
SocialGraph randomGraph(int n, int edges, std::mt19937& rng) {
    SocialGraph g = usersOnly(n);
    while (static_cast<int>(g.friendshipCount()) < edges) {
        int a = randomBelow(rng, n);
        int b = randomBelow(rng, n);
        if (a != b) {
            g.addFriendship(a, b);  // returns false for duplicates, which we just retry
        }
    }
    return g;
}

SocialGraph gridGraph(int side) {
    SocialGraph g = usersOnly(side * side);
    for (int r = 0; r < side; ++r) {
        for (int c = 0; c < side; ++c) {
            int id = r * side + c;
            if (c + 1 < side) {
                g.addFriendship(id, id + 1);
            }
            if (r + 1 < side) {
                g.addFriendship(id, id + side);
            }
        }
    }
    return g;
}

SocialGraph pathGraph(int n) {
    SocialGraph g = usersOnly(n);
    for (int id = 0; id + 1 < n; ++id) {
        g.addFriendship(id, id + 1);
    }
    return g;
}

struct Totals {
    double milliseconds = 0;
    double expanded = 0;
};

template <typename Search>
long long timeQuery(Search search, const SocialGraph& g, int s, int t, Totals& totals) {
    auto start = std::chrono::steady_clock::now();
    PathResult r = search(g, s, t);
    auto end = std::chrono::steady_clock::now();
    totals.milliseconds += std::chrono::duration<double, std::milli>(end - start).count();
    totals.expanded += static_cast<double>(r.expanded);
    return r.distance;
}

void runBenchmark(const std::string& name, const SocialGraph& g, int queries, std::uint32_t seed) {
    std::mt19937 rng(seed);
    int n = static_cast<int>(g.userCount());
    Totals bfs;
    Totals bidir;
    double totalDistance = 0;
    int reachable = 0;
    int disagreements = 0;

    for (int q = 0; q < queries; ++q) {
        int s = randomBelow(rng, n);
        int t = randomBelow(rng, n);
        // Alternate which algorithm runs first so neither always gets warm caches.
        long long d1;
        long long d2;
        if (q % 2 == 0) {
            d1 = timeQuery(GraphAlgorithms::bfsShortestPath, g, s, t, bfs);
            d2 = timeQuery(GraphAlgorithms::bidirectionalBfs, g, s, t, bidir);
        } else {
            d2 = timeQuery(GraphAlgorithms::bidirectionalBfs, g, s, t, bidir);
            d1 = timeQuery(GraphAlgorithms::bfsShortestPath, g, s, t, bfs);
        }
        if (d1 != d2) {
            ++disagreements;
        }
        if (d1 >= 0) {
            ++reachable;
            totalDistance += static_cast<double>(d1);
        }
    }

    std::cout << name << "\n"
              << "  users: " << g.userCount() << ", friendships: " << g.friendshipCount()
              << ", queries: " << queries << " (" << reachable << " reachable, avg distance "
              << std::fixed << std::setprecision(1)
              << (reachable > 0 ? totalDistance / reachable : 0.0) << ")\n";
    auto printRow = [](const std::string& label, const std::string& ms,
                       const std::string& expanded) {
        std::cout << "  " << std::left << std::setw(20) << label << std::right << std::setw(16)
                  << ms << std::setw(26) << expanded << '\n';
    };
    auto fixed = [](double value, int decimals) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << value;
        return ss.str();
    };
    printRow("algorithm", "avg ms/query", "avg vertices expanded");
    printRow("BFS", fixed(bfs.milliseconds / queries, 3), fixed(bfs.expanded / queries, 0));
    printRow("Bidirectional BFS", fixed(bidir.milliseconds / queries, 3),
             fixed(bidir.expanded / queries, 0));
    if (disagreements != 0) {
        std::cout << "  ERROR: algorithms disagreed on " << disagreements << " queries\n";
    }
    std::cout << '\n';
}

}  // namespace

int main() {
#ifndef NDEBUG
    std::cout << "Warning: built without optimisation; timings are not representative.\n\n";
#endif
    const int queries = 200;

    std::mt19937 graphRng(42);
    SocialGraph random = randomGraph(200000, 800000, graphRng);  // average degree 8
    runBenchmark("Random sparse graph (G(n, m), average degree 8)", random, queries, 1);

    SocialGraph grid = gridGraph(400);
    runBenchmark("400 x 400 grid", grid, queries, 2);

    SocialGraph path = pathGraph(100000);
    runBenchmark("Path graph", path, queries, 3);
    return 0;
}
