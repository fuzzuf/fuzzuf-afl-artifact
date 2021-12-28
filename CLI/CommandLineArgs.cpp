#include "CLI/CommandLineArgs.hpp"
#include "Logger/Logger.hpp"

std::vector<std::string> CommandLineArgs::Args() {
    std::vector<std::string> res;
    for (int i = 0; i < argc; i += 1) {
        if (argv[i] == nullptr) break;
        // DEBUG("[*] CommandLineArgs::Args(): argv[%d] = %s", i, argv[i]);
        res.push_back(std::string(argv[i]));
    }
    return res;
}