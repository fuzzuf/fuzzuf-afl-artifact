#pragma once

#include <map>
#include "CLI/FuzzerBuilder.hpp"
#include "CLI/Fuzzers/AFL/BuildAFLFuzzerFromArgs.hpp"

using BuilderMap = std::map<std::string, FuzzerBuilder>;

// Used only for CLI
class FuzzerBuilderRegister {
    public:
        FuzzerBuilderRegister(std::string name, FuzzerBuilder builder);

        static FuzzerBuilder Get(std::string name);

    private:
        static BuilderMap& GetBuilderMap();
};
