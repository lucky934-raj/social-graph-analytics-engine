# Social Graph Analytics Engine

A C++17 command-line tool that models a social network as an undirected graph
(users are vertices, friendships are edges) and runs graph algorithms on it:
mutual friends, Jaccard-based friend recommendations, BFS and bidirectional BFS
shortest paths, Dijkstra on weighted connections, and DSU-based connectivity.

Work in progress — features are added one at a time.

## Build

Requires CMake 3.14+ and a C++17 compiler (GCC, Clang or MSVC).

```bash
cmake -S . -B build
cmake --build build
./build/social_graph
```
