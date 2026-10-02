#include "GraphStorage.h"

#include "TextParsing.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace GraphStorage {

void save(const SocialGraph& graph, std::ostream& out) {
    std::vector<int> ids = graph.userIds();
    out << "# Social graph: " << graph.userCount() << " users, " << graph.friendshipCount()
        << " friendships\n";
    for (int id : ids) {
        out << "USER " << id << ' ' << graph.findUser(id)->name << '\n';
    }

    std::vector<std::tuple<int, int, int>> edges;
    edges.reserve(graph.friendshipCount());
    for (int id : ids) {
        for (const auto& edge : graph.neighbors(id)) {
            if (id < edge.first) {  // each undirected edge is stored twice
                edges.emplace_back(id, edge.first, edge.second);
            }
        }
    }
    std::sort(edges.begin(), edges.end());
    for (const auto& [a, b, weight] : edges) {
        out << "FRIEND " << a << ' ' << b << ' ' << weight << '\n';
    }
}

SocialGraph load(std::istream& in) {
    SocialGraph graph;
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        std::istringstream args(line);
        std::string record;
        if (!(args >> record) || record[0] == '#') {
            continue;
        }
        try {
            if (record == "USER") {
                int id = TextParsing::readInt(args, "user id");
                std::string name = TextParsing::readRestOfLine(args);
                if (!graph.addUser(id, name)) {
                    throw std::invalid_argument("duplicate user " + std::to_string(id));
                }
            } else if (record == "FRIEND") {
                int a = TextParsing::readInt(args, "first user id");
                int b = TextParsing::readInt(args, "second user id");
                int weight = TextParsing::hasMoreArgs(args)
                                 ? TextParsing::readInt(args, "weight")
                                 : 1;
                TextParsing::expectNoMoreArgs(args);
                if (!graph.addFriendship(a, b, weight)) {
                    throw std::invalid_argument("duplicate friendship " + std::to_string(a) +
                                                " - " + std::to_string(b));
                }
            } else {
                throw std::invalid_argument("unknown record type '" + record + "'");
            }
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error("line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
    return graph;
}

void saveToFile(const SocialGraph& graph, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot open '" + path + "' for writing");
    }
    save(graph, out);
    out.flush();
    if (!out) {
        throw std::runtime_error("error while writing '" + path + "'");
    }
}

SocialGraph loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open '" + path + "'");
    }
    return load(in);
}

}  // namespace GraphStorage
