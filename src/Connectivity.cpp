#include "Connectivity.h"

#include <algorithm>
#include <stdexcept>
#include <string>

Connectivity::Connectivity(const SocialGraph& graph)
    : idByIndex_(graph.userIds()), dsu_(idByIndex_.size()), graphVersion_(graph.version()) {
    // User ids can be arbitrary (sparse, negative), while the DSU works on
    // 0..n-1, so give every user a dense index first.
    for (std::size_t i = 0; i < idByIndex_.size(); ++i) {
        indexById_[idByIndex_[i]] = static_cast<int>(i);
    }
    for (int user : idByIndex_) {
        for (const auto& edge : graph.neighbors(user)) {
            // Each undirected edge is stored twice; unite it only once.
            if (user < edge.first) {
                dsu_.unite(indexById_.at(user), indexById_.at(edge.first));
            }
        }
    }
}

bool Connectivity::connected(int a, int b) {
    return dsu_.connected(indexOf(a), indexOf(b));
}

std::size_t Connectivity::largestComponentSize() {
    int largest = 0;
    for (std::size_t i = 0; i < idByIndex_.size(); ++i) {
        largest = std::max(largest, dsu_.setSize(static_cast<int>(i)));
    }
    return static_cast<std::size_t>(largest);
}

std::vector<std::vector<int>> Connectivity::components() {
    std::unordered_map<int, std::size_t> slotOfRoot;
    std::vector<std::vector<int>> result;
    // idByIndex_ is sorted, so each component's list comes out sorted too.
    for (std::size_t i = 0; i < idByIndex_.size(); ++i) {
        int root = dsu_.find(static_cast<int>(i));
        auto it = slotOfRoot.find(root);
        if (it == slotOfRoot.end()) {
            it = slotOfRoot.emplace(root, result.size()).first;
            result.emplace_back();
        }
        result[it->second].push_back(idByIndex_[i]);
    }
    std::sort(result.begin(), result.end(),
              [](const std::vector<int>& x, const std::vector<int>& y) {
                  if (x.size() != y.size()) {
                      return x.size() > y.size();
                  }
                  return x.front() < y.front();
              });
    return result;
}

int Connectivity::indexOf(int userId) const {
    auto it = indexById_.find(userId);
    if (it == indexById_.end()) {
        throw std::invalid_argument("user " + std::to_string(userId) + " does not exist");
    }
    return it->second;
}
