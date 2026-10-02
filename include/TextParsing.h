#pragma once

#include <sstream>
#include <string>

// Helpers for reading whitespace-separated arguments from a line of text.
// Shared by the CLI and the graph file loader. All of them throw
// std::invalid_argument with a readable message on bad input.
namespace TextParsing {

// Reads the next token and parses it as a whole integer ("12abc" is rejected).
int readInt(std::istringstream& args, const std::string& what);

bool hasMoreArgs(std::istringstream& args);
void expectNoMoreArgs(std::istringstream& args);

// Everything left on the line, without leading or trailing whitespace.
std::string readRestOfLine(std::istringstream& args);

}  // namespace TextParsing
