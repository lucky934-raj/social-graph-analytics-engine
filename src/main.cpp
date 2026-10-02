#include "CommandProcessor.h"

#include <fstream>
#include <iostream>

// Usage:
//   social_graph               interactive mode (reads commands from stdin)
//   social_graph <script>      runs the commands in <script>, echoing each one
int main(int argc, char* argv[]) {
    CommandProcessor cli;

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [command-script]\n";
        return 1;
    }

    if (argc == 2) {
        std::ifstream script(argv[1]);
        if (!script) {
            std::cerr << "Could not open " << argv[1] << '\n';
            return 1;
        }
        cli.run(script, std::cout, /*echo=*/true, /*prompt=*/false);
        return 0;
    }

    std::cout << "Social Graph Analytics Engine. Type HELP for commands.\n";
    cli.run(std::cin, std::cout, /*echo=*/false, /*prompt=*/true);
    return 0;
}
