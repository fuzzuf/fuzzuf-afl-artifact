#include "Algorithms/AFL/AFLUpdateHierarFlowRoutines.hpp"

#include <string>
#include <utility>
#include <functional>
#include <memory>

#include "Mutator/Mutator.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"
#include "Algorithms/AFL/AFLTestcase.hpp"
#include "Algorithms/AFL/AFLMacro.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"
#include "Feedback/PUTExitReasonType.hpp"
#include "Logger/Logger.hpp"

namespace afl {
namespace pipeline {
namespace update {

static std::string DescribeOp(AFLState &state, u8 hnb) {
    std::string ret;

    if (!state.syncing_party.empty()) {
        ret = Util::StrPrintf("sync:%s,src:%06u", 
                  state.syncing_party.c_str(), state.syncing_case);
    } else {
        ret = Util::StrPrintf("src:%06u", state.current_entry);

        if (state.splicing_with >= 0) {
            ret += Util::StrPrintf("+%06u", state.splicing_with);
        }

        ret += ",op:" + state.stage_short;

        if (state.stage_cur_byte >= 0) {
            ret += Util::StrPrintf(",pos:%u", state.stage_cur_byte);
            
            if (state.stage_val_type != AFLOption::STAGE_VAL_NONE) {
                ret += Util::StrPrintf(",val:%s%+d", 
                          (state.stage_val_type == AFLOption::STAGE_VAL_BE) ? "be:" : "",
                          state.stage_cur_val);
            } 
        } else {
            ret += Util::StrPrintf(",rep:%u", state.stage_cur_val);
        }
    }

    if (hnb == 2) ret += ",+cov";
    
    return ret;
}

static bool CheckEqualNocase(const std::vector<u8> &m1, const std::vector<u8> &m2) {
    // NOTE: this function implicitly assumes m1.size() == m2.size()
    u32 len = m1.size(); 

    for (u32 i=0; i<len; i++) {
        if (std::tolower(m1[i]) != std::tolower(m2[i])) return false;
    }

    return true;
}

static void MaybeAddAuto(AFLState &state, const std::vector<u8> &mem) {
    /* Allow users to specify that they don't want auto dictionaries. */

    if (AFLOption::MAX_AUTO_EXTRAS == 0 || AFLOption::USE_AUTO_EXTRAS == 0) return;

    /* Skip runs of identical bytes. */

    if (mem == std::vector<u8>(mem.size(), mem[0])) return;

    /* Reject builtin interesting values. */

    if (mem.size() == 2) {
        u16 converted = *(u16*)mem.data();
        for (auto val : Mutator::interesting_16) {
            if (converted == (u16)val || converted == SWAP16(val)) return;
        }
    } else if (mem.size() == 4) {
        u32 converted = *(u32*)mem.data();
        for (auto val : Mutator::interesting_32) {
            if (converted == (u32)val || converted == SWAP32(val)) return;
        }
    }

    /* Reject anything that matches existing extras. Do a case-insensitive
       match. We optimize by exploiting the fact that extras[] are sorted
       by size. */

    auto itr = state.extras.begin();
    for (; itr != state.extras.end(); itr++)  {
        if (itr->data.size() >= mem.size()) break;
    }

    for (; itr != state.extras.end() && itr->data.size() == mem.size(); itr++) {
        if (CheckEqualNocase(itr->data, mem)) return;
    }

    /* Last but not least, check a_extras[] for matches. There are no
       guarantees of a particular sort order. */

    state.auto_changed = 1;

    bool will_append = true;
    for (auto &extra : state.a_extras) {
        if (extra.data.size() == mem.size() && CheckEqualNocase(extra.data, mem)) {
            extra.hit_cnt++;
            will_append = false;
            break;
        }
    }

    if (will_append) {
        /* At this point, looks like we're dealing with a new entry. So, let's
           append it if we have room. Otherwise, let's randomly evict some other
           entry from the bottom half of the list. */
    
        if (state.a_extras.size() < AFLOption::MAX_AUTO_EXTRAS) {
            state.a_extras.emplace_back(AFLDictData{mem, 0});
        } else {
            using afl::util::UR;

            int idx = AFLOption::MAX_AUTO_EXTRAS / 2;
            idx += UR((AFLOption::MAX_AUTO_EXTRAS + 1) / 2, state.rand_fd);
    
            state.a_extras[idx].data = mem;
            state.a_extras[idx].hit_cnt = 0;
        }
    }

    std::sort(state.a_extras.begin(), state.a_extras.end(), 
        [](const AFLDictData& e1, const AFLDictData& e2) {
            return e1.hit_cnt > e2.hit_cnt;
        }
    );

    size_t lim = std::min<size_t>(AFLOption::USE_AUTO_EXTRAS, state.a_extras.size());
    std::sort(state.a_extras.begin(), state.a_extras.begin() + lim, 
        [](const AFLDictData& e1, const AFLDictData& e2) {
            return e1.data.size() < e2.data.size();
        }
    );
}

static bool SaveIfInteresting(
    AFLState &state,
    const u8 *buf, 
    u32 len, 
    InplaceMemoryFeedback &inp_feed,
    ExitStatusFeedback &exit_status
) {
    bool keeping = false;

    std::string fn;
    if (exit_status.exit_reason == state.crash_mode) {
        /* Keep only if there are new bits in the map, add to queue for
           future fuzzing, etc. */

        u8 hnb;

        inp_feed.ShowMemoryToFunc(
            [&state, &hnb](const u8* trace_bits, u32 map_size) {
                hnb = afl::util::HasNewBits(trace_bits, &state.virgin_bits[0], map_size, state);
            }
        );

        if (!hnb) {
            if (state.crash_mode == PUTExitReasonType::FAULT_CRASH) {
                state.total_crashes++;
            }
            return false;
        }

        if (!state.setting.simple_files) {
            fn = Util::StrPrintf("%s/queue/id:%06u,%s", 
                                    state.setting.out_dir.c_str(), 
                                    state.queued_paths, 
                                    DescribeOp(state, hnb).c_str()
                                );
        } else {
            fn = Util::StrPrintf("%s/queue/id_%06u", 
                                    state.setting.out_dir.c_str(), 
                                    state.queued_paths
                                );
        }

        auto testcase = afl::util::AddToQueue(state, fn, buf, len, false);
        if (hnb == 2) {
            testcase->has_new_cov = 1;
            state.queued_with_cov++;
        }

        testcase->exec_cksum = inp_feed.CalcCksum32();
        
        // inp_feed will may be discard to start a new execution
        // in that case inp_feed will receive the new feedback 
        PUTExitReasonType res = afl::util::CalibrateCaseWithFeedDestroyed(
                  *testcase, 
                  buf, len, 
                  state,
                  inp_feed,
                  exit_status,
                  state.queue_cycle - 1,
                  false);

        if (res == PUTExitReasonType::FAULT_ERROR) {
            ERROR("Unable to execute target application");
        }

        keeping = true;
    } 

    switch (exit_status.exit_reason) {
    case PUTExitReasonType::FAULT_TMOUT:
        
        /* Timeouts are not very interesting, but we're still obliged to keep
           a handful of samples. We use the presence of new bits in the
           hang-specific bitmap as a signal of uniqueness. In "dumb" mode, we
           just keep everything. */

        state.total_tmouts++;
        if (state.unique_hangs >= AFLOption::KEEP_UNIQUE_HANG) {
            // originally here "return keeping" is used, but this is clearer right?
            return false; 
        }

        if (!state.setting.dumb_mode) {
            if constexpr (sizeof(size_t) == 8) {
                inp_feed.ModifyMemoryWithFunc(
                    [](u8* trace_bits, u32 map_size) {
                        afl::util::SimplifyTrace<u64>((u64*)trace_bits, map_size);
                    }
                );
            } else {
                inp_feed.ModifyMemoryWithFunc(
                    [](u8* trace_bits, u32 map_size) {
                        afl::util::SimplifyTrace<u32>((u32*)trace_bits, map_size);
                    }
                );
            }

            u8 res;
            inp_feed.ShowMemoryToFunc(
                [&state, &res](const u8* trace_bits, u32 map_size) {
                    res = afl::util::HasNewBits(trace_bits, &state.virgin_tmout[0], map_size, state);
                }
            );

            if (!res) {
                // originally here "return keeping" is used, but this is clearer right?
                return false;
            }
        }

        state.unique_tmouts++;

        /* Before saving, we make sure that it's a genuine hang by re-running
           the target with a more generous timeout (unless the default timeout
           is already generous). */

        if (state.setting.exec_timelimit_ms < state.hang_tmout) {
            // discard inp_feed here because we will use executor
            InplaceMemoryFeedback::DiscardActive(std::move(inp_feed));
            inp_feed = state.RunExecutorWithClassifyCounts(
                                buf, len, exit_status, state.hang_tmout);

            /* A corner case that one user reported bumping into: increasing the
               timeout actually uncovers a crash. Make sure we don't discard it if
               so. */

            if (!state.stop_soon && exit_status.exit_reason == PUTExitReasonType::FAULT_CRASH) {
                goto keep_as_crash; // FIXME: goto
            }

            if ( state.stop_soon 
              || exit_status.exit_reason != PUTExitReasonType::FAULT_TMOUT) {
                return false;
            }
        }

        if (!state.setting.simple_files) {
            fn = Util::StrPrintf("%s/hangs/id:%06llu,%s", 
                                    state.setting.out_dir.c_str(),
                                    state.unique_hangs, 
                                    DescribeOp(state, 0).c_str()
                                );
        } else {
            fn = Util::StrPrintf("%s/hangs/id_%06llu", 
                                    state.setting.out_dir.c_str(),
                                    state.unique_hangs
                                );
        }

        state.unique_hangs++;
        state.last_hang_time = Util::GetCurTimeMs();
        break;

    case PUTExitReasonType::FAULT_CRASH:
keep_as_crash:
        
        /* This is handled in a manner roughly similar to timeouts,
           except for slightly different limits and no need to re-run test
           cases. */

        state.total_crashes++;

        if (state.unique_crashes >= AFLOption::KEEP_UNIQUE_CRASH) {
            // unlike FAULT_TMOUT case, keeping can be true when "crash mode" is enabled 
            return keeping;
        }

        if (!state.setting.dumb_mode) {
            if constexpr (sizeof(size_t) == 8) {
                inp_feed.ModifyMemoryWithFunc(
                    [](u8* trace_bits, u32 map_size) {
                        afl::util::SimplifyTrace<u64>((u64*)trace_bits, map_size);
                    }
                );
            } else {
                inp_feed.ModifyMemoryWithFunc(
                    [](u8* trace_bits, u32 map_size) {
                        afl::util::SimplifyTrace<u32>((u32*)trace_bits, map_size);
                    }
                );
            }

            u8 res;
            inp_feed.ShowMemoryToFunc(
                [&state, &res](const u8* trace_bits, u32 map_size) {
                    res = afl::util::HasNewBits(trace_bits, &state.virgin_crash[0], map_size, state);
                }
            );

            if (!res) {
                // unlike FAULT_TMOUT case, keeping can be true when "crash mode" is enabled 
                return keeping;
            }
        }

#if 0
        if (!state.unique_crashes) WriteCrashReadme(); // FIXME?
#endif

        if (!state.setting.simple_files) {
            fn = Util::StrPrintf("%s/crashes/id:%06llu,sig:%02u,%s", 
                                    state.setting.out_dir.c_str(),
                                    state.unique_crashes,
                                    exit_status.signal,
                                    DescribeOp(state, 0).c_str()
                                );
        } else {
            fn = Util::StrPrintf("%s/hangs/id_%06llu_%02u", 
                                    state.setting.out_dir.c_str(),
                                    state.unique_crashes,
                                    exit_status.signal
                                );
        }

        state.unique_crashes++;

        state.last_crash_time = Util::GetCurTimeMs();
        state.last_crash_execs = state.total_execs;

        break;
        
    case PUTExitReasonType::FAULT_ERROR:
        ERROR("Unable to execute target application");

    default:
        return keeping;
    }

    /* If we're here, we apparently want to save the crash or hang
       test case, too. */

    int fd = Util::OpenFile(fn, O_WRONLY | O_CREAT | O_EXCL, 0600);
    Util::WriteFile(fd, buf, len);
    Util::CloseFile(fd);

    return keeping;
}

NormalUpdate::NormalUpdate(AFLState &state) : state(state) {}

/* Write a modified test case, run program, process results. Handle
   error conditions, returning true if it's time to bail out. */

AFLUpdCalleeRef NormalUpdate::operator()(
    const u8 *buf, 
    u32 len, 
    InplaceMemoryFeedback& inp_feed,
    ExitStatusFeedback &exit_status
) {
    if (state.stop_soon) {
        SetResponseValue(true);
        return GoToParent();
    }

    if (exit_status.exit_reason == PUTExitReasonType::FAULT_TMOUT) {
        if (state.subseq_tmouts++ > AFLOption::TMOUT_LIMIT) {
            state.cur_skipped_paths++;
            SetResponseValue(true);
            return GoToParent();
        }
    } else state.subseq_tmouts = 0;

    // FIXME: to handle SIGUSR1, we need to reconsider how and where we implement signal handlers
#if 0  
    /* Users can hit us with SIGUSR1 to request the current input
       to be abandoned. */

    if (state.skip_requested) {
        state.skip_requested = false;
        state.cur_skipped_paths++;
        SetResponseValue(true);
        return GoToParent();
    }
#endif
    
    if (SaveIfInteresting(state, buf, len, inp_feed, exit_status)) {
        state.queued_discovered++;
    }

    if ( state.stage_cur % state.stats_update_freq == 0
      || state.stage_cur + 1 == state.stage_max) {
        state.ShowStats();
    }

    return GoToDefaultNext();
}

ConstructAutoDict::ConstructAutoDict(AFLState &state) : state(state) {}

// Deal with dictionary construction in bitflip 1/1 stage
AFLUpdCalleeRef ConstructAutoDict::operator()(
    const u8 *buf, 
    u32 /* unused */, 
    InplaceMemoryFeedback& inp_feed,
    ExitStatusFeedback & /* unused */ 
) {
    if (!state.ShouldConstructAutoDict()) return GoToDefaultNext();

    u32 cksum = inp_feed.CalcCksum32();

    if (state.stage_cur == state.stage_max - 1 && cksum == state.prev_cksum) {

        /* If at end of file and we are still collecting a string, grab the
           final character and force output. */

        if (state.a_len < AFLOption::MAX_AUTO_EXTRA) {
            // At this point, the content of buf is different from the original AFL,
            // because the original one does this procedure AFTER restoring buf.
            state.a_collect.emplace_back(buf[state.stage_cur >> 3] ^ 1);
        }

        // NOTE: why do we have a_len? Isn't a_collect.size() enough?
        // In the original AFL, a_len can be MAX_AUTO_EXTRA+1:
        // when AFL tries to append a character while a_len == MAX_AUTO_EXTRA,
        // it fails to do that, only to increment a_len.
        // This is considered as the problem of the original AFL,
        // but we still follow the original one to have consistency.
        state.a_len++;
        
        if (AFLOption::MIN_AUTO_EXTRA <= state.a_len && state.a_len <= AFLOption::MAX_AUTO_EXTRA) {
            MaybeAddAuto(state, state.a_collect);
        }
    } else if (cksum != state.prev_cksum) {

        /* Otherwise, if the checksum has changed, see if we have something
           worthwhile queued up, and collect that if the answer is yes. */

        if (AFLOption::MIN_AUTO_EXTRA <= state.a_len && state.a_len <= AFLOption::MAX_AUTO_EXTRA) {
            MaybeAddAuto(state, state.a_collect);
        }
        
        state.a_collect.clear();
        state.a_len = 0;
        state.prev_cksum = cksum;
    }

    /* Continue collecting string, but only if the bit flip actually made
       any difference - we don't want no-op tokens. */

    if (cksum != state.queue_cur_exec_cksum) {
        if (state.a_len < AFLOption::MAX_AUTO_EXTRA) {
            state.a_collect.emplace_back(buf[state.stage_cur >> 3] ^ 1);
        }
        state.a_len++;
    }

    return GoToDefaultNext();
}

ConstructEffMap::ConstructEffMap(AFLState &state) : state(state) {}

// Deal with eff_map construction in bitflip 8/8 stage
AFLUpdCalleeRef ConstructEffMap::operator()(
    const u8 * /* unused */, 
    u32 len, 
    InplaceMemoryFeedback& inp_feed,
    ExitStatusFeedback & /* unused */ 
) {
    /* We also use this stage to pull off a simple trick: we identify
       bytes that seem to have no effect on the current execution path
       even when fully flipped - and we skip them during more expensive
       deterministic stages, such as arithmetics or known ints. */

    using afl::util::EFF_APOS;

    if (!state.eff_map[EFF_APOS(state.stage_cur)]) {
        bool set_eff_map_bit = false;
        if (state.setting.dumb_mode || len < AFLOption::EFF_MIN_LEN) {
            set_eff_map_bit = true;
        } else {
            u32 cksum = inp_feed.CalcCksum32();
            if (cksum != state.queue_cur_exec_cksum) {
                set_eff_map_bit = true;
            }
        }

        if (set_eff_map_bit) {
            state.eff_map[EFF_APOS(state.stage_cur)] = 1;
            state.eff_cnt++;
        }
    }

    return GoToDefaultNext();
}

} // namespace update
} // namespace pipeline
} // namespace afl
