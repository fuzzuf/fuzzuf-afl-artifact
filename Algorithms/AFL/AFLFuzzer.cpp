#include "Algorithms/AFL/AFLFuzzer.hpp"

#include <cstddef>
#include <unistd.h>
#include <sys/ioctl.h>

#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Utils/Workspace.hpp"
#include "Feedback/ExitStatusFeedback.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

#include "Algorithms/AFL/CountClasses.hpp"
#include "Algorithms/AFL/AFLMacro.hpp"
#include "Algorithms/AFL/AFLSetting.hpp"
#include "Algorithms/AFL/AFLState.hpp"
#include "Algorithms/AFL/AFLMutationHierarFlowRoutines.hpp"
#include "Algorithms/AFL/AFLUpdateHierarFlowRoutines.hpp"
#include "Algorithms/AFL/AFLOtherHierarFlowRoutines.hpp"

#include "Executor/NativeLinuxExecutor.hpp"

/* Destructively classify execution counts in a trace. This is used as a
   preprocessing step for any newly acquired traces. Called on every exec,
   must be fast. */

static constexpr std::array<u8, 256> InitCountClass8() {
    std::array<u8, 256> count_class_lookup8 {};

    count_class_lookup8[0] = 0;
    count_class_lookup8[1] = 1;
    count_class_lookup8[2] = 2;
    count_class_lookup8[3] = 4;
    for (int i=4;   i<8;   i++) count_class_lookup8[i] = 8;
    for (int i=8;   i<16;  i++) count_class_lookup8[i] = 16;
    for (int i=16;  i<32;  i++) count_class_lookup8[i] = 32;
    for (int i=32;  i<128; i++) count_class_lookup8[i] = 64;
    for (int i=128; i<256; i++) count_class_lookup8[i] = 128;
    return count_class_lookup8;
}

static constexpr std::array<u16, 65536> InitCountClass16(
    const std::array<u8, 256>& count_class_lookup8
) {
    std::array<u16, 65536> count_class_lookup16 {};

    for (int b1 = 0; b1 < 256; b1++) {
        for (int b2 = 0; b2 < 256; b2++) {
            count_class_lookup16[(b1 << 8) + b2] =
                (count_class_lookup8[b1] << 8) | count_class_lookup8[b2];
        }
    }

    return count_class_lookup16;
}

static constexpr CountClasses InitCountClasses() {
    auto count_class_lookup8  = InitCountClass8();
    auto count_class_lookup16 = InitCountClass16(count_class_lookup8);
    return CountClasses{ count_class_lookup8, count_class_lookup16 };
}

const CountClasses AFLFuzzer::count_class = InitCountClasses();

// These static functions should be moved to somewhere like inside AFLState
// if AFL's forks want to use these in a different way from AFL.
static void SaveCmdline(AFLState &state, const std::vector<std::string> &argv) {
    for (u32 i=0; i < argv.size(); i++) {
        if (i > 0) state.orig_cmdline += ' ';
        state.orig_cmdline += argv[i];
    }
}

static void FixUpBanner(AFLState &state, const std::string &name) {
    if (state.use_banner.empty()) {
        if (state.sync_id.empty()) {
            fs::path put_path(name);
            state.use_banner = put_path.filename().string();
        } else {
            state.use_banner = state.sync_id;
        }
    }

    // In the original AFL, 40 is used instead of 33.
    // Because we add "fuzzuf " in the banner, we reduce this value.
    if (state.use_banner.size() > 33) {
        state.use_banner.resize(33);
        state.use_banner += "...";
    }
}

static void CheckIfTty(AFLState &state) {
    struct winsize ws;

    if (getenv("AFL_NO_UI")) {
        OKF("Disabling the UI because AFL_NO_UI is set.");
        state.not_on_tty = 1;
        return;
    }

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) {
        if (errno == ENOTTY) {
            OKF("Looks like we're not running on a tty, so I'll be a bit less verbose.");
            state.not_on_tty = 1;
        }

        return;
    }
}

static void ShufflePtrs(void** ptrs, u32 cnt, int rand_fd) {
    using afl::util::UR;
    for (u32 i=0; i < cnt-2; i++) {
        u32 j = i + UR(cnt - i, rand_fd);
        std::swap(ptrs[i], ptrs[j]);
    }
}

static void ReadTestcases(AFLState &state) {
    //FIXME: support resume
    struct dirent **nl;

    auto& in_dir = state.setting.in_dir;
    ACTF("Scanning '%s'...", in_dir.c_str());

    /* We use scandir() + alphasort() rather than readdir() because otherwise,
       the ordering  of test cases would vary somewhat randomly and would be
       difficult to control. */

    int nl_cnt = Util::ScanDirAlpha(in_dir.string(), &nl);
    if (nl_cnt < 0) {
        SAYF("\n" cLRD "[-] " cRST
            "The input directory does not seem to be valid - try again. The fuzzer needs\n"
            "    one or more test case to start with - ideally, a small file under 1 kB\n"
            "    or so. The cases must be stored as regular files directly in the input\n"
            "    directory.\n");

        PFATAL("Unable to open '%s'", in_dir.c_str());
    }

    if (state.shuffle_queue && nl_cnt > 1) {
        ACTF("Shuffling queue...");
        ShufflePtrs((void**)nl, nl_cnt, state.rand_fd);
    }

    for (int i=0; i < nl_cnt; i++) {
        struct stat st;

        std::string fn = Util::StrPrintf("%s/%s", in_dir.c_str(), nl[i]->d_name);
        std::string dfn = Util::StrPrintf("%s/.state/deterministic_done/%s", in_dir.c_str(), nl[i]->d_name);

        bool passed_det = false;
        free(nl[i]); /* not tracked */

        if (lstat(fn.c_str(), &st) != 0 || access(fn.c_str(), R_OK) != 0) {
            PFATAL("Unable to access '%s'", fn.c_str());
        }

        /* This also takes care of . and .. */

        if (!S_ISREG(st.st_mode) || !st.st_size || fn.find("/README.txt") != std::string::npos) {
            continue;
        }

        if (st.st_size > AFLOption::MAX_FILE) {
            FATAL("Test case '%s' is too big (%s, limit is %s)",
                fn.c_str(),
                afl::util::DescribeMemorySize(st.st_size).c_str(),
                afl::util::DescribeMemorySize(AFLOption::MAX_FILE).c_str()
            );
        }

        /* Check for metadata that indicates that deterministic fuzzing
           is complete for this entry. We don't want to repeat deterministic
           fuzzing when resuming aborted scans, because it would be pointless
           and probably very time-consuming. */

        if (access(dfn.c_str(), F_OK) == 0) passed_det = true;

        afl::util::AddToQueue(state, fn, nullptr, (u32)st.st_size, passed_det);
    }

    free(nl); /* not tracked */

    if (!state.queued_paths) {
        SAYF("\n" cLRD "[-] " cRST
             "Looks like there are no valid test cases in the input directory! The fuzzer\n"
             "    needs one or more test case to start with - ideally, a small file under\n"
             "    1 kB or so. The cases must be stored as regular files directly in the\n"
             "    input directory.\n");

        FATAL("No usable test cases in '%s'", in_dir.c_str());
    }

    state.last_path_time = 0;
    state.queued_at_start = state.queued_paths;

}

static void PivotInputs(AFLState &state) {
    ACTF("Creating hard links for all input files...");

    u32 id = 0;
    for (const auto& testcase : state.case_queue) {
        auto& input = *testcase->input;

        std::string rsl = input.GetPath().filename().string();

        const std::string case_prefix = state.setting.simple_files ? "id_" : "id:";

        u32 orig_id;
        std::string nfn;
        if ( rsl.substr(0, 3) == case_prefix
          && sscanf(rsl.c_str()+3, "%06u", &orig_id) == 1
          && orig_id == id) {
            state.resuming_fuzz = true;
            nfn = Util::StrPrintf("%s/queue/%s", state.setting.out_dir.c_str(), rsl.c_str());

            /* Since we're at it, let's also try to find parent and figure out the
               appropriate depth for this entry. */

            u32 src_id;
            auto pos = rsl.find(':');
            if ( pos != std::string::npos
              && sscanf(rsl.c_str()+pos+1, "%06u", &src_id) == 1) {

                if (src_id < state.case_queue.size()) {
                    testcase->depth = state.case_queue[src_id]->depth + 1;
                }

                if (state.max_depth < testcase->depth) {
                    state.max_depth = testcase->depth;
                }
            }
        } else {
            /* No dice - invent a new name, capturing the original one as a
               substring. */

            if (!state.setting.simple_files) {
                auto pos = rsl.find(",orig:");

                if (pos != std::string::npos) pos += 6;
                else pos = 0;

                nfn = Util::StrPrintf("%s/queue/id:%06u,orig:%s",
                                        state.setting.out_dir.c_str(),
                                        id,
                                        rsl.c_str() + pos);
            } else {
                nfn = Util::StrPrintf("%s/queue/id_%06u",
                                        state.setting.out_dir.c_str(), id);
            }
        }

        /* Pivot to the new queue entry. */

        if (!input.LinkAndRefer(nfn)) {
            input.CopyAndRefer(nfn);
        }

        /* Make sure that the passed_det value carries over, too. */

        if (testcase->passed_det) afl::util::MarkAsDetDone(state, *testcase);

        id++;
    }

#if 0
    if (state.in_place_resume) NukeResumeDir();
#endif
}

static void CheckMapCoverage(const InplaceMemoryFeedback &inp_feed) {
    if (inp_feed.CountNonZeroBytes() < 100) return ;

    inp_feed.ShowMemoryToFunc(
        [](const u8 *trace_bits, u32 map_size) {
            u32 start = 1 << (AFLOption::MAP_SIZE_POW2 - 1);
            for (u32 i = start; i < map_size; i++) {
                if (trace_bits[i]) return;
            }

            WARNF("Recompile binary with newer version of afl to improve coverage!");
        }
    );
}

static void PerformDryRun(AFLState &state) {
    u32 cal_failures = 0;
    char *skip_crashes = getenv("AFL_SKIP_CRASHES");

    for (const auto& testcase : state.case_queue) {
        auto& input = *testcase->input;

        std::string fn = input.GetPath().filename().string();

        ACTF("Attempting dry run with '%s'...", fn.c_str());

        input.Load();

        // There should be no active instance of InplaceMemoryFeedback at this point.
        // So we can just create a temporary instance to get a result.
        InplaceMemoryFeedback inp_feed;
        ExitStatusFeedback exit_status;
        PUTExitReasonType res = afl::util::CalibrateCaseWithFeedDestroyed(
                                    *testcase,
                                    input.GetBuf(),
                                    input.GetLen(),
                                    state,
                                    inp_feed,
                                    exit_status,
                                    0,
                                    true);

        input.Unload();

        if (state.stop_soon) return;

        if (res == state.crash_mode || res == PUTExitReasonType::FAULT_NOBITS) {
            SAYF(cGRA "    len = %u, map size = %u, exec speed = %llu us\n" cRST,
                 input.GetLen(), testcase->bitmap_size, testcase->exec_us);
        }

        switch (res) {
        case PUTExitReasonType::FAULT_NONE:
            if (testcase == state.case_queue.front()) {
                CheckMapCoverage(inp_feed);
            }

            if (state.crash_mode != PUTExitReasonType::FAULT_NONE) {
                FATAL("Test case '%s' does *NOT* crash", fn.c_str());
            }

            break;

        case PUTExitReasonType::FAULT_TMOUT:

            if (state.timeout_given) {
                /* The -t nn+ syntax in the command line sets timeout_given to '2' and
                   instructs afl-fuzz to tolerate but skip queue entries that time
                   out. */

                if (state.timeout_given > 1) {
                    WARNF("Test case results in a timeout (skipping)");
                    testcase->cal_failed = AFLOption::CAL_CHANCES;
                    cal_failures++;
                    break;
                }

                SAYF("\n" cLRD "[-] " cRST
                     "The program took more than %u ms to process one of the initial test cases.\n"
                     "    Usually, the right thing to do is to relax the -t option - or to delete it\n"
                     "    altogether and allow the fuzzer to auto-calibrate. That said, if you know\n"
                     "    what you are doing and want to simply skip the unruly test cases, append\n"
                     "    '+' at the end of the value passed to -t ('-t %u+').\n",
                     state.setting.exec_timelimit_ms,
                     state.setting.exec_timelimit_ms);

                FATAL("Test case '%s' results in a timeout", fn.c_str());
            } else {
                SAYF("\n" cLRD "[-] " cRST
                     "The program took more than %u ms to process one of the initial test cases.\n"
                     "    This is bad news; raising the limit with the -t option is possible, but\n"
                     "    will probably make the fuzzing process extremely slow.\n\n"

                     "    If this test case is just a fluke, the other option is to just avoid it\n"
                     "    altogether, and find one that is less of a CPU hog.\n",
                     state.setting.exec_timelimit_ms);

                FATAL("Test case '%s' results in a timeout", fn.c_str());
            }

        case PUTExitReasonType::FAULT_CRASH:

            if (state.crash_mode == PUTExitReasonType::FAULT_CRASH) break;

            if (skip_crashes) {
                WARNF("Test case results in a crash (skipping)");
                testcase->cal_failed = AFLOption::CAL_CHANCES;
                cal_failures++;
                break;
            }

            if (state.setting.exec_memlimit > 0) {
                SAYF("\n" cLRD "[-] " cRST
                    "Oops, the program crashed with one of the test cases provided. There are\n"
                    "    several possible explanations:\n\n"

                    "    - The test case causes known crashes under normal working conditions. If\n"
                    "      so, please remove it. The fuzzer should be seeded with interesting\n"
                    "      inputs - but not ones that cause an outright crash.\n\n"

                    "    - The current memory limit (%s) is too low for this program, causing\n"
                    "      it to die due to OOM when parsing valid files. To fix this, try\n"
                    "      bumping it up with the -m setting in the command line. If in doubt,\n"
                    "      try something along the lines of:\n\n"

#ifdef RLIMIT_AS
                    "      ( ulimit -Sv $[%llu << 10]; /path/to/binary [...] <testcase )\n\n"
#else
                    "      ( ulimit -Sd $[%llu << 10]; /path/to/binary [...] <testcase )\n\n"
#endif /* ^RLIMIT_AS */

                    "      Tip: you can use http://jwilk.net/software/recidivm to quickly\n"
                    "      estimate the required amount of virtual memory for the binary. Also,\n"
                    "      if you are using ASAN, see %s/notes_for_asan.txt.\n\n"

#ifdef __APPLE__

                    "    - On MacOS X, the semantics of fork() syscalls are non-standard and may\n"
                    "      break afl-fuzz performance optimizations when running platform-specific\n"
                    "      binaries. To fix this, set AFL_NO_FORKSRV=1 in the environment.\n\n"

#endif /* __APPLE__ */

                    "",
                    afl::util::DescribeMemorySize(state.setting.exec_memlimit << 20).c_str(),
                    state.setting.exec_memlimit - 1,
                    "docs"
                );
            } else {
                SAYF("\n" cLRD "[-] " cRST
                    "Oops, the program crashed with one of the test cases provided. There are\n"
                    "    several possible explanations:\n\n"

                    "    - The test case causes known crashes under normal working conditions. If\n"
                    "      so, please remove it. The fuzzer should be seeded with interesting\n"
                    "      inputs - but not ones that cause an outright crash.\n\n"

#ifdef __APPLE__

                    "    - On MacOS X, the semantics of fork() syscalls are non-standard and may\n"
                    "      break afl-fuzz performance optimizations when running platform-specific\n"
                    "      binaries. To fix this, set AFL_NO_FORKSRV=1 in the environment.\n\n"

#endif /* __APPLE__ */

                );
            }

            FATAL("Test case '%s' results in a crash", fn.c_str());

        case PUTExitReasonType::FAULT_ERROR:

            FATAL("Unable to execute target application ('%s')",
                state.setting.argv[0].c_str());

        case PUTExitReasonType::FAULT_NOINST:

            FATAL("No instrumentation detected");

        case PUTExitReasonType::FAULT_NOBITS:

            state.useless_at_start++;

            if (state.in_bitmap.empty() && !state.shuffle_queue)
                WARNF("No new instrumentation output, test case may be useless.");

            break;
        }

        if (testcase->var_behavior)
            WARNF("Instrumentation output varies across runs.");
    }

    if (cal_failures) {
        if (cal_failures == state.queued_paths)
            FATAL("All test cases time out%s, giving up!",
                skip_crashes ? " or crash" : "");

        WARNF("Skipped %u test cases (%0.02f%%) due to timeouts%s.", cal_failures,
            ((double)cal_failures) * 100 / state.queued_paths,
            skip_crashes ? " or crashes" : "");

        if (cal_failures * 5 > state.queued_paths)
            WARNF(cLRD "High percentage of rejected test cases, check settings!");
    }

    OKF("All test cases processed.");
}

AFLFuzzer::AFLFuzzer(
    const std::vector<std::string> &argv,
    const std::string &in_dir,
    const std::string &out_dir,
    u32 exec_timelimit_ms,
    u32 exec_memlimit,
    bool forksrv
) :
    setting( argv, 
             in_dir, 
             out_dir, 
             exec_timelimit_ms, 
             exec_memlimit,
             forksrv,
             false,
             NativeLinuxExecutor::CPUID_BIND_WHICHEVER)
    // executor, afl_*_strat, and fuzz_prim will be initialized inside the function
{
    // Executor needs the directory specified by "out_dir" to be already set up
    // so we need to create the directory first, and then initialize Executor
    SetupDirs(setting.out_dir.string());

    // TODO: support more types of executors
    executor.reset(
        new NativeLinuxExecutor(
            setting.argv, 
            setting.exec_timelimit_ms,
            setting.exec_memlimit,
            setting.forksrv,
            setting.out_dir / AFLOption::DEFAULT_OUTFILE,
            true,                 // need_afl_cov
            false,                // need_bb_cov
            setting.cpuid_to_bind
        )
    );

    state.reset(new AFLState( setting, *executor ));

    state->start_time = Util::GetCurTimeMs();

    BuildFuzzFlow();

    SaveCmdline(*state, setting.argv);
    FixUpBanner(*state, setting.argv[0]);
    CheckIfTty(*state);

    ReadTestcases(*state);
    PivotInputs(*state);
    PerformDryRun(*state);
}

AFLFuzzer::~AFLFuzzer() {}

void AFLFuzzer::BuildFuzzFlow() {
    using namespace afl::pipeline::other;
    using namespace afl::pipeline::mutation;
    using namespace afl::pipeline::update;

    using pipeline::CreateNode;

    // the head node(the fuzzing loop starts from seed selection in AFL)
    fuzz_loop = CreateNode<SelectSeed>(*state);

    // middle nodes(steps done before and after actual mutations)
    auto abandon_node = CreateNode<AbandonEntry>(*state);

    auto consider_skip_mut = CreateNode<ConsiderSkipMut>(*state);
    auto retry_calibrate = CreateNode<RetryCalibrate>(*state, *abandon_node);
    auto trim_case = CreateNode<TrimCase>(*state, *abandon_node);
    auto calc_score = CreateNode<CalcScore>(*state);
    auto apply_det_muts  = CreateNode<ApplyDetMuts>(*state, *abandon_node);
    auto apply_rand_muts = CreateNode<ApplyRandMuts>(*state, *abandon_node);

    // actual mutations
    auto bit_flip1 = CreateNode<BitFlip1WithAutoDictBuild>(*state);
    auto bit_flip_other = CreateNode<BitFlipOther>(*state);
    auto byte_flip1 = CreateNode<ByteFlip1WithEffMapBuild>(*state);
    auto byte_flip_other = CreateNode<ByteFlipOther>(*state);
    auto arith = CreateNode<Arith>(*state);
    auto interest = CreateNode<Interest>(*state);
    auto user_dict_overwrite = CreateNode<UserDictOverwrite>(*state);
    auto user_dict_insert = CreateNode<UserDictInsert>(*state);
    auto auto_dict_overwrite = CreateNode<AutoDictOverwrite>(*state);
    auto havoc = CreateNode<Havoc>(*state);
    auto splicing = CreateNode<Splicing>(*state);

    // execution
    auto execute = CreateNode<ExecutePUT>(*state);

    // updates corresponding to mutations
    auto normal_update = CreateNode<NormalUpdate>(*state);
    auto construct_auto_dict = CreateNode<ConstructAutoDict>(*state);
    auto construct_eff_map = CreateNode<ConstructEffMap>(*state);

    fuzz_loop << ( 
         consider_skip_mut
      || retry_calibrate
      || trim_case
      || calc_score
      || apply_det_muts << (
             bit_flip1 << execute << (normal_update || construct_auto_dict)
          || bit_flip_other << execute.HardLink() << normal_update.HardLink()
          || byte_flip1 << execute.HardLink() << (normal_update.HardLink() 
                                               || construct_eff_map)
          || byte_flip_other << execute.HardLink() << normal_update.HardLink()
          || arith << execute.HardLink() << normal_update.HardLink()
          || interest << execute.HardLink() << normal_update.HardLink()
          || user_dict_overwrite << execute.HardLink() << normal_update.HardLink()
          || auto_dict_overwrite << execute.HardLink() << normal_update.HardLink()
         )
       || apply_rand_muts << (
               havoc << execute.HardLink() << normal_update.HardLink()
            || splicing << execute.HardLink() << normal_update.HardLink()
          )
       || abandon_node
    );

    /*[
        consider_skip_mut,
        retry_calibrate,
        trim_case,
        calc_score,
        apply_det_muts [
            bit_flip1 << execute [ normal_update, construct_auto_dict ],
            bit_flip_other << execute << normal_update,
            byte_flip1 << execute [ normal_update, construct_eff_map ],
            byte_flip_other << execute << normal_update,
            arith << execute << normal_update,
            interest << execute << normal_update,
            user_dict_overwrite << execute << normal_update,
            auto_dict_overwrite << execute << normal_update
        ],
        apply_rand_muts [
            havoc << execute << normal_update,
            splicing << execute << normal_update
        ],
        abandon_node
    ];*/

}

static void CullQueue(AFLState &state) {
    if (state.setting.dumb_mode || !state.score_changed) return;

    state.score_changed = false;

    std::bitset<AFLOption::MAP_SIZE> has_top_rated;

    state.queued_favored = 0;
    state.pending_favored = 0;

    for (const auto& testcase : state.case_queue) {
        testcase->favored = false;
    }

    for (u32 i=0; i < AFLOption::MAP_SIZE; i++) {
        if (state.top_rated[i] && !has_top_rated[i]) {
            auto &top_testcase = state.top_rated[i].value().get();

            has_top_rated |= *(top_testcase.trace_mini);

            top_testcase.favored = true;
            state.queued_favored++;

            if (!top_testcase.was_fuzzed) state.pending_favored++;
        }
    }

    for (const auto& testcase : state.case_queue) {
        afl::util::MarkAsRedundant(state, *testcase, !testcase->favored);
    }
}

// FIXME: CullQueue can be a node
void AFLFuzzer::OneLoop(void) {
    CullQueue(*state);
    fuzz_loop();
}

// Do not call non aync-signal-safe functions inside
// because this function can be called during signal handling
void AFLFuzzer::ReceiveStopSignal(void) {
    state->ReceiveStopSignal(); 
    executor->ReceiveStopSignal();
}
