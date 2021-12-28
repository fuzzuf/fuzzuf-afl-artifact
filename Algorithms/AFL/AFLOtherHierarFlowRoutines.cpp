#include "Algorithms/AFL/AFLOtherHierarFlowRoutines.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

#include "Algorithms/AFL/AFLMacro.hpp"
#include "Logger/Logger.hpp"

namespace afl {
namespace pipeline {

// pipeline steps other than mutations and updates
namespace other {

SelectSeed::SelectSeed(AFLState &state) : state(state) {}

NullableRef<HierarFlowCallee<void(void)>> SelectSeed::operator()(void) {
    state.current_entry++;

    if (state.current_entry >= state.case_queue.size()) {
        state.queue_cycle++;
        state.current_entry = state.seek_to; // seek_to is used in resume mode
        state.seek_to = 0;
        state.cur_skipped_paths = 0;

        state.ShowStats();
        if (state.not_on_tty) {
            ACTF("Entering queue cycle %llu.", state.queue_cycle);
            fflush(stdout);
        }

        /* If we had a full queue cycle with no new finds, try
           recombination strategies next. */

        if (state.queued_paths == prev_queued) {
            if (state.use_splicing) state.cycles_wo_finds++;
            else state.use_splicing = true;
        } else state.cycles_wo_finds = 0;

        prev_queued = state.queued_paths;

#if 0
        if (sync_id && queue_cycle == 1 && getenv("AFL_IMPORT_FIRST"))
            sync_fuzzers(use_argv);
#endif

    }

    // FIXME: here assert is used
    // this assert ensures that the container "case_queue" has the key "state.current_entry" (since case_queue is a vector)
    assert(state.current_entry < state.case_queue.size());

    // get the testcase indexed by state.current_entry and start mutations
    auto& testcase = state.case_queue[state.current_entry];
    CallSuccessors(testcase);

#if 0
    auto skipped_fuzz = CallSuccessors(testcase);

    if (!state.stop_soon && state.sync_id && !skipped_fuzz) {
      if (!(state.sync_interval_cnt++ % AFLOption::SYNC_INTERVAL))
        SyncFuzzers(use_argv);
    }
#endif

    return GoToDefaultNext();
}

ConsiderSkipMut::ConsiderSkipMut(AFLState &state) : state(state) {}

// Randomly, sometimes skip the entire process of mutations
AFLMidCalleeRef ConsiderSkipMut::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    // just an alias of afl::util::UR
    auto UR = [this](u32 limit) {
        return afl::util::UR(limit, state.rand_fd);
    };

    if (state.setting.ignore_finds) {
        if (testcase->depth > 1) {
            SetResponseValue(true);
            return GoToParent();
        }
    } else {
        if (state.pending_favored) {
            if ( (testcase->was_fuzzed || !testcase->favored) 
              && UR(100) < AFLOption::SKIP_TO_NEW_PROB) {
                SetResponseValue(true);
                return GoToParent();
            }
        } else if (!state.setting.dumb_mode && 
                   !testcase->favored && 
                   state.queued_paths > 10) {
            if (state.queue_cycle > 1 && !testcase->was_fuzzed) {
                if (UR(100) < AFLOption::SKIP_NFAV_NEW_PROB) {
                    SetResponseValue(true);
                    return GoToParent();
                }
            } else {
                if (UR(100) < AFLOption::SKIP_NFAV_OLD_PROB) {
                    SetResponseValue(true);
                    return GoToParent();
                }
            }
        }
    }

    if (state.not_on_tty) {
        ACTF("Fuzzing test case #%u (%u total, %llu uniq crashes found)...",
           state.current_entry, state.queued_paths, state.unique_crashes);
        fflush(stdout);
    }

    // We don't use LoadByMmap here because we may modify
    // the underlying file in calibration and trimming.
    // The original AFL also should not do that though it does.
    testcase->input->Load();

    return GoToDefaultNext();
}

RetryCalibrate::RetryCalibrate(
    AFLState &state,
    AFLMidCalleeRef abandon_entry
) : state(state), 
    abandon_entry(abandon_entry) {}

AFLMidCalleeRef RetryCalibrate::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    auto& input = *testcase->input;
   
    // Mutator should be allocated after trimming is done
    // auto mutator = Mutator( input );

    state.subseq_tmouts = 0;
    state.cur_depth = testcase->depth;

    /*******************************************
     * CALIBRATION (only if failed earlier on) * 
     *******************************************/

    if (testcase->cal_failed == 0) return GoToDefaultNext();

    PUTExitReasonType res = PUTExitReasonType::FAULT_TMOUT;
    if (testcase->cal_failed < AFLOption::CAL_CHANCES) {
        /* Reset exec_cksum to tell calibrate_case to re-execute the testcase
           avoiding the usage of an invalid trace_bits.
           For more info: https://github.com/AFLplusplus/AFLplusplus/pull/425 */

        testcase->exec_cksum = 0;

        // There should be no active instance of InplaceMemoryFeedback at this point. 
        // So we can just create a temporary instance to get a result.
        InplaceMemoryFeedback inp_feed; 
        ExitStatusFeedback exit_status;
        res = afl::util::CalibrateCaseWithFeedDestroyed(
            *testcase,
            input.GetBuf(), 
            input.GetLen(),
            state,
            inp_feed,
            exit_status,
            state.queue_cycle - 1,
            false
        );
        if (res == PUTExitReasonType::FAULT_ERROR) 
            ERROR("Unable to execute target application");
    }

    // FIXME: state.setting.crash_mode?
    if (state.stop_soon || res != state.crash_mode) {
        state.cur_skipped_paths++;
        SetResponseValue(true);
        return abandon_entry;
    }

    return GoToDefaultNext();
}

TrimCase::TrimCase(
    AFLState &state,
    AFLMidCalleeRef abandon_entry
) : state(state), 
    abandon_entry(abandon_entry) {}

/* Trim all new test cases to save cycles when doing deterministic checks. The
   trimmer uses power-of-two increments somewhere between 1/16 and 1/1024 of
   file size, to keep the stage short and sweet. */
static PUTExitReasonType DoTrimCase(AFLState &state, AFLTestcase &testcase) {
    auto &input = *testcase.input;

    /* Although the trimmer will be less useful when variable behavior is
       detected, it will still work to some extent, so we don't check for
       this. */

    if (input.GetLen() < 5) return PUTExitReasonType::FAULT_NONE;

    state.bytes_trim_in += input.GetLen();
    
    /* Select initial chunk len, starting with large steps. */
    u32 len_p2 = Util::NextP2(input.GetLen());

    u32 remove_len = std::max(
                        len_p2 / AFLOption::TRIM_START_STEPS, 
                        AFLOption::TRIM_MIN_BYTES
                     );

    /* Continue until the number of steps gets too high or the stepover
       gets too small. */

    // end_len is re-calculated everytime len_p2 is changed
    u32 end_len = std::max(
                      len_p2 / AFLOption::TRIM_END_STEPS, 
                      AFLOption::TRIM_MIN_BYTES
                  );
    
    PersistentMemoryFeedback pers_feed;
    u32 trim_exec = 0;
    bool needs_write = false;
    PUTExitReasonType fault = PUTExitReasonType::FAULT_NONE;
    while (remove_len >= end_len) {
        u32 remove_pos = remove_len;

        state.stage_name = Util::StrPrintf(
                              "trim %s/%s", 
                              afl::util::DescribeInteger(remove_len).c_str(),
                              afl::util::DescribeInteger(remove_len).c_str()
                           );

        state.stage_cur = 0;
        state.stage_max = input.GetLen() / remove_len;

        while (remove_pos < input.GetLen()) { 
            u32 trim_avail = std::min(remove_len, input.GetLen() - remove_pos);

            // FIXME: we can't represent this in fuzzuf without preparing another buf
            // write_with_gap(in_buf, q->len, remove_pos, trim_avail);

            u32 test_len = input.GetLen() - trim_avail;
            std::unique_ptr<u8[]> test_buf(new u8[test_len]);
            u32 move_tail = input.GetLen() - remove_pos - trim_avail;

            std::memcpy(test_buf.get(), input.GetBuf(), remove_pos);
            std::memmove(
                test_buf.get() + remove_pos, 
                input.GetBuf() + remove_pos + trim_avail,
                move_tail
            );

            ExitStatusFeedback exit_status;

            InplaceMemoryFeedback inp_feed = state.RunExecutorWithClassifyCounts(
                                                 test_buf.get(), 
                                                 test_len,
                                                 exit_status
                                             );
            fault = exit_status.exit_reason;
            state.trim_execs++;

            if ( state.stop_soon 
              || fault == PUTExitReasonType::FAULT_ERROR) 
                goto abort_trimming; //FIXME: goto

            /* Note that we don't keep track of crashes or hangs here; maybe TODO? */

            u32 cksum = inp_feed.CalcCksum32();

            /* If the deletion had no impact on the trace, make it permanent. This
               isn't perfect for variable-path inputs, but we're just making a
               best-effort pass, so it's not a big deal if we end up with false
               negatives every now and then. */

            if (cksum == testcase.exec_cksum) {
                input.OverwriteKeepingLoaded(std::move(test_buf), test_len);

                len_p2 = Util::NextP2(input.GetLen());
                end_len = std::max(
                              len_p2 / AFLOption::TRIM_END_STEPS,
                              AFLOption::TRIM_MIN_BYTES
                          );

                /* Let's save a clean trace, which will be needed by
                   update_bitmap_score once we're done with the trimming stuff. */

                if (!needs_write) {
                    needs_write = true;
                    pers_feed = inp_feed.ConvertToPersistent();
                }
            } else {
                remove_pos += remove_len;
            }

            /* Since this can be slow, update the screen every now and then. */

            if (trim_exec % state.stats_update_freq == 0) state.ShowStats();
            trim_exec++;
            state.stage_cur++;
        }

        remove_len >>= 1;
    }

    /* If we have made changes to in_buf, we also need to update the on-disk
       version of the test case. */

    if (needs_write) {
        // already saved on disk by "input.OverwriteKeepingLoaded()"

        afl::util::UpdateBitmapScoreWithRawTrace(
            testcase, 
            state, 
            pers_feed.mem.get(), 
            AFLOption::MAP_SIZE
        );
    }

abort_trimming:
    state.bytes_trim_out += input.GetLen();
    return fault;
}

AFLMidCalleeRef TrimCase::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    /************
     * TRIMMING *
     ************/
    if (state.setting.dumb_mode || testcase->trim_done) return GoToDefaultNext();


