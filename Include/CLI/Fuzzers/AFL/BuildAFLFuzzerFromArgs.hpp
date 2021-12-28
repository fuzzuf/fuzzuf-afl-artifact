#pragma once

#include "CLI/PutArgs.hpp"
#include "Exceptions.hpp"
#include "Utils/OptParser.hpp"
#include "Options.hpp" // includes AFLOption

// Used only for CLI
template <class TFuzzer, class TAFLFuzzer>
std::unique_ptr<TFuzzer> BuildAFLFuzzerFromArgs(FuzzerArgs &fuzzer_args, GlobalFuzzerOptions &global_options) {
    enum optionIndex {
        // Blank
    };

    const option::Descriptor usage[] = {
        // Blank
        {0, 0, 0, 0, 0, 0}
    };

    option::Stats  stats(usage, fuzzer_args.argc, fuzzer_args.argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, fuzzer_args.argc, fuzzer_args.argv, options, buffer);

    if (parse.error()) {
        throw exceptions::cli_error(Util::StrPrintf("Failed to parse AFL command line"), __FILE__, __LINE__);
    }

    // もしAFL固有のオプションに対応したら、ここにハンドラを追加してください

    PutArgs put = {
        .argc = parse.nonOptionsCount(),
        .argv = parse.nonOptions()
    };

    if (put.argc == 0) {
        throw exceptions::cli_error(Util::StrPrintf("Command line of PUT is not specified"), __FILE__, __LINE__);
    }

    // これはTraceレベルのログ
    DEBUG("[*] PUT: put = [");
    for (auto v : put.Args()) {
        DEBUG("\t\"%s\",", v.c_str());
    }
    DEBUG("    ]");

    return std::unique_ptr<TFuzzer>(
            dynamic_cast<TFuzzer *>(
                new TAFLFuzzer(
                    put.Args(),
                    global_options.in_dir,
                    global_options.out_dir,
                    global_options.exec_timelimit_ms.value_or(AFLOption::EXEC_TIMEOUT),
                    global_options.exec_memlimit.value_or(AFLOption::MEM_LIMIT),
                    /* forksrv */ true
                )
            )
        );
}