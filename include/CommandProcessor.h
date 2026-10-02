#pragma once

#include "SocialGraph.h"

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

// Text command interface on top of SocialGraph. Reads from / writes to any
// stream, so the same code serves the interactive CLI, script files and tests.
class CommandProcessor {
public:
    // Executes a single command line. Returns false when the command is EXIT.
    bool execute(const std::string& line, std::ostream& out);

    // Processes commands until EXIT or end of input. With `echo` set, every
    // command is printed before its output so scripted runs are readable.
    void run(std::istream& in, std::ostream& out, bool echo, bool prompt);

    const SocialGraph& graph() const { return graph_; }

private:
    void dispatch(const std::string& command, std::istringstream& args, std::ostream& out);
    std::string label(int id) const;
    void printUsers(const std::vector<int>& ids, std::ostream& out) const;

    SocialGraph graph_;
};
