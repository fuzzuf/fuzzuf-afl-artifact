#include "Algorithms/AFL/AFLUtil.hpp"

#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"
#include "Algorithms/AFL/AFLMacro.hpp"

// ここに入れるべきか、クラスのメンバ関数にすべきかの判断基準（暫定）:
//     - AFL派生のアルゴリズムなどを実装するにあたって有用そうな、AFL特有のutility関数がここに追加されるべき
//     - 基本的に、stateを改変するなどの、大きい副作用を持つものはここにあるべきではない
//        * ただし、複数のクラス（e.g. RetryCalibrateとSaveIfInteresting）で呼ばざるを得ないような関数についてはここに実装する
//        * これの利点としては、メンバ変数を経由せず必ず引数経由で変数に参照するので、副作用がよく見えること

namespace afl {
namespace util {

u32 UR(u32 limit, int rand_fd) {
    static u32 rand_cnt;

#ifdef BEHAVE_DETERMINISTIC
    rand_fd++; // Just to muzzle Wunused-variable
    static u32 cnt;
    if (unlikely(!rand_cnt--)) {
        srandom(cnt++);
    }
#else
    if (rand_fd != -1 && unlikely(!rand_cnt--)) {
        u32 seed[2];
        Util::ReadFile(rand_fd, &seed, sizeof(seed));
        srandom(seed[0]);
        rand_cnt = (AFLOption::RESEED_RNG / 2) + (seed[1] % AFLOption::RESEED_RNG);
    }
#endif

    return random() % limit;
}

/* Describe all the integers with five characters or less */

std::string DescribeInteger(u64 val) {
#define CHK_FORMAT(_divisor, _limit_mult, _fmt, _cast) do { \
        if (val < (_divisor) * (_limit_mult)) { \
            return Util::StrPrintf(_fmt, ((_cast)val) / (_divisor)); \
        } \
    } while (0)

    /* 0-9999 */
    CHK_FORMAT(1, 10000, "%llu", u64);

    /* 10.0k - 99.9k */
    CHK_FORMAT(1000, 99.95, "%0.01fk", double);

    /* 100k - 999k */
    CHK_FORMAT(1000, 1000, "%lluk", u64);

    /* 1.00M - 9.99M */
    CHK_FORMAT(1000 * 1000, 9.995, "%0.02fM", double);

    /* 10.0M - 99.9M */
    CHK_FORMAT(1000 * 1000, 99.95, "%0.01fM", double);

    /* 100M - 999M */
    CHK_FORMAT(1000 * 1000, 1000, "%lluM", u64);

    /* 1.00G - 9.99G */
    CHK_FORMAT(1000LL * 1000 * 1000, 9.995, "%0.02fG", double);

    /* 10.0G - 99.9G */
    CHK_FORMAT(1000LL * 1000 * 1000, 99.95, "%0.01fG", double);

    /* 100G - 999G */
    CHK_FORMAT(1000LL * 1000 * 1000, 1000, "%lluG", u64);

    /* 1.00T - 9.99G */
    CHK_FORMAT(1000LL * 1000 * 1000 * 1000, 9.995, "%0.02fT", double);

    /* 10.0T - 99.9T */
    CHK_FORMAT(1000LL * 1000 * 1000 * 1000, 99.95, "%0.01fT", double);

