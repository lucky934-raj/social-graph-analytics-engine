#include "TextParsing.h"

#include <cctype>
#include <stdexcept>

namespace TextParsing {

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
        used = 0;  // not a number or out of int range
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

}  // namespace TextParsing
