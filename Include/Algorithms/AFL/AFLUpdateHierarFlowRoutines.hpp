#pragma once

#include <string>
#include <memory>
#include "Utils/Common.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"
#include "Algorithms/AFL/AFLState.hpp"

namespace afl {
namespace pipeline {
namespace update {

using AFLUpdInputType = bool(const u8*, u32, InplaceMemoryFeedback&, ExitStatusFeedback&);
using AFLUpdCalleeRef = NullableRef<HierarFlowCallee<AFLUpdInputType>>;
using AFLUpdOutputType = void(void);

struct NormalUpdate
    : public HierarFlowRoutine<
        AFLUpdInputType,
        AFLUpdOutputType
    > {
public:
    NormalUpdate(AFLState &state);

    AFLUpdCalleeRef operator()(
        const u8*, u32, InplaceMemoryFeedback&, ExitStatusFeedback&);

private:
    AFLState &state;
};

struct ConstructAutoDict
    : public HierarFlowRoutine<
        AFLUpdInputType,
        AFLUpdOutputType
    > {
public:
    ConstructAutoDict(AFLState &state);

    AFLUpdCalleeRef operator()(
        const u8*, u32, InplaceMemoryFeedback&, ExitStatusFeedback&);

private:
    AFLState &state;
};

struct ConstructEffMap
    : public HierarFlowRoutine<
        AFLUpdInputType,
        AFLUpdOutputType
    > {
public:
    ConstructEffMap(AFLState &state);

    AFLUpdCalleeRef operator()(
        const u8*, u32, InplaceMemoryFeedback&, ExitStatusFeedback&);

private:
    AFLState &state;
};

} // namespace update
} // namespace pipeline
} // namespace afl
