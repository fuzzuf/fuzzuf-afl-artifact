#include "Algorithms/AFL/AFLMutationHierarFlowRoutines.hpp"

#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

#include "Algorithms/AFL/AFLMutator.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"

namespace afl {
namespace pipeline {
namespace mutation {

BitFlip1WithAutoDictBuild::BitFlip1WithAutoDictBuild(AFLState &state)
        : state(state) {}
    
AFLMutCalleeRef
    BitFlip1WithAutoDictBuild::operator()(AFLMutator& mutator) 
{

    /*********************************************
     * SIMPLE BITFLIP (+dictionary construction) *
     *********************************************/

    state.prev_cksum = state.queue_cur_exec_cksum;
    
    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;

    state.stage_short = "flip1";
    state.stage_name = "bitflip 1/1";
    state.stage_max = mutator.GetLen() << 3;

    state.a_len = 0;
    state.a_collect.clear();
    state.SetShouldConstructAutoDict(false);

    for (state.stage_cur=0; state.stage_cur < state.stage_max; state.stage_cur++) {
        state.stage_cur_byte = state.stage_cur >> 3;
        
        // If the following conditions are met, consider appending Auto Extra values in the successor
        // For the details, check afl::pipeline::update::ConstructAutoDict

        if (!state.setting.dumb_mode) {
            state.SetShouldConstructAutoDict((state.stage_cur & 7) == 7);
        }

        mutator.FlipBit(state.stage_cur, 1);
        if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) {
            SetResponseValue(true);
            return GoToParent();
        }
        mutator.FlipBit(state.stage_cur, 1);
    }

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
    state.stage_finds[AFLOption::STAGE_FLIP1]  += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[AFLOption::STAGE_FLIP1] += state.stage_max;

    return GoToDefaultNext();
}

BitFlipOther::BitFlipOther(AFLState &state) : state(state) {}

AFLMutCalleeRef BitFlipOther::operator()(AFLMutator& mutator) {
    /*********************************************
     * SIMPLE BITFLIP                            *
     *********************************************/

    int stage_idxs[] = {
        AFLOption::STAGE_FLIP2, 
        AFLOption::STAGE_FLIP4
    };
    
    for (u32 bit_width=2, idx=0; bit_width <= 4; bit_width *= 2, idx++) {
        u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;

        state.stage_short = "flip" + std::to_string(bit_width);
        state.stage_name = "bitflip " + std::to_string(bit_width) + "/1";
        state.stage_max = (mutator.GetLen() << 3) + 1 - bit_width;

        for (state.stage_cur=0; state.stage_cur < state.stage_max; state.stage_cur++) {
            state.stage_cur_byte = state.stage_cur >> 3;
            
            mutator.FlipBit(state.stage_cur, bit_width);
            if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) {
                SetResponseValue(true);
                return GoToParent();
            }
            mutator.FlipBit(state.stage_cur, bit_width);
        }
            
        u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
        state.stage_finds[stage_idxs[idx]] += new_hit_cnt - orig_hit_cnt;
        state.stage_cycles[stage_idxs[idx]] += state.stage_max;
    }

    return GoToDefaultNext();
}

ByteFlip1WithEffMapBuild::ByteFlip1WithEffMapBuild(AFLState &state)
    : state(state) {}

AFLMutCalleeRef
        ByteFlip1WithEffMapBuild::operator()(AFLMutator& mutator) {
    /* Walking byte. */

    using afl::util::EFF_APOS;
    using afl::util::EFF_ALEN;
    
    state.eff_map.assign(EFF_ALEN(mutator.GetLen()), 0);
    state.eff_map[0] = 1;
    state.eff_cnt = 1;
    if (EFF_APOS(mutator.GetLen() - 1) != 0) {
        state.eff_map[EFF_APOS(mutator.GetLen() - 1)] = 1;
        state.eff_cnt++;
    }

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;
    u32 num_mutable_pos = mutator.GetLen();

    state.stage_short = "flip8";
    state.stage_name = "bitflip 8/8";
    state.stage_cur = 0;
    state.stage_max = num_mutable_pos;

    for (u32 i=0; i < num_mutable_pos; i++) {
        state.stage_cur_byte = i;

        mutator.FlipByte(state.stage_cur, 1);
        if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) {
            SetResponseValue(true);
            return GoToParent();
        }
        mutator.FlipByte(state.stage_cur, 1);

        state.stage_cur++;
    }

    if (state.eff_cnt != EFF_ALEN(mutator.GetLen()) && 
        state.eff_cnt * 100 / EFF_ALEN(mutator.GetLen()) > AFLOption::EFF_MAX_PERC) {
        
        std::memset(&state.eff_map[0], 1, EFF_ALEN(mutator.GetLen()));
        state.blocks_eff_select += EFF_ALEN(mutator.GetLen());
    } else {
        state.blocks_eff_select += state.eff_cnt;
    }

