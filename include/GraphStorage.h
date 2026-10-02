#pragma once

#include "SocialGraph.h"

#include <iosfwd>
#include <string>

// Plain-text graph format, one record per line:
//
//   # comment
//   USER <id> <name, may contain spaces>
//   FRIEND <a> <b> [weight]      (weight defaults to 1)
//
// A FRIEND line may only refer to users declared on an earlier line.
namespace GraphStorage {

// Writes users sorted by id, then each friendship once (smaller id first).
// O(V log V + E log E) because of the sorting, which keeps files diff-friendly.
void save(const SocialGraph& graph, std::ostream& out);

// Parses a whole graph. Throws std::runtime_error("line N: ...") on the first
// invalid line; nothing is returned in that case, so a caller's existing graph
// is never left half-loaded. O(V + E) average.
SocialGraph load(std::istream& in);

void saveToFile(const SocialGraph& graph, const std::string& path);
SocialGraph loadFromFile(const std::string& path);

}  // namespace GraphStorage
