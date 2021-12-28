#pragma once

#include <string>
#include <optional>
#include "Utils/Common.hpp"
#include "Logger/Logger.hpp"
#include "Utils/Filesystem.hpp"

struct GlobalFuzzerOptions {
    bool help;
    std::string fuzzer;                     // Required
    std::string in_dir;                     // Required; TODO: fs::path の方がいいかも
    std::string out_dir;                    // Required
    std::optional<u32> exec_timelimit_ms;   // Optional
    std::optional<u32> exec_memlimit;       // Optional
    Logger logger;                          // Required
    std::optional<fs::path> log_file;       // Optional

    // Default values
    GlobalFuzzerOptions() : 
        help(false),
        fuzzer("afl"), 
        in_dir("./seeds"), // FIXME: Linux前提
        out_dir("/tmp/fuzzuf-out_dir"), // FIXME: Linux前提
        exec_timelimit_ms(std::nullopt), // Specify no limits
        exec_memlimit(std::nullopt),
        logger(Logger::Stdout),
        log_file(std::nullopt)
        {};
};