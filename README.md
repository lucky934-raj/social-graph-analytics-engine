# Social Graph Analytics Engine

A C++17 command-line tool that models a social network as an undirected graph
(users are vertices, friendships are edges) and answers questions about it:
mutual friends, friend recommendations, shortest paths and connectivity.

I built it as a data-structures-and-algorithms project. The core algorithms
(BFS, bidirectional BFS, Dijkstra, union-find, Jaccard ranking) are written
from scratch on top of the STL containers, with tests that check them against
simpler reference implementations.

## Features

- Add/remove users and friendships, with validation (duplicates, self-friendship,
  unknown users, non-positive weights)
- Mutual friends of two users
- Friend recommendations ranked by Jaccard similarity
- Shortest path by number of hops with BFS and with bidirectional BFS
- Minimum-cost path over weighted friendships with Dijkstra
- Connectivity queries and connected components with a disjoint set union (DSU)
- Graph statistics
- Save/load to a plain-text file
- Interactive CLI, or run a script of commands

## Build and run

Requires CMake 3.14+ and a C++17 compiler (tested with GCC 13.3 and Clang 18.1).

```bash
cmake -S . -B build
cmake --build build
./build/social_graph                          # interactive
./build/social_graph data/demo_commands.txt   # run the demo script (from the repo root)
```

Single-config generators (Makefiles, Ninja) default to a Release build. With
Visual Studio use `cmake --build build --config Release`; the binaries are then
under `build\Release\`.

Run the tests:

```bash
cd build && ctest --output-on-failure    # or ./build/unit_tests for per-test output
```

### Example session

Output from `data/demo_commands.txt` on `data/sample_graph.txt` (13 users):

```
> RECOMMEND 1 3
Recommendations for Aarav(1):
  1. Ananya(4)  jaccard=0.250 (1 mutual / 4 in union)
  2. Vikram(5)  jaccard=0.250 (1 mutual / 4 in union)
> BFS 1 10
Distance: 5
Path: Aarav(1) -> Rohan(3) -> Vikram(5) -> Kabir(7) -> Arjun(9) -> Diya(10)
Vertices expanded: 8
> DIJKSTRA 1 10
Cost: 8
Path: Aarav(1) -> Priya(2) -> Ananya(4) -> Meera(6) -> Isha(8) -> Diya(10)
Vertices expanded: 10
> COMPONENTS
3 connected component(s)
  #1 (10 user(s)): Aarav(1) Priya(2) Rohan(3) Ananya(4) Vikram(5) Meera(6) Kabir(7) Isha(8) Arjun(9) Diya(10)
  #2 (2 user(s)): Neel(11) Sara(12)
  #3 (1 user(s)): Tara(13)
```

BFS and Dijkstra return different routes here because BFS counts hops and
Dijkstra adds up weights. When several paths are equally short, BFS may return
any of them (see "Design notes").

### Commands

| Command | Description |
|---|---|
| `ADD_USER <id> <name>` | Add a user; the name may contain spaces |
| `REMOVE_USER <id>` | Remove a user and all their friendships |
| `FIND_USER <id>` | Show a user and their number of friends |
| `ADD_FRIEND <a> <b> [weight]` | Add a friendship (weight defaults to 1, must be > 0) |
| `REMOVE_FRIEND <a> <b>` | Remove a friendship |
| `ARE_FRIENDS <a> <b>` | Check for a direct friendship |
| `FRIENDS <id>` | List a user's friends |
| `MUTUAL <a> <b>` | Friends shared by two users |
| `RECOMMEND <id> [k]` | Top-k suggestions by Jaccard similarity (default k = 5) |
| `JACCARD <a> <b>` | Jaccard similarity of two users' friend sets |
| `BFS <a> <b>` | Fewest-hops path |
| `BIDIR_BFS <a> <b>` | Fewest-hops path using bidirectional BFS |
| `DIJKSTRA <a> <b>` | Minimum total-weight path |
| `CONNECTED <a> <b>` | Whether two users are in the same component |
| `COMPONENTS` | List connected components, largest first |
| `STATS` | Users, friendships, average/max degree, isolated users, components, largest component |
| `SAVE <file>` / `LOAD <file>` | Write / read the graph |
| `HELP`, `EXIT` | |

Commands are case-insensitive. Lines starting with `#` are comments. Invalid
input prints `Error: ...` and the CLI keeps running.

File format used by `SAVE`/`LOAD`:

```
# comment
USER 1 Aarav
USER 2 Priya
FRIEND 1 2 1        # a b [weight]
```

## Design

### Graph representation

```cpp
struct Node { User user; std::unordered_map<int, int> friends; };  // neighbor id -> weight
std::unordered_map<int, Node> nodes_;
```

- **Adjacency list, not a matrix.** Social graphs are sparse (E is much smaller
  than V^2). A matrix needs O(V^2) memory and O(V) time to list one user's
  friends; adjacency lists need O(V + E) memory and O(degree) time.
- **Hash maps keyed by user id.** Ids can be arbitrary (sparse, negative), and
  users can be removed, so a `vector` indexed by id would leave holes. Lookups,
  inserts and erases are O(1) on average. The trade-off is worse cache locality
  than a contiguous layout and O(n) worst case if hashing degrades.
- **One structure for weighted and unweighted use.** Every friendship stores a
  positive weight (default 1). BFS ignores it and Dijkstra uses it, so there is
  no second weighted copy of the graph to keep in sync.
- **User and adjacency list in one map entry**, so a user can never exist
  without an adjacency list. Each undirected edge is stored in both endpoints'
  maps.

### Error handling

One rule throughout `SocialGraph`: a `bool` return value means "did the graph
change?" (adding an existing friendship returns `false`), and
`std::invalid_argument` means the request makes no sense (unknown user,
self-friendship, weight <= 0, empty name). The CLI catches exceptions and
prints them.

### Modules

| File | Responsibility |
|---|---|
| `SocialGraph` | Users, friendships and invariants. Owns the data. |
| `GraphAlgorithms` | Mutual friends, BFS, bidirectional BFS, Dijkstra, statistics. Free functions taking `const SocialGraph&`. |
| `RecommendationEngine` | Jaccard similarity and top-k recommendations. |
| `DisjointSetUnion` | Union-find with path compression and union by size. |
| `Connectivity` | DSU snapshot of a graph: components and connectivity queries. |
| `GraphStorage` | Save/load the text format. |
| `TextParsing` | Strict argument parsing shared by the CLI and the loader. |
| `CommandProcessor` | The CLI. Works on any `istream`/`ostream`, so tests can drive it. |

The algorithms are free functions rather than classes because they keep no
state between calls. Classes are used where something owns state and
invariants: the graph, the DSU, the connectivity snapshot and the CLI.

### Design notes

- **Deterministic output.** `unordered_map` iteration order is unspecified, so
  anything printed as a list (friends, mutual friends, components,
  recommendations) is sorted explicitly. Shortest paths are the exception: when
  several are equally short, which one is returned depends on iteration order.
  The tests therefore check the distance and that the path is valid, not one
  specific path.
- **Connectivity cache.** A DSU can merge sets but cannot split them, so
  removing a friendship cannot be applied to an existing DSU. The CLI keeps a
  `Connectivity` snapshot and rebuilds it only when `SocialGraph::version()`
  (incremented on every change) differs from the version it was built from.
  Repeated `CONNECTED` queries on an unchanged graph cost O(alpha(V)) each.
- **Loading is all-or-nothing.** A file is parsed into a new graph, which only
  replaces the current one if the whole file is valid. Errors report the line
  number.

## Algorithms

**Mutual friends.** Walk the smaller of the two adjacency lists and look each id
up in the larger one, then sort the result.

**Jaccard recommendations.** J(A, B) = |N(A) ∩ N(B)| / |N(A) ∪ N(B)|. For a
user u, every friend-of-friend w that is not u and not already a friend is a
candidate. Walking `for f in N(u): for w in N(f)` reaches w once per mutual
friend, so counting visits gives |N(u) ∩ N(w)| directly. The union size then
follows from inclusion-exclusion: deg(u) + deg(w) − mutual. Users outside the
friends-of-friends set share no friends with u (J = 0) and are never suggested.

Candidates are ranked by Jaccard (descending), then mutual count (descending),
then user id (ascending). Jaccard values are compared exactly by
cross-multiplying (`a·d` vs `c·b` in `long long`) instead of comparing
`double`s. `std::partial_sort` selects the top k.

