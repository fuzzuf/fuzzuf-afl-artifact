#include "CLI/ParseGlobalOptionsForFuzzer.hpp"
#include "Utils/CheckIfStringIsDecimal.hpp"
#include "Utils/Common.hpp"
#include "Exceptions.hpp"
#include "Utils/OptParser.hpp"
#include "Logger/Logger.hpp"
#include "Utils/Common.hpp"
#include <string>

inline void CheckIfOptionHasValue(option::Option &opt) {
    if (!opt.arg) {
        throw exceptions::cli_error(Util::StrPrintf("Option %s does not have value", opt.name), __FILE__, __LINE__);
    }
}

FuzzerArgs ParseGlobalOptionsForFuzzer(GlobalArgs &global_args, GlobalFuzzerOptions &global_options) {
    enum optionIndex { 
        Unknown,
        Help,
        InDir,
        OutDir,
        ExecTimelimitMs, 
        ExecMemLimit,
        Logger,
        LogFile,
    };

    const option::Descriptor usage[] = {
        {optionIndex::Unknown, 0, "" , "" ,option::Arg::None, "Usage: fuzzuf [fuzzer] [options]\n\n"
            "Options:" },
        {optionIndex::Help, 0, "", "help", option::Arg::None, "    --help \tPrint this help." },
        {optionIndex::InDir, 0, "", "in_dir", option::Arg::Optional, "    --in_dir IN_DIR \tSet seed dir. Default is `./seeds`." },
        {optionIndex::OutDir, 0, "", "out_dir", option::Arg::Optional, "    --out_dir OUT_DIR \tSet output dir. Default is `/tmp/fuzzuf-out_dir`." },
        {optionIndex::ExecTimelimitMs, 0, "", "exec_timelimit_ms",option::Arg::Optional, "    --exec_timelimit_ms EXEC_TIME_LIMIT \tLimit execution time of PUT. Unit is milli-seconds." },
        {optionIndex::ExecMemLimit, 0, "", "exec_memlimit",option::Arg::Optional, "    --exec_memlimit EXEC_MEMORY_LIMIT \tLimit memory usage for PUT execution. " },
        {optionIndex::LogFile, 0, "", "log_file",option::Arg::Optional, "    --log_file LOG_FILE \tEnable LogFile logger and set the log file path for LogFile logger" },
        {0, 0, 0, 0, 0, 0}
    };

    option::Stats  stats(usage, global_args.argc, global_args.argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, global_args.argc, global_args.argv, options, buffer);

    if (parse.error()) {
        throw exceptions::cli_error(Util::StrPrintf("Failed to parse command line"), __FILE__, __LINE__);
    }

    // Trace レベルのログ
    // DEBUG("[*] ParseGlobalOptionsForFuzzer: argc = %d", global_args.argc);
    // DEBUG("[*] ParseGlobalOptionsForFuzzer: parse.optionsCount() = %d", parse.optionsCount());
    // DEBUG("[*] ParseGlobalOptionsForFuzzer: parse.nonOptionsCount() = %d", parse.nonOptionsCount());

    for (int i = 0; i < parse.optionsCount(); ++i) {
        option::Option& opt = buffer[i];

        switch(opt.index()) {
            case optionIndex::InDir: {
                CheckIfOptionHasValue(opt);
                global_options.in_dir = opt.arg;
                break;
            }
            case optionIndex::OutDir: {
                CheckIfOptionHasValue(opt);
                global_options.out_dir = opt.arg;
                break;
            }
            case optionIndex::ExecTimelimitMs: {
                CheckIfOptionHasValue(opt);
                if (!CheckIfStringIsDecimal(opt.arg)) {
                    throw exceptions::cli_error(
                        Util::StrPrintf("Option \"--exec_timelimit_ms\" has non decimal caractors: %s", opt.arg),
                        __FILE__, __LINE__);
                }
                global_options.exec_timelimit_ms = atoi(opt.arg);
                break;
            }
            case optionIndex::ExecMemLimit: {
                CheckIfOptionHasValue(opt);
                if (!CheckIfStringIsDecimal(opt.arg)) {
                    throw exceptions::cli_error(
                        Util::StrPrintf("Option \"--exec_memlimit\" has non decimal caractors: %s", opt.arg),
                        __FILE__, __LINE__);
                }
                global_options.exec_memlimit = atoi(opt.arg);
                break;
            }
            case optionIndex::LogFile: {
                 CheckIfOptionHasValue(opt);
                 global_options.logger = Logger::LogFile;
                 global_options.log_file = fs::path(opt.arg);
                 break;
            }
            default: {
                throw exceptions::cli_error(
                    Util::StrPrintf("Unknown option or missing handler for option \"%s\"", opt.name),
                    __FILE__, __LINE__);
            }
        }
    }

    if (options[optionIndex::Help]) {
        // help メッセージを表示
        option::printUsage(std::cout, usage);
        global_options.help = true;
    }

    return FuzzerArgs {
        .argc = parse.nonOptionsCount(),
        .argv = parse.nonOptions()
    };
}