    PUTExitReasonType res = DoTrimCase(state, *testcase);
    if (res == PUTExitReasonType::FAULT_ERROR) 
        ERROR("Unable to execute target application");

    if (state.stop_soon) {
        state.cur_skipped_paths++;
        SetResponseValue(true);
        return abandon_entry;
    }

    testcase->trim_done = true;
    return GoToDefaultNext();
}

CalcScore::CalcScore(AFLState &state) : state(state) {}

static u32 DoCalcScore(AFLState &state, AFLTestcase &testcase) {
    u32 avg_exec_us = state.total_cal_us / state.total_cal_cycles;
    u32 avg_bitmap_size = state.total_bitmap_size / state.total_bitmap_entries;
    u32 perf_score = 100;

    /* Adjust score based on execution speed of this path, compared to the
       global average. Multiplier ranges from 0.1x to 3x. Fast inputs are
       less expensive to fuzz, so we're giving them more air time. */

    if (testcase.exec_us * 0.1 > avg_exec_us) perf_score = 10;
    else if (testcase.exec_us * 0.25 > avg_exec_us) perf_score = 25;
    else if (testcase.exec_us * 0.5 > avg_exec_us) perf_score = 50;
    else if (testcase.exec_us * 0.75 > avg_exec_us) perf_score = 75;
    else if (testcase.exec_us * 4 < avg_exec_us) perf_score = 300;
    else if (testcase.exec_us * 3 < avg_exec_us) perf_score = 200;
    else if (testcase.exec_us * 2 < avg_exec_us) perf_score = 150;

    /* Adjust score based on bitmap size. The working theory is that better
       coverage translates to better targets. Multiplier from 0.25x to 3x. */

    if (testcase.bitmap_size * 0.3 > avg_bitmap_size) perf_score *= 3;
    else if (testcase.bitmap_size * 0.5 > avg_bitmap_size) perf_score *= 2;
    else if (testcase.bitmap_size * 0.75 > avg_bitmap_size) perf_score *= 1.5;
    else if (testcase.bitmap_size * 3 < avg_bitmap_size) perf_score *= 0.25;
    else if (testcase.bitmap_size * 2 < avg_bitmap_size) perf_score *= 0.5;
    else if (testcase.bitmap_size * 1.5 < avg_bitmap_size) perf_score *= 0.75;

    /* Adjust score based on handicap. Handicap is proportional to how late
       in the game we learned about this path. Latecomers are allowed to run
       for a bit longer until they catch up with the rest. */

    if (testcase.handicap >= 4) {
      perf_score *= 4;
      testcase.handicap -= 4;
    } else if (testcase.handicap) {
      perf_score *= 2;
      testcase.handicap--;
    }

    /* Final adjustment based on input depth, under the assumption that fuzzing
       deeper test cases is more likely to reveal stuff that can't be
       discovered with traditional fuzzers. */

    switch (testcase.depth) {
    case 0 ... 3: 
        break;
    case 4 ... 7: 
        perf_score *= 2;
        break;
    case 8 ... 13: 
        perf_score *= 3;
        break;
    case 14 ... 25: 
        perf_score *= 4;
        break;
    default:
        perf_score *= 5;
        break;
    }

    /* Make sure that we don't go over limit. */

    if (perf_score > AFLOption::HAVOC_MAX_MULT * 100) {
        perf_score = AFLOption::HAVOC_MAX_MULT * 100;
    }

    return perf_score;
}

