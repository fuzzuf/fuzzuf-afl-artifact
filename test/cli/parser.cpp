#define BOOST_TEST_MODULE cli.parser
#define BOOST_TEST_DYN_LINK
#include <array>
#include <iostream>
#include <boost/test/unit_test.hpp>

#include <CLI/GlobalArgs.hpp>
#include <CLI/GlobalFuzzerOptions.hpp>
#include <CLI/ParseGlobalOptionsForFuzzer.hpp>
#include <Exceptions.hpp>
#include <Logger/Logger.hpp>
#include <CLI/Stub/AFLFuzzerStub.hpp>
#include <CLI/Fuzzers/AFL/BuildAFLFuzzerFromArgs.hpp>

#define Argc(argv) (sizeof(argv) / sizeof(char *))

GlobalFuzzerOptions default_options; // Default value goes here

BOOST_AUTO_TEST_CASE(TheLeanMeanCPPOptPerser_CheckSpec) {
    GlobalFuzzerOptions options;
    options.fuzzer = "xyz";

    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--in_dir=test-in", "--out_dir=test-out", "--exec_timelimit_ms=123", "--exec_memlimit=456", "--", "--some-afl-option=true"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    FuzzerArgs fuzzer_args = ParseGlobalOptionsForFuzzer(args, options);

    DEBUG("[*] &argv[5] = %p", &argv[5]);
    DEBUG("[*] fuzzer_args.argv = %p", fuzzer_args.argv);

    BOOST_CHECK_EQUAL(fuzzer_args.argc, 1);
    BOOST_CHECK_EQUAL(fuzzer_args.argv, &argv[5]);
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_AllOptions) {
    GlobalFuzzerOptions options;
    options.fuzzer = "xyz";

    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--in_dir=test-in", "--out_dir=test-out", "--exec_timelimit_ms=123", "--exec_memlimit=456", "--", "--some-afl-option=true"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    FuzzerArgs fuzzer_args = ParseGlobalOptionsForFuzzer(args, options);

    // Check if `options` reflects `argv`
    BOOST_CHECK_EQUAL(options.in_dir, "test-in");
    BOOST_CHECK_EQUAL(options.out_dir, "test-out");
    BOOST_CHECK_EQUAL(options.exec_timelimit_ms.value(), 123);
    BOOST_CHECK_EQUAL(options.exec_memlimit.value(), 456);

    // Check if `fuzzer` is not affected
    BOOST_CHECK_EQUAL(options.fuzzer, "xyz");

    // Check if FuzzerArgs contains command line options for a fuzzer
    BOOST_CHECK_EQUAL(fuzzer_args.argc, 1);
    BOOST_CHECK_EQUAL(fuzzer_args.argv[0], "--some-afl-option=true");
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_DirsAreBlank) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--exec_timelimit_ms=123", "--exec_memlimit=456", "--"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    ParseGlobalOptionsForFuzzer(args, options);

    // `*_dir` must be default value since they are not specifed by the commad line
    BOOST_CHECK_EQUAL(options.in_dir, default_options.in_dir);
    BOOST_CHECK_EQUAL(options.out_dir, default_options.out_dir);
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_NoLogOption) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    ParseGlobalOptionsForFuzzer(args, options);

    BOOST_CHECK_EQUAL(options.logger, Logger::Stdout);
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_LogFileSpecified) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--log_file=5rC3kk6PzF5P2sPs.log", "--"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    ParseGlobalOptionsForFuzzer(args, options);

    BOOST_CHECK_EQUAL(options.logger, Logger::LogFile);

    BOOST_CHECK_EQUAL(options.log_file.value().string(), "5rC3kk6PzF5P2sPs.log");
}

inline void BaseSenario_ParseGlobalFuzzerOptions_WithWrongOption(
    const char test_case_name[], GlobalArgs &args, GlobalFuzzerOptions &options) {
    try {
        ParseGlobalOptionsForFuzzer(args, options);

        BOOST_ASSERT_MSG(false, "cli_error should be thrown in this test case");
    } catch (const exceptions::cli_error &e) {
        // Exception has been thrown. This test case passed
        const char expected_cli_error_message[] = "Unknown option or missing handler for option";
        BOOST_CHECK_EQUAL(strncmp(e.what(), expected_cli_error_message, strlen(expected_cli_error_message)), 0);
        DEBUG("[*] Caught cli_error as expected: %s: %s at %s:%d", test_case_name, e.what(), e.file, e.line);
    }
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_WithWrongOption_Case1) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--in_dir=in", "--no-such-option"};
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    BaseSenario_ParseGlobalFuzzerOptions_WithWrongOption("ParseGlobalFuzzerOptions_WithWrongOption_Case1", args, options);
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptions_WithWrongOption_Case2) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {"--in_dir=in", "--no-such-option", "--out_dir=out"}; // Sandwitch
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };
    BaseSenario_ParseGlobalFuzzerOptions_WithWrongOption("ParseGlobalFuzzerOptions_WithWrongOption_Case2", args, options);
}

BOOST_AUTO_TEST_CASE(ParseGlobalFuzzerOptionsAndPUT) {
    GlobalFuzzerOptions options;
    #pragma GCC diagnostic ignored "-Wwrite-strings"
    const char *argv[] = {
        "--in_dir=gS53LCfbAhunlziS", // Global options
        "PUT-9HmQ02GYQ09Vzwou", "Jpx1kB6oh8N9wUe0" // PUT
        };
    GlobalArgs args = {
        .argc = Argc(argv),
        .argv = argv,
    };

    // Parse global options and PUT, and build fuzzer
    auto fuzzer_args = ParseGlobalOptionsForFuzzer(args, options);
    auto fuzzer = BuildAFLFuzzerFromArgs<AFLFuzzerStub, AFLFuzzerStub>(
            fuzzer_args, options
        );

    BOOST_CHECK_EQUAL(options.in_dir, "gS53LCfbAhunlziS");

    BOOST_CHECK_EQUAL(fuzzer->argv.size(), 2);
    BOOST_CHECK_EQUAL(fuzzer->argv[0], "PUT-9HmQ02GYQ09Vzwou");
    BOOST_CHECK_EQUAL(fuzzer->argv[1], "Jpx1kB6oh8N9wUe0");
}

// もしAFL向けのオプションを追加したら、それの正常動作を確認するテストケースを追加してくださいね