**BFS.** Queue plus a `parent` map that also serves as the visited set. BFS
reaches vertices in order of distance, so the first time the target is
discovered we already have a shortest path and can stop. The path is rebuilt by
following `parent` from the target back to the source and reversing.

**Bidirectional BFS.** Two searches, one from each end. Each round expands one
whole level of whichever side has the smaller frontier, and checks every newly
reached vertex against the other side's visited set. The first meeting vertex
gives a shortest path. Before the round, the two searches covered distances D1
and D2 with no overlap, so the shortest path L > D1 + D2. A meeting found now
gives a path of length at most D1 + 1 + D2 <= L, so it equals L. Expanding one
vertex at a time instead of one level can return a longer path; a randomized
test catches that variant. The path is the forward parent chain to the meeting
vertex followed by the backward chain to the target.

With branching factor b and distance d, plain BFS explores about b^d vertices
and bidirectional BFS about 2·b^(d/2). There is no such gain when the frontier
does not grow (for example on a path), and the worst case is O(V + E) either
way.

**Dijkstra.** `std::priority_queue` turned into a min-heap with
`std::greater`. There is no decrease-key, so an improved distance is pushed as a
new entry and stale entries are skipped when popped (lazy deletion). The search
stops when the target is popped, because with non-negative weights a popped
vertex's distance is final. Distances are `long long`; weights are `int`.

Negative weights are rejected when a friendship is added. Dijkstra assumes a
settled vertex can never be improved, and a negative edge found later could
break that. With undirected edges a single negative edge would also create a
negative cycle (going back and forth), so shortest paths would not be defined.

**Disjoint Set Union.** `parent` and `size` arrays over dense indices
0..n−1. `Connectivity` maps user ids to indices first, since ids can be sparse
or negative.
- `find` uses path compression: after locating the root, every vertex on the
  walked path is pointed straight at it. It is iterative, so there is no deep
  recursion.
- `unite` uses union by size: the smaller tree is attached under the larger
  one, which keeps trees O(log n) deep on its own.

Together these give O(α(n)) amortized time per operation, where α is the
inverse Ackermann function. α(n) ≤ 4 for any n that fits in memory.

## Complexity

V = users, E = friendships, d(x) = degree of x. Hash map operations are
average-case O(1).

| Operation | Approach | Time |
|---|---|---|
| Add user / find user / user exists | hash map | O(1) |
| Remove user | erase from each neighbor's map | O(d(u)) |
| Add / remove / check friendship | two hash map updates | O(1) |
| List friends (`FRIENDS`) | copy + sort | O(d log d) |
| Mutual friends | iterate smaller list, probe larger | O(min(d(a), d(b)) + m log m), m = result size |
| Jaccard of two users | same as mutual friends, no sort | O(min(d(a), d(b))) |
| Recommendations | friends-of-friends counting + `partial_sort` | O(S + C log k), S = Σ d(f) over friends f of u, C = candidates |
| BFS | queue + parent map | O(V + E) time, O(V) space |
| Bidirectional BFS | two level-synchronous searches | O(V + E) worst case |
| Dijkstra | binary heap with lazy deletion | O((V + E) log V) time, O(V + E) space |
| DSU find / unite | path compression + union by size | O(α(V)) amortized |
| Build connectivity snapshot | sort ids + one unite per edge | O(V log V + E·α(V)) |
| `CONNECTED` on an unchanged graph | cached DSU | O(α(V)) |
| `COMPONENTS` | group by root + sort | O(V log V) |
| Statistics | degrees + connectivity snapshot | O(V log V + E·α(V)) |
| Save | sorted users and edges | O(V log V + E log E) |
| Load | one insert per line | O(V + E) |

The graph itself uses O(V + E) memory (each edge stored twice).

On Dijkstra: with lazy deletion the heap can hold up to O(E) entries, so the
bound is O(E log E). Since E ≤ V², log E ≤ 2 log V, so this is the same as
O((V + E) log V).

## Testing

`tests/` uses a small self-written framework (`TestFramework.h`, under 70
lines: `TEST_CASE`, `CHECK`, `CHECK_THROWS`) to avoid external dependencies.
The test executable is registered with CTest with a 60-second timeout, so an
accidental infinite loop fails instead of hanging. At the time of writing there
are 87 test cases (330 checks), all passing with GCC 13.3 and Clang 18.1, with zero
warnings under `-Wall -Wextra -Wpedantic`. They also pass under
AddressSanitizer and UndefinedBehaviorSanitizer.

