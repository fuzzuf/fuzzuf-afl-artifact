#pragma once

#include <memory>
#include "Algorithms/AFL/AFLState.hpp"
#include "Algorithms/AFL/AFLTestcase.hpp"
#include "Algorithms/AFL/AFLMutator.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"

#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

namespace afl {
namespace pipeline {
namespace other {

// the head node(the fuzzing loop starts from seed selection in AFL)

struct SelectSeed 
    : public HierarFlowRoutine<
        void(void),
        bool(std::shared_ptr<AFLTestcase>)
      > {
public:
    SelectSeed(AFLState &state);

    NullableRef<HierarFlowCallee<void(void)>> operator()(void);

private:
    AFLState &state;
    u64 prev_queued = 0;
};

// middle nodes(steps done before and after actual mutations)

using AFLMidInputType = bool(std::shared_ptr<AFLTestcase>);
using AFLMidCalleeRef = NullableRef<HierarFlowCallee<AFLMidInputType>>;
using AFLMidOutputType = bool(AFLMutator&);

struct ConsiderSkipMut
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    ConsiderSkipMut(AFLState &state);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
};

struct RetryCalibrate
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    RetryCalibrate(AFLState &state, AFLMidCalleeRef abandon_entry);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
    AFLMidCalleeRef abandon_entry;
};

struct TrimCase
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    TrimCase(AFLState &state, AFLMidCalleeRef abandon_entry);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
    AFLMidCalleeRef abandon_entry;
};

struct CalcScore
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    CalcScore(AFLState &state);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
};

struct ApplyDetMuts
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    ApplyDetMuts(AFLState &state, AFLMidCalleeRef abandon_entry);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
    AFLMidCalleeRef abandon_entry;
};

struct ApplyRandMuts
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    ApplyRandMuts(AFLState &state, AFLMidCalleeRef abandon_entry);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
    AFLMidCalleeRef abandon_entry;
};

struct AbandonEntry
    : public HierarFlowRoutine<
        AFLMidInputType,
        AFLMidOutputType
    > {
public:
    AbandonEntry(AFLState &state);

    AFLMidCalleeRef operator()(
        std::shared_ptr<AFLTestcase>
    );

private:
    AFLState &state;
};

struct ExecutePUT
    : public HierarFlowRoutine<
        bool(const u8*, u32),
        bool(const u8*, u32, InplaceMemoryFeedback&, ExitStatusFeedback&)
    > {
public:
    ExecutePUT(AFLState &state);

    NullableRef<HierarFlowCallee<bool(const u8*, u32)>> operator()(
        const u8*, 
        u32
    );

private:
    AFLState &state;
};

} // namespace other
} // namespace pipeline
} // namespace afl