AFLMidCalleeRef CalcScore::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    /*********************
     * PERFORMANCE SCORE *
     *********************/

    state.orig_perf = DoCalcScore(state, *testcase);
    return GoToDefaultNext();
}

ApplyDetMuts::ApplyDetMuts(
    AFLState &state,
    AFLMidCalleeRef abandon_entry
) : state(state), 
    abandon_entry(abandon_entry) {}

AFLMidCalleeRef ApplyDetMuts::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {

    // We no longer modify this testcase.
    // So we can reload the file with mmap.
    testcase->input->LoadByMmap(); // no need to Unload

    /* Skip right away if -d is given, if we have done deterministic fuzzing on
       this entry ourselves (was_fuzzed), or if it has gone through deterministic
       testing in earlier, resumed runs (passed_det). */

    if (state.skip_deterministic || testcase->was_fuzzed || testcase->passed_det) {
        state.doing_det = false;
        return GoToDefaultNext();
    }

    /* Skip deterministic fuzzing if exec path checksum puts this out of scope
       for this master instance. */

    if ( state.master_max 
      && (testcase->exec_cksum % state.master_max) != state.master_id - 1) {
        state.doing_det = false;
        return GoToDefaultNext();
    }

    state.doing_det = true;

    auto mutator = AFLMutator( *testcase->input, state );

    state.stage_val_type = AFLOption::STAGE_VAL_NONE;

    // this will be required in dictionary construction and eff_map construction
    state.queue_cur_exec_cksum = testcase->exec_cksum;

    // call deterministic mutations
    // if they return true, then we should go to abandon_entry
    auto should_abandon_entry = CallSuccessors(mutator);

    if (should_abandon_entry) {
        SetResponseValue(true);
        return abandon_entry;
    }

    // NOTE: "if (!testcase->passed_det)" seems unnecessary to me
    // because passed_det == 0 always holds here
    if (!testcase->passed_det) afl::util::MarkAsDetDone(state, *testcase);

    return GoToDefaultNext();
}

ApplyRandMuts::ApplyRandMuts(
    AFLState &state,
    AFLMidCalleeRef abandon_entry
) : state(state), 
    abandon_entry(abandon_entry) {}

AFLMidCalleeRef ApplyRandMuts::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    auto mutator = AFLMutator( *testcase->input, state );

    // call probablistic mutations
    // if they return true, then we should go to abandon_entry
    auto should_abandon_entry = CallSuccessors(mutator);
    if (should_abandon_entry) {
        SetResponseValue(true);
        return abandon_entry;
    }

    return GoToDefaultNext();
}

AbandonEntry::AbandonEntry(AFLState &state) : state(state) {}

AFLMidCalleeRef AbandonEntry::operator()(
    std::shared_ptr<AFLTestcase> testcase
) {
    state.splicing_with = -1;

    /* Update pending_not_fuzzed count if we made it through the calibration
       cycle and have not seen this entry before. */

    if (!state.stop_soon && !testcase->cal_failed && !testcase->was_fuzzed) {
        testcase->was_fuzzed = true;
        state.pending_not_fuzzed--;
        if (testcase->favored) state.pending_favored--;
    }

    testcase->input->Unload();

    // ReponseValue should be set in previous steps, so do nothing here
    return GoToDefaultNext();
}

ExecutePUT::ExecutePUT(AFLState &state) : state(state) {}

NullableRef<HierarFlowCallee<bool(const u8*, u32)>> ExecutePUT::operator()(
    const u8 *input,
    u32 len
) {
    ExitStatusFeedback exit_status;

    auto inp_feed = state.RunExecutorWithClassifyCounts(input, len, exit_status);
    CallSuccessors(input, len, inp_feed, exit_status);
    return GoToDefaultNext();
}

} // namespace other
} // namespace pipeline
} // namespace afl