Besides hand-built cases (duplicates, self-friendship, unknown users,
unreachable users, empty and single-user graphs, removed and re-used ids,
`INT_MIN`/`INT_MAX` ids, large stars and long paths), three tests compare
against independent reference implementations on random graphs:

- bidirectional BFS vs plain BFS, on every pair of users in 300 random graphs
- Dijkstra vs Floyd–Warshall, on every pair in 200 random weighted graphs
- DSU connectivity vs BFS reachability, on 100 random graphs

While developing, I checked that these tests catch real bugs by planting
mistakes: expanding one vertex per turn in bidirectional BFS, and stopping
Dijkstra when the target is pushed instead of popped. Both made tests fail.

## Benchmark: BFS vs bidirectional BFS

`bench/bfs_benchmark.cpp` (built as `bfs_benchmark`, not part of the tests):

```bash
cmake -S . -B build && cmake --build build && ./build/bfs_benchmark
```

How it works:
- Graphs and queries come from `std::mt19937` with fixed seeds. Its output is
  fixed by the C++ standard; `std::uniform_int_distribution` is not, so it
  isn't used.
- Each graph gets the same 200 random (source, target) pairs for both
  algorithms. The order alternates per query so neither always runs on warm
  caches.
- Every query checks that both algorithms return the same distance.
- It reports average wall time per query (`steady_clock`) and the average
  number of vertices expanded.

Results from one run on a 2-vCPU cloud VM (Intel Xeon @ 2.10 GHz), GCC 13.3,
Release build (`-O3`):

| Graph | Users | Friendships | Avg distance | BFS ms/query | Bidir ms/query | BFS vertices expanded | Bidir vertices expanded |
|---|---|---|---|---|---|---|---|
| Random sparse G(n, m), average degree 8 | 200,000 | 800,000 | 6.1 | 45.0 | 0.24 | 24,733 | 112 |
| 400 × 400 grid | 160,000 | 319,200 | 263.0 | 26.2 | 19.7 | 78,495 | 53,254 |
| Path | 100,000 | 99,999 | 35,122 | 3.28 | 4.63 | 52,160 | 52,159 |

How to read these:
- **Random sparse graph:** the number of vertices within distance d grows
  roughly like 8^d, so meeting in the middle saves the most. Bidirectional BFS
  expanded about 220 times fewer vertices.
- **Grid:** the frontier grows only linearly with distance, so the gain is
  small (about 1.3×).
- **Path:** the frontier never grows. Both algorithms expand the same vertices,
  and bidirectional BFS is slower because of the bookkeeping for two searches.
  It is not always faster.

Timings vary between runs (a second run differed by up to about 12%) and
between machines. The expanded-vertex counts do not change between runs on the
same toolchain. They may differ slightly with another standard library,
because BFS stops as soon as it finds the target, and which neighbor it looks
at first depends on `unordered_map` iteration order.

## Limitations and possible improvements

- **Memory layout.** For millions of users, hash maps of hash maps are
  memory-heavy and cache-unfriendly. Mapping ids to dense indices and storing
  adjacency in compressed sparse row (CSR) form would be far more compact.
  `vector` visited/parent arrays could then replace the per-query hash maps in
  BFS and Dijkstra.
- **Incremental connectivity.** Adding a friendship could simply `unite` into
  the cached DSU; only removals need a rebuild. The current code rebuilds after
  any change.
- **Concurrency.** Queries only read the graph, so a `std::shared_mutex` would
  allow many concurrent readers with exclusive writers. The connectivity cache
  would need its own lock, or be built per request.
- **Recommendations at scale.** Hub users make the friends-of-friends walk
  expensive. Sampling neighbors or precomputing candidates offline would help.
  Other scores such as Adamic–Adar could be compared with Jaccard.
- **Persistence.** The text format is fine for small graphs. A real system
  would use a database, for example a `users` table and a `friendships(a, b,
  weight)` table with an index on each endpoint.

## Project layout

```
include/   headers (SocialGraph, GraphAlgorithms, RecommendationEngine,
           DisjointSetUnion, Connectivity, GraphStorage, TextParsing, CommandProcessor, User)
src/       implementations + main.cpp
tests/     test framework and test files
bench/     BFS vs bidirectional BFS benchmark
data/      sample graph and demo command script
```
