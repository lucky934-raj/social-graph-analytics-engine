#include "DisjointSetUnion.h"

#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

DisjointSetUnion::DisjointSetUnion(std::size_t n) : parent_(n), size_(n, 1), setCount_(n) {
    std::iota(parent_.begin(), parent_.end(), 0);  // every element starts as its own root
}

int DisjointSetUnion::find(int x) {
    checkIndex(x);
    int root = x;
    while (parent_[root] != root) {
        root = parent_[root];
    }
    // Path compression: point every vertex on the walked path straight at the
    // root, so later finds on any of them take one step. Done iteratively to
    // avoid deep recursion.
    while (x != root) {
        int next = parent_[x];
        parent_[x] = root;
        x = next;
    }
    return root;
}

bool DisjointSetUnion::unite(int a, int b) {
    int rootA = find(a);
    int rootB = find(b);
    if (rootA == rootB) {
        return false;
    }
    // Union by size: hang the smaller tree under the larger one. A vertex's
    // depth only grows when its tree at least doubles in size, which keeps
    // trees O(log n) deep even before path compression.
    if (size_[rootA] < size_[rootB]) {
        std::swap(rootA, rootB);
    }
    parent_[rootB] = rootA;
    size_[rootA] += size_[rootB];
    --setCount_;
    return true;
}

void DisjointSetUnion::checkIndex(int x) const {
    if (x < 0 || static_cast<std::size_t>(x) >= parent_.size()) {
        throw std::out_of_range("DSU index " + std::to_string(x) + " out of range");
    }
}