    state.blocks_eff_total += EFF_ALEN(mutator.GetLen());
    
    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
    state.stage_finds[AFLOption::STAGE_FLIP8]  += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[AFLOption::STAGE_FLIP8] += state.stage_max;

    return GoToDefaultNext();
}

ByteFlipOther::ByteFlipOther(AFLState &state) : state(state) {}

AFLMutCalleeRef ByteFlipOther::operator()(AFLMutator& mutator) {
    /* Walking byte. */

    using afl::util::EFF_APOS;

    int stage_idxs[] = {
        AFLOption::STAGE_FLIP16, 
        AFLOption::STAGE_FLIP32
    };

    for (u32 byte_width=2, idx=0; byte_width <= 4; byte_width *= 2, idx++) {
        // if the input is too short, then it's impossible
        if (mutator.GetLen() < byte_width) return GoToDefaultNext();

        u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;
        u32 num_mutable_pos = mutator.GetLen() + 1 - byte_width;

        state.stage_short = "flip" + std::to_string(byte_width * 8);
        state.stage_name = "bitflip " + std::to_string(byte_width * 8) + "/8";
        state.stage_cur = 0;
        state.stage_max = num_mutable_pos;

        for (u32 i=0; i < num_mutable_pos; i++) {
            // Why this is suffice to check \forall j(i <= j < i+byte_width) !eff_map[EFF_APOS(i)]?:
            // now since byte_width <= 8, |{EFF_APOS(j) | \forall j}| <= 2 holds

            if ( !state.eff_map[EFF_APOS(i)] 
              && !state.eff_map[EFF_APOS(i + byte_width - 1)] ) {

                state.stage_max--;
                continue;
            }

            state.stage_cur_byte = i;

            mutator.FlipByte(i, byte_width);
            if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) {
                SetResponseValue(true);
                return GoToParent();
            }
            mutator.FlipByte(i, byte_width);

            state.stage_cur++;
        }
        
        u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
        state.stage_finds[stage_idxs[idx]]  += new_hit_cnt - orig_hit_cnt;
        state.stage_cycles[stage_idxs[idx]] += state.stage_max;
    }

    return GoToDefaultNext();
}

Arith::Arith(AFLState &state) : state(state) {}

