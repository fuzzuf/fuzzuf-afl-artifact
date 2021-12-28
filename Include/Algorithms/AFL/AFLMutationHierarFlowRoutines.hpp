#pragma once

#include <memory>
#include "ExecInput/ExecInput.hpp"
#include "Algorithms/AFL/AFLState.hpp"
#include "Algorithms/AFL/AFLMutator.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

namespace afl {
namespace pipeline {
namespace mutation {

using AFLMutInputType = bool(AFLMutator&);
using AFLMutCalleeRef = NullableRef<HierarFlowCallee<AFLMutInputType>>;
using AFLMutOutputType = bool(const u8*, u32);

struct BitFlip1WithAutoDictBuild
    : public HierarFlowRoutine<
        AFLMutInputType,
        AFLMutOutputType
    > {
public:
    BitFlip1WithAutoDictBuild(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

struct BitFlipOther
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    BitFlipOther(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

struct ByteFlip1WithEffMapBuild
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    ByteFlip1WithEffMapBuild(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

struct ByteFlipOther
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    ByteFlipOther(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

struct Arith
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    Arith(AFLState &state);

    // Because, in Arith, we need to access the buffer as u8*, u16*, u32*,
    // we create a template function which describes each step of the stage
    template<class UInt>
    bool DoArith(AFLMutator &mutator);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

template<class UInt>
bool Arith::DoArith(AFLMutator &mutator) {
    using afl::util::EFF_APOS;
    constexpr auto byte_width = sizeof(UInt);   

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;
    u32 num_mutable_pos = mutator.GetLen() + 1 - byte_width;

    state.stage_short = "arith" + std::to_string(byte_width * 8);
    state.stage_name = "arith " + std::to_string(byte_width * 8) + "/8";
    state.stage_cur = 0;

    // the number of mutations conducted for each byte
    int times_mut_per_unit = 2; // AddN, SubN
    if (byte_width >= 2) times_mut_per_unit *= 2; // Big Endian, Little Endian

    state.stage_max = times_mut_per_unit * num_mutable_pos * AFLOption::ARITH_MAX;
    
    for (u32 i=0; i < num_mutable_pos; i++) {
        if (!state.eff_map[EFF_APOS(i)] && !state.eff_map[EFF_APOS(i + byte_width - 1)]) {
            state.stage_max -= times_mut_per_unit * AFLOption::ARITH_MAX;
            continue;
        }

        state.stage_cur_byte = i;
        for (s32 j=1; j <= (s32)AFLOption::ARITH_MAX; j++) { 
            // deal with Little Endian
            state.stage_val_type = AFLOption::STAGE_VAL_LE;

            if (mutator.AddN<UInt>(i, j, false)) {
                state.stage_cur_val = j;
                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;
                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;

            if (mutator.SubN<UInt>(i, j, false)) {
                state.stage_cur_val = -j;
                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;
                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;
            
            if (byte_width == 1) continue;

            // Big Endian
            state.stage_val_type = AFLOption::STAGE_VAL_BE;

            if (mutator.AddN<UInt>(i, j, true)) {
                state.stage_cur_val = j;
                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;
                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;

            if (mutator.SubN<UInt>(i, j, true)) {
                state.stage_cur_val = -j;
                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;
                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;

        }
    }

    int stage_idx;
    if (byte_width == 1) stage_idx = AFLOption::STAGE_ARITH8;
    else if (byte_width == 2) stage_idx = AFLOption::STAGE_ARITH16;
    else if (byte_width == 4) stage_idx = AFLOption::STAGE_ARITH32;

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
    state.stage_finds[stage_idx] += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[stage_idx] += state.stage_max;

    return false;
}

struct Interest
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    Interest(AFLState &state);

    // Because, in Interest, we need to access the buffer as u8*, u16*, u32*,
    // we create a template function which describes each step of the stage
    template<class UInt>
    bool DoInterest(AFLMutator &mutator);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

template<class UInt>
bool Interest::DoInterest(AFLMutator &mutator) {
    using afl::util::EFF_APOS;
    constexpr auto byte_width = sizeof(UInt);

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;
    u32 num_mutable_pos = mutator.GetLen() + 1 - byte_width;

    using SInt = typename std::make_signed<UInt>::type;

    // We can't assign values to reference types like below
    // So we have to use a pointer...
    const std::vector<SInt>* interest_values;
    if constexpr (byte_width == 1) interest_values = &Mutator::interesting_8;
    else if constexpr (byte_width == 2) interest_values = &Mutator::interesting_16;
    else if constexpr (byte_width == 4) interest_values = &Mutator::interesting_32;

    int num_endians;
    if constexpr (byte_width == 1) num_endians = 1;
    else num_endians = 2;

    state.stage_short = "int" + std::to_string(byte_width * 8);
    state.stage_name = "interest " + std::to_string(byte_width * 8) + "/8";
    state.stage_cur = 0;
    state.stage_max = num_endians * num_mutable_pos * interest_values->size();

    for (u32 i=0; i < num_mutable_pos; i++) {
        /* Let's consult the effector map... */

        if (!state.eff_map[EFF_APOS(i)] && !state.eff_map[EFF_APOS(i + byte_width - 1)]) {
            state.stage_max -= num_endians * interest_values->size();
            continue;
        }

        state.stage_cur_byte = i;

        for (u32 j=0; j < interest_values->size(); j++) {
            state.stage_cur_val = (*interest_values)[j];

            if (mutator.InterestN<UInt>(i, j, false)) {
                state.stage_val_type = AFLOption::STAGE_VAL_LE;

                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;

                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;

            if constexpr (byte_width == 1) continue;

            if (mutator.InterestN<UInt>(i, j, true)) {
                state.stage_val_type = AFLOption::STAGE_VAL_BE;

                if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) return true;

                state.stage_cur++;
                mutator.RestoreOverwrite<UInt>();
            } else state.stage_max--;
        }
    }

    int stage_idx;
    if constexpr (byte_width == 1) stage_idx = AFLOption::STAGE_INTEREST8;
    else if constexpr (byte_width == 2) stage_idx = AFLOption::STAGE_INTEREST16;
    else if constexpr (byte_width == 4) stage_idx = AFLOption::STAGE_INTEREST32;

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;
    state.stage_finds[stage_idx] += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[stage_idx] += state.stage_max;

    return false;
}

// the stage of overwriting with dictionary
// there are two options:
//   a) use user-defined dictionary b) use automatically created dictionary
// they are different stages but the processes are almost the same
// so we put them into one template function
template<bool is_auto>
struct DictOverwrite
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    DictOverwrite(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};

template<bool is_auto>
DictOverwrite<is_auto>::DictOverwrite(AFLState &state)
    : state(state) {}

template<bool is_auto>
AFLMutCalleeRef DictOverwrite<is_auto>::operator()(
    AFLMutator& mutator
) {
    // skip this step if the dictionary is empty
    if constexpr(is_auto) {
        if (state.a_extras.empty()) return GoToDefaultNext();
    } else {
        if (state.extras.empty())   return GoToDefaultNext();
    }

    /* Overwrite with user-supplied extras. */

    using afl::util::EFF_APOS;
    using afl::util::EFF_SPAN_ALEN;

    u32 num_used_extra;
    if constexpr (is_auto) {
        state.stage_name = "auto extras (over)";
        state.stage_short = "ext_AO";
        num_used_extra = std::min<u32>(state.a_extras.size(), AFLOption::USE_AUTO_EXTRAS);
    } else {
        state.stage_name = "user extras (over)";
        state.stage_short = "ext_UO";
        num_used_extra = state.extras.size();
    }
    state.stage_cur = 0;
    state.stage_max = mutator.GetLen() * num_used_extra;

    state.stage_val_type = AFLOption::STAGE_VAL_NONE;

    u64 orig_hit_cnt = state.queued_paths + state.unique_crashes;

    auto& extras = is_auto ? state.a_extras : state.extras;

    for (u32 i=0; i<mutator.GetLen(); i++) {
        state.stage_cur_byte = i;

        /* Extras are sorted by size, from smallest to largest. This means
           that we don't have to worry about restoring the buffer in
           between writes at a particular offset determined by the outer
           loop. */

        u32 last_len = 0;
        for (auto itr=extras.begin(); itr != extras.begin() + num_used_extra; itr++) {
            auto& extra = *itr;

            /* Skip extras probabilistically if extras_cnt > MAX_DET_EXTRAS. Also
               skip them if there's no room to insert the payload, if the token
               is redundant, or if its entire span has no bytes set in the
               effector map. */

            using afl::util::UR;
            if (!is_auto) {
                if ( extras.size() > AFLOption::MAX_DET_EXTRAS
                  && UR(state.extras.size(), state.rand_fd) >= AFLOption::MAX_DET_EXTRAS) {
                    state.stage_max--;
                    continue;
                }
            }

            if ( extra.data.size() > mutator.GetLen() - i
              || !std::memcmp(&extra.data[0], mutator.GetBuf() + i, extra.data.size())
              || !std::memchr(&state.eff_map[EFF_APOS(i)], 1, EFF_SPAN_ALEN<u32>(i, extra.data.size()))
            ) {
                state.stage_max--;
                continue;
            }

            last_len = extra.data.size();
            mutator.Replace(i, &extra.data[0], last_len);

            if (CallSuccessors(mutator.GetBuf(), mutator.GetLen())) {
                SetResponseValue(true);
                return GoToParent();
            }

            state.stage_cur++;
        }

        mutator.Replace(i, mutator.GetSource().GetBuf() + i, last_len);
    }

    u64 new_hit_cnt = state.queued_paths + state.unique_crashes;

    int stage_idx;
    if (is_auto) {
        stage_idx = AFLOption::STAGE_EXTRAS_AO;
    } else {
        stage_idx = AFLOption::STAGE_EXTRAS_UO;
    }

    state.stage_finds[stage_idx]  += new_hit_cnt - orig_hit_cnt;
    state.stage_cycles[stage_idx] += state.stage_max;

    return GoToDefaultNext();
}

using UserDictOverwrite = DictOverwrite<false>;
using AutoDictOverwrite = DictOverwrite<true>;

struct UserDictInsert
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {
public:
    UserDictInsert(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);

private:
    AFLState &state;
};


struct HavocBase
    : public HierarFlowRoutine<
          AFLMutInputType,
          AFLMutOutputType
      > {

public:
    HavocBase(AFLState &state);
    virtual ~HavocBase() {}

    bool DoHavoc(
        AFLMutator& mutator,
        const std::string &stage_name,
        const std::string &stage_short,
        u32 perf_score,
        s32 stage_max_multiplier, 
        int stage_idx
    );

    virtual AFLMutCalleeRef operator()(AFLMutator& mutator) = 0;

protected:
    AFLState &state;
};

struct Havoc : public HavocBase {
public:
    Havoc(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);
};

struct Splicing : public HavocBase {
public:
    Splicing(AFLState &state);

    AFLMutCalleeRef operator()(AFLMutator& mutator);
};

} // namespace mutation
} // namespace pipeline
} // namespace afl
