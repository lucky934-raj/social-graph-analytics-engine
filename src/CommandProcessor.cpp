#include "CommandProcessor.h"

#include "GraphAlgorithms.h"
#include "RecommendationEngine.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

int readInt(std::istringstream& args, const std::string& what) {
    std::string token;
    if (!(args >> token)) {
        throw std::invalid_argument("missing " + what);
    }
    std::size_t used = 0;
    int value = 0;
    try {
        value = std::stoi(token, &used);
    } catch (const std::exception&) {
        used = 0;
    }
    if (used == 0 || used != token.size()) {
        throw std::invalid_argument("expected an integer for " + what + ", got '" + token + "'");
    }
    return value;
}

bool hasMoreArgs(std::istringstream& args) {
    args >> std::ws;
    return !args.eof();
}

void expectNoMoreArgs(std::istringstream& args) {
    if (hasMoreArgs(args)) {
        throw std::invalid_argument("too many arguments");
    }
}

std::string readRestOfLine(std::istringstream& args) {
    std::string rest;
    std::getline(args >> std::ws, rest);
    while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.back()))) {
        rest.pop_back();
    }
    return rest;
}

// Formats with a fixed number of decimals without changing the state of the
// caller's output stream (std::fixed / setprecision are sticky).
std::string formatDecimal(double value, int decimals = 3) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << value;
    return ss.str();
}

std::string toUpper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

const char* kHelpText =
    "Commands:\n"
    "  ADD_USER <id> <name>          add a user (name may contain spaces)\n"
    "  REMOVE_USER <id>              remove a user and all their friendships\n"
    "  FIND_USER <id>                show a user\n"
    "  ADD_FRIEND <a> <b> [weight]   add a friendship (weight defaults to 1)\n"
    "  REMOVE_FRIEND <a> <b>         remove a friendship\n"
    "  ARE_FRIENDS <a> <b>           check whether two users are friends\n"
    "  FRIENDS <id>                  list a user's friends\n"
    "  MUTUAL <a> <b>                list friends shared by two users\n"
    "  RECOMMEND <id> [k]            top-k friend suggestions by Jaccard similarity (k=5)\n"
    "  JACCARD <a> <b>               Jaccard similarity of two users' friend sets\n"
    "  HELP                          show this message\n"
    "  EXIT                          quit\n";

}  // namespace

bool CommandProcessor::execute(const std::string& line, std::ostream& out) {
    std::istringstream args(line);
    std::string command;
    if (!(args >> command) || command[0] == '#') {
        return true;  // blank line or comment
    }
    command = toUpper(command);
    if (command == "EXIT" || command == "QUIT") {
        return false;
    }
    try {
        dispatch(command, args, out);
    } catch (const std::exception& e) {
        out << "Error: " << e.what() << '\n';
    }
    return true;
}

void CommandProcessor::run(std::istream& in, std::ostream& out, bool echo, bool prompt) {
    std::string line;
    while (true) {
        if (prompt) {
            out << "> " << std::flush;
        }
        if (!std::getline(in, line)) {
            break;
        }
        if (echo && !line.empty()) {
            out << "> " << line << '\n';
        }
        if (!execute(line, out)) {
            break;
        }
    }
}