    /* 100T+ */
    return "infty";
}

/* Describe float. Similar to the above, except with a single 
   static buffer. */

std::string DescribeFloat(double val) {
    if (val < 99.995) {
        return Util::StrPrintf("%0.02f", val);
    }

    if (val < 999.95) {
        return Util::StrPrintf("%0.01f", val);
    }

    return DescribeInteger((u64)val);
}

std::string DescribeMemorySize(u64 val) {
    /* 0-9999 */
    CHK_FORMAT(1, 10000, "%llu B", u64);

    /* 10.0k - 99.9k */
    CHK_FORMAT(1024, 99.95, "%0.01f kB", double);

    /* 100k - 999k */
    CHK_FORMAT(1024, 1000, "%llu kB", u64);

    /* 1.00M - 9.99M */
    CHK_FORMAT(1024 * 1024, 9.995, "%0.02f MB", double);

    /* 10.0M - 99.9M */
    CHK_FORMAT(1024 * 1024, 99.95, "%0.01f MB", double);

    /* 100M - 999M */
    CHK_FORMAT(1024 * 1024, 1000, "%llu MB", u64);

    /* 1.00G - 9.99G */
    CHK_FORMAT(1024LL * 1024 * 1024, 9.995, "%0.02f GB", double);

    /* 10.0G - 99.9G */
    CHK_FORMAT(1024LL * 1024 * 1024, 99.95, "%0.01f GB", double);

    /* 100G - 999G */
    CHK_FORMAT(1024LL * 1024 * 1024, 1000, "%llu GB", u64);

    /* 1.00T - 9.99G */
    CHK_FORMAT(1024LL * 1024 * 1024 * 1024, 9.995, "%0.02f TB", double);

    /* 10.0T - 99.9T */
    CHK_FORMAT(1024LL * 1024 * 1024 * 1024, 99.95, "%0.01f TB", double);

#undef CHK_FORMAT

    /* 100T+ */
    return "infty";
}

/* Describe time delta. Returns one static buffer, 34 chars of less. */

std::string DescribeTimeDelta(u64 cur_ms, u64 event_ms) {

  u64 delta;
  s32 t_d, t_h, t_m, t_s;

  if (!event_ms) return "none seen yet";

  delta = cur_ms - event_ms;

  t_d = delta / 1000 / 60 / 60 / 24;
  t_h = (delta / 1000 / 60 / 60) % 24;
  t_m = (delta / 1000 / 60) % 60;
  t_s = (delta / 1000) % 60;

  return Util::StrPrintf("%s days, %u hrs, %u min, %u sec", 
                            DescribeInteger(t_d).c_str(), t_h, t_m, t_s);
}

void MarkAsDetDone(const AFLState& state, AFLTestcase &testcase) {
    const auto& input = *testcase.input;

    std::string fn = input.GetPath().filename().string();
    fn = Util::StrPrintf("%s/queue/.state/deterministic_done/%s", 
        state.setting.out_dir.c_str(), fn.c_str());

    int fd = Util::OpenFile(fn, O_WRONLY | O_CREAT | O_EXCL, 0600);
    Util::CloseFile(fd);

    testcase.passed_det = true;
}

void MarkAsVariable(const AFLState& state, AFLTestcase &testcase) {
    const auto& input = *testcase.input;

    std::string fn = input.GetPath().filename().string();
    std::string ldest = Util::StrPrintf("../../%s", fn.c_str());
    fn = Util::StrPrintf("%s/queue/.state/variable_behavior/%s", 
        state.setting.out_dir.c_str(), fn.c_str());

    if (symlink(ldest.c_str(), fn.c_str()) == -1) {
        int fd = Util::OpenFile(fn, O_WRONLY | O_CREAT | O_EXCL, 0600);
        Util::CloseFile(fd);
    }

    testcase.var_behavior = true;
}

void MarkAsRedundant(const AFLState& state, AFLTestcase &testcase, bool val) {
    const auto& input = *testcase.input;

    if (val == testcase.fs_redundant) return;

    testcase.fs_redundant = val;

    std::string fn = input.GetPath().filename().string();
    fn = Util::StrPrintf("%s/queue/.state/redundant_edges/%s", 
        state.setting.out_dir.c_str(), fn.c_str());
    
    if (val) {
        int fd = Util::OpenFile(fn, O_WRONLY | O_CREAT | O_EXCL, 0600);
        Util::CloseFile(fd);
    } else {
        if (unlink(fn.c_str())) PFATAL("Unable to remove '%s'", fn.c_str());
    }
}

u8 HasNewBits(const u8 *trace_bits, u8 *virgin_map, u32 map_size, AFLState &state) {
    if constexpr (sizeof(size_t) == 8) {
        return DoHasNewBits<u64>(trace_bits, virgin_map, map_size, state);
    } else {
        return DoHasNewBits<u32>(trace_bits, virgin_map, map_size, state);
    }
}

static void MinimizeBits(
    std::bitset<AFLOption::MAP_SIZE> &trace_mini, const u8 *trace_bits
) {
    for (u32 i=0; i < AFLOption::MAP_SIZE; i++) {
        trace_mini[i] = trace_bits[i] != 0;
    }
}

void UpdateBitmapScoreWithRawTrace(
    AFLTestcase &testcase,
    AFLState &state,
    const u8 *trace_bits,
    u32 map_size
) {
#ifdef BEHAVE_DETERMINISTIC // Just for ease of grep
#else
    u64 fav_factor = testcase.exec_us * testcase.input->GetLen();
#endif

    for (u32 i=0; i<map_size; i++) {
        if (trace_bits[i]) {
            if (state.top_rated[i]) {
                auto &top_testcase = state.top_rated[i].value().get();
#ifdef BEHAVE_DETERMINISTIC
                // FIXME: this probability may be too much. Need for opinions.
                if (UR(2, -1)) continue; 
#else
                u64 factor = top_testcase.exec_us * top_testcase.input->GetLen();
                if (fav_factor > factor) continue;
#endif
             
                /* Looks like we're going to win. Decrease ref count for the
                   previous winner, discard its trace_bits[] if necessary. */        
                --top_testcase.tc_ref;
                if (top_testcase.tc_ref == 0) {
                    top_testcase.trace_mini.reset();
                }
            }

            /* Insert ourselves as the new winner. */

            state.top_rated[i] = std::ref(testcase);
            testcase.tc_ref++;

            if (!testcase.trace_mini) {
                testcase.trace_mini.reset(
                    new std::bitset<AFLOption::MAP_SIZE>()
                );
                MinimizeBits(*testcase.trace_mini, trace_bits);
            }

            state.score_changed = true;
        }
    }
}

void UpdateBitmapScore(
    AFLTestcase &testcase,
    AFLState &state,
    const InplaceMemoryFeedback &inp_feed   
) {
    inp_feed.ShowMemoryToFunc(
        [&testcase, &state](const u8* trace_bits, u32 map_size) {
            UpdateBitmapScoreWithRawTrace(testcase, state, trace_bits, map_size);
        }
    );
}

PUTExitReasonType CalibrateCaseWithFeedDestroyed(
    AFLTestcase &testcase,
    const u8 *buf,
    u32 len,
    AFLState &state,
    InplaceMemoryFeedback &inp_feed,
    ExitStatusFeedback &exit_status,
    u32 handicap,
    bool from_queue
) {
    std::array<u8, AFLOption::MAP_SIZE> first_trace;

    bool first_run = testcase.exec_cksum == 0;

    s32 old_sc = state.stage_cur;
    s32 old_sm = state.stage_max;
    std::string old_sn = std::move(state.stage_name);

    u32 use_tmout;
    if (!from_queue || state.resuming_fuzz) {
        use_tmout = std::max(state.setting.exec_timelimit_ms + AFLOption::CAL_TMOUT_ADD,
                             state.setting.exec_timelimit_ms * AFLOption::CAL_TMOUT_PERC / 100);
    } else {
        use_tmout = state.setting.exec_timelimit_ms;
    }

    testcase.cal_failed++;

    state.stage_name = "calibration";
    state.stage_max = state.fast_cal ? 3 : AFLOption::CAL_CYCLES;

    u8 hnb = 0;
    u8 new_bits = 0;
    if (testcase.exec_cksum) {
        inp_feed.ShowMemoryToFunc(
            [&state, &first_trace, &hnb](const u8* trace_bits, u32 map_size) {
                std::memcpy(first_trace.data(), trace_bits, map_size);
                hnb = HasNewBits(trace_bits, &state.virgin_bits[0], map_size, state);
            }
        );

        if (hnb > new_bits) new_bits = hnb;
    }

    bool var_detected = false;
    u64 start_us = Util::GetCurTimeUs();
    u64 stop_us;
    for (state.stage_cur=0; state.stage_cur < state.stage_max; state.stage_cur++) {

        if (!first_run && state.stage_cur % state.stats_update_freq == 0) {
            state.ShowStats();
        }

        InplaceMemoryFeedback::DiscardActive(std::move(inp_feed));
        inp_feed = 
            state.RunExecutorWithClassifyCounts(buf, len, exit_status, use_tmout);

        /* stop_soon is set by the handler for Ctrl+C. When it's pressed,
           we want to bail out quickly. */
        
        if (state.stop_soon || exit_status.exit_reason != state.crash_mode) 
            goto abort_calibration; // FIXME: goto

        if (!state.setting.dumb_mode && !state.stage_cur && !inp_feed.CountNonZeroBytes()) {
            exit_status.exit_reason = PUTExitReasonType::FAULT_NOINST;
            goto abort_calibration; // FIXME: goto
        }

        u32 cksum = inp_feed.CalcCksum32();
        
        if (testcase.exec_cksum != cksum) {
            inp_feed.ShowMemoryToFunc(
                [&state, &hnb](const u8* trace_bits, u32 map_size) {
                    hnb = HasNewBits(trace_bits, &state.virgin_bits[0], map_size, state);
                }
            );
            
            if (hnb > new_bits) new_bits = hnb;
            
            if (testcase.exec_cksum) {
                inp_feed.ShowMemoryToFunc(
                    [&state, &first_trace](const u8* trace_bits, u32 map_size) {
                        for (u32 i=0; i < map_size; i++) {
                            if (!state.var_bytes[i] && first_trace[i] != trace_bits[i]) {
                                state.var_bytes[i] = 1;
                                state.stage_max = AFLOption::CAL_CYCLES_LONG;
                            }
                        }
                    }
                );

                var_detected = true;
            } else {
                testcase.exec_cksum = cksum;
                inp_feed.ShowMemoryToFunc(
                    [&first_trace](const u8* trace_bits, u32 map_size) {
                        std::memcpy(first_trace.data(), trace_bits, map_size);
                    }
                );
            }
        }
    }

    stop_us = Util::GetCurTimeUs();

    state.total_cal_us += stop_us - start_us;
    state.total_cal_cycles += state.stage_max;

    testcase.exec_us = (stop_us - start_us) / state.stage_max;
    testcase.bitmap_size = inp_feed.CountNonZeroBytes();
    testcase.handicap = handicap;
    testcase.cal_failed = 0;

    state.total_bitmap_size += testcase.bitmap_size;
    state.total_bitmap_entries++;

    UpdateBitmapScore(testcase, state, inp_feed);

    /* If this case didn't result in new output from the instrumentation, tell
       parent. This is a non-critical problem, but something to warn the user
       about. */

    if ( !state.setting.dumb_mode 
      && first_run 
      && exit_status.exit_reason == PUTExitReasonType::FAULT_NONE 
      && new_bits == 0) {
        exit_status.exit_reason = PUTExitReasonType::FAULT_NOBITS;
    }

abort_calibration:
    
    if (new_bits == 2 && !testcase.has_new_cov) {
        testcase.has_new_cov = true;
        state.queued_with_cov++;
    }

    /* Mark variable paths. */

    if (var_detected) {
        state.var_byte_count = Util::CountBytes(&state.var_bytes[0], state.var_bytes.size());
        if (!testcase.var_behavior) {
            MarkAsVariable(state, testcase);
            state.queued_variable++;
        }
    }

    state.stage_name = old_sn;
    state.stage_cur  = old_sc;
    state.stage_max  = old_sm; 

    if (!first_run) state.ShowStats();

    return exit_status.exit_reason;
}

// Difference with AFL's add_to_queue: 
// if buf is not nullptr, then this function saves "buf" in a file specified by "fn"

std::shared_ptr<AFLTestcase> AddToQueue(
    AFLState &state,
    const std::string &fn,
    const u8 *buf,
    u32 len,
    bool passed_det
) {
    auto input = state.input_set.CreateOnDisk(fn);
    if (buf) {
        input->OverwriteThenUnload(buf, len);
    }

    std::shared_ptr<AFLTestcase> testcase( new AFLTestcase(std::move(input)) );

    testcase->depth = state.cur_depth + 1;
    testcase->passed_det = passed_det;

    if (testcase->depth > state.max_depth) state.max_depth = testcase->depth;

    state.case_queue.emplace_back(testcase);

    state.queued_paths++;
    state.pending_not_fuzzed++;
    state.cycles_wo_finds = 0;
    state.last_path_time = Util::GetCurTimeMs();

    if (state.queued_paths % 100 == 0) {
        printf("%u reached. time: %llu\n", state.queued_paths, state.last_path_time - state.start_time);
        if (state.queued_paths == 900) {
            exit(0);
        }
    }

    return testcase;
}

} // namespace util
} // namespace afl