AFLMutCalleeRef Arith::operator()(AFLMutator& mutator) {
    /**********************
     * ARITHMETIC INC/DEC *
     **********************/

    if (state.no_arith) return GoToDefaultNext();

    if (DoArith<u8>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    if (mutator.GetLen() < 2) return GoToDefaultNext();

    if (DoArith<u16>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    if (mutator.GetLen() < 4) return GoToDefaultNext();

    if (DoArith<u32>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    return GoToDefaultNext();
}

Interest::Interest(AFLState &state) : state(state) {}

AFLMutCalleeRef Interest::operator()(AFLMutator& mutator) {
    /**********************
     * INTERESTING VALUES *
     **********************/   
    
    if (DoInterest<u8>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    if (state.no_arith || mutator.GetLen() < 2) return GoToDefaultNext();

    if (DoInterest<u16>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    if (mutator.GetLen() < 4) return GoToDefaultNext();

    if (DoInterest<u32>(mutator)) {
        SetResponseValue(true);
        return GoToParent();
    }
    return GoToDefaultNext();
}

UserDictInsert::UserDictInsert(AFLState &state) : state(state) {}

AFLMutCalleeRef UserDictInsert::operator()(AFLMutator& mutator) {
    // skip this step if the dictionary is empty
    if (state.extras.empty()) return GoToDefaultNext();

    /* Insertion of user-supplied extras. */

    state.stage_name = "user extras (insert)";
    state.stage_short = "ext_UI";
    state.stage_cur = 0;
    state.stage_max = state.extras.size() * (mutator.GetLen() + 1);

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;

    std::unique_ptr<u8[]> ex_tmp(new u8[mutator.GetLen() + AFLOption::MAX_DICT_FILE]);
    for (u32 i=0; i <= mutator.GetLen(); i++) {
        state.stage_cur_byte = i;

       /* Extras are sorted by size, from smallest to largest. This means
          that once len + extras[j].len > MAX_FILE holds, the same is true for 
          all the subsequent extras. */

        for (u32 j=0; j < state.extras.size(); j++) {
            auto &extra = state.extras[j];
            if (mutator.GetLen() + extra.data.size() > AFLOption::MAX_FILE) {
                s32 rem = state.extras.size() - j;
                state.stage_max -= rem;
                break;
            }

            /* Insert token */
            std::memcpy(ex_tmp.get() + i, &state.extras[j].data[0], state.extras[j].data.size());

            /* Copy tail */
            std::memcpy(ex_tmp.get() + i + state.extras[j].data.size(), mutator.GetBuf() + i, mutator.GetLen() - i);

            if (
                CallSuccessors(
                    ex_tmp.get(), 
                    mutator.GetLen() + state.extras[j].data.size()
                )
            ) {
                SetResponseValue(true);
                return GoToParent();
            }

            state.stage_cur++;
        }

        /* Copy head */
        ex_tmp[i] = mutator.GetBuf()[i];
    }

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;

    state.stage_finds[AFLOption::STAGE_EXTRAS_UI]  += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[AFLOption::STAGE_EXTRAS_UI] += state.stage_max;

    return GoToDefaultNext();
}

HavocBase::HavocBase(AFLState &state) : state(state) {}

// Because the process details of Havoc will be used also in Splicing, 
// we extract the "core" of Havoc into another HierarFlowRoutine, HavocBase.
// both Havoc and Splicing will inherit this and use DoHavoc
bool HavocBase::DoHavoc(
    AFLMutator& mutator, 
    const std::string &stage_name, 
    const std::string &stage_short, 
    u32 perf_score,
    s32 stage_max_multiplier, // see directly below
    int stage_idx
) {
    state.stage_name = stage_name;
    state.stage_short = stage_short;
    state.stage_max = stage_max_multiplier * perf_score / state.havoc_div / 100;
    state.stage_cur_byte = -1;

    if (state.stage_max < AFLOption::HAVOC_MIN) {
        state.stage_max = AFLOption::HAVOC_MIN;
    }

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;
    
    u64 havoc_queued = state.queued_paths;

    /* We essentially just do several thousand runs (depending on perf_score)
       where we take the input file and make random stacked tweaks. */

    for (state.stage_cur = 0; state.stage_cur < state.stage_max; state.stage_cur++) {
        using afl::util::UR;
        u32 use_stacking = 1 << (1 + UR(AFLOption::HAVOC_STACK_POW2, state.rand_fd));

        state.stage_cur_val = use_stacking;
        mutator.Havoc(use_stacking, state.extras, state.a_extras);
        if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;

        /* out_buf might have been mangled a bit, so let's restore it to its
           original size and shape. */

        mutator.RestoreHavoc();

        /* If we're finding new stuff, let's run for a bit longer, limits
           permitting. */

        if (state.queued_paths != havoc_queued) {
            if (perf_score <= AFLOption::HAVOC_MAX_MULT * 100) {
                state.stage_max *= 2;
                perf_score *= 2;
            }

            havoc_queued = state.queued_paths;
        }
    }

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
    state.stage_finds[stage_idx] += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[stage_idx] += state.stage_max;

    return false;
}

Havoc::Havoc(AFLState &state) : HavocBase(state) {}

AFLMutCalleeRef Havoc::operator()(AFLMutator& mutator) {
    s32 stage_max_multiplier;
    if (state.doing_det) stage_max_multiplier = AFLOption::HAVOC_CYCLES_INIT;
    else stage_max_multiplier = AFLOption::HAVOC_CYCLES;
    
    if (DoHavoc(
              mutator, 
              "havoc", "havoc", 
              state.orig_perf, stage_max_multiplier, 
              AFLOption::STAGE_HAVOC)) {
        SetResponseValue(true);
        return GoToParent();
    }

    return GoToDefaultNext();
}

Splicing::Splicing(AFLState &state) : HavocBase(state) {}

AFLMutCalleeRef Splicing::operator()(AFLMutator& mutator) {
    if (!state.use_splicing || state.setting.ignore_finds) return GoToDefaultNext();

    u32 splice_cycle = 0;
    while (splice_cycle++ < AFLOption::SPLICE_CYCLES 
        && state.queued_paths > 1 
        && mutator.GetSource().GetLen() > 1) {
        
        /* Pick a random queue entry and seek to it. Don't splice with yourself. */

        u32 tid;
        do { 
            using afl::util::UR;
            tid = UR(state.queued_paths, state.rand_fd);
        } while (tid == state.current_entry);

        /* Make sure that the target has a reasonable length. */

        while (tid < state.case_queue.size()) {
            if (state.case_queue[tid]->input->GetLen() >= 2 && 
                tid != state.current_entry) break;

            ++tid;
        }

        if (tid == state.case_queue.size()) continue;

        auto &target_case = *state.case_queue[tid];
        state.splicing_with = tid;

        /* Read the testcase into a new buffer. */

        target_case.input->Load();
        bool success = mutator.Splice(*target_case.input);
        target_case.input->Unload();

        if (!success) {
            continue;
        }

        if (DoHavoc(mutator,
                         Util::StrPrintf("splice %u", splice_cycle), 
                         "splice",
                         state.orig_perf, AFLOption::SPLICE_HAVOC,
                         AFLOption::STAGE_SPLICE)) {
            SetResponseValue(true);
            return GoToParent();
        }

        mutator.RestoreSplice();
    }

    return GoToDefaultNext();
}

} // namespace mutation
} // namespace pipeline
} // namespace afl