void CommandProcessor::dispatch(const std::string& command, std::istringstream& args,
                                std::ostream& out) {
    if (command == "ADD_USER") {
        int id = readInt(args, "user id");
        std::string name = readRestOfLine(args);
        if (graph_.addUser(id, name)) {
            out << "Added user " << label(id) << '\n';
        } else {
            out << "User " << id << " already exists\n";
        }
    } else if (command == "REMOVE_USER") {
        int id = readInt(args, "user id");
        expectNoMoreArgs(args);
        std::string name = graph_.userExists(id) ? label(id) : std::to_string(id);
        if (graph_.removeUser(id)) {
            out << "Removed user " << name << '\n';
        } else {
            out << "User " << id << " does not exist\n";
        }
    } else if (command == "FIND_USER") {
        int id = readInt(args, "user id");
        expectNoMoreArgs(args);
        if (graph_.userExists(id)) {
            out << label(id) << ", " << graph_.degree(id) << " friend(s)\n";
        } else {
            out << "User " << id << " not found\n";
        }
    } else if (command == "ADD_FRIEND") {
        int a = readInt(args, "first user id");
        int b = readInt(args, "second user id");
        int weight = hasMoreArgs(args) ? readInt(args, "weight") : 1;
        expectNoMoreArgs(args);
        if (graph_.addFriendship(a, b, weight)) {
            out << "Added friendship " << label(a) << " - " << label(b);
            if (weight != 1) {
                out << " (weight " << weight << ")";
            }
            out << '\n';
        } else {
            out << label(a) << " and " << label(b) << " are already friends\n";
        }
    } else if (command == "REMOVE_FRIEND") {
        int a = readInt(args, "first user id");
        int b = readInt(args, "second user id");
        expectNoMoreArgs(args);
        if (graph_.removeFriendship(a, b)) {
            out << "Removed friendship " << label(a) << " - " << label(b) << '\n';
        } else {
            out << label(a) << " and " << label(b) << " are not friends\n";
        }
    } else if (command == "ARE_FRIENDS") {
        int a = readInt(args, "first user id");
        int b = readInt(args, "second user id");
        expectNoMoreArgs(args);
        out << (graph_.areFriends(a, b) ? "Yes" : "No") << '\n';
    } else if (command == "FRIENDS") {
        int id = readInt(args, "user id");
        expectNoMoreArgs(args);
        std::vector<int> friends;
        for (const auto& entry : graph_.neighbors(id)) {
            friends.push_back(entry.first);
        }
        std::sort(friends.begin(), friends.end());
        out << "Friends of " << label(id) << " (" << friends.size() << "):";
        printUsers(friends, out);
    } else if (command == "MUTUAL") {
        int a = readInt(args, "first user id");
        int b = readInt(args, "second user id");
        expectNoMoreArgs(args);
        std::vector<int> mutual = GraphAlgorithms::mutualFriends(graph_, a, b);
        out << "Mutual friends of " << label(a) << " and " << label(b) << " (" << mutual.size()
            << "):";
        printUsers(mutual, out);
    } else if (command == "RECOMMEND") {
        int id = readInt(args, "user id");
        int k = hasMoreArgs(args) ? readInt(args, "count") : 5;
        expectNoMoreArgs(args);
        if (k < 0) {
            throw std::invalid_argument("count must not be negative");
        }
        auto recs = RecommendationEngine::recommendFriends(graph_, id, static_cast<std::size_t>(k));
        if (recs.empty()) {
            out << "No recommendations for " << label(id) << '\n';
        } else {
            out << "Recommendations for " << label(id) << ":\n";
            for (std::size_t i = 0; i < recs.size(); ++i) {
                out << "  " << i + 1 << ". " << label(recs[i].userId) << "  jaccard="
                    << formatDecimal(recs[i].jaccard()) << " (" << recs[i].mutualCount << " mutual / " << recs[i].unionSize
                    << " in union)\n";
            }
        }
    } else if (command == "JACCARD") {
        int a = readInt(args, "first user id");
        int b = readInt(args, "second user id");
        expectNoMoreArgs(args);
        out << "Jaccard(" << label(a) << ", " << label(b)
            << ") = " << formatDecimal(RecommendationEngine::jaccardSimilarity(graph_, a, b))
            << '\n';
    } else if (command == "HELP") {
        out << kHelpText;
    } else {
        throw std::invalid_argument("unknown command '" + command + "' (type HELP)");
    }
}

std::string CommandProcessor::label(int id) const {
    const User* user = graph_.findUser(id);
    if (user == nullptr) {
        return std::to_string(id);
    }
    return user->name + "(" + std::to_string(id) + ")";
}

void CommandProcessor::printUsers(const std::vector<int>& ids, std::ostream& out) const {
    for (int id : ids) {
        out << ' ' << label(id);
    }
    out << '\n';
}
