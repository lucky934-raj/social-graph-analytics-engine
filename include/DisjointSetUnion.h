#pragma once

#include <cstddef>
#include <vector>

// Disjoint Set Union (union-find) over elements 0..n-1, with path compression
// and union by size. Any sequence of m operations takes O(m * alpha(n)) time,
// where alpha is the inverse Ackermann function (at most 4 for any n that
// fits in memory), so each operation is effectively constant on average.
class DisjointSetUnion {
public:
    explicit DisjointSetUnion(std::size_t n);

    // Representative of x's set. Compresses the path it walks.
    int find(int x);

    // Merges the sets of a and b. Returns false if they were already together.
    bool unite(int a, int b);

    bool connected(int a, int b) { return find(a) == find(b); }
    int setSize(int x) { return size_[find(x)]; }

    std::size_t elementCount() const { return parent_.size(); }
    std::size_t setCount() const { return setCount_; }

private:
    void checkIndex(int x) const;

    std::vector<int> parent_;
    std::vector<int> size_;  // only meaningful for roots
    std::size_t setCount_;
};
