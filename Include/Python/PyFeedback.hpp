#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include "Utils/Common.hpp"

class PyFeedback{
public:
    PyFeedback(
      const std::optional<std::unordered_map<int, u8>> bb_trace,
      const std::optional<std::unordered_map<int, u8>> afl_trace
    );
    PyFeedback( const PyFeedback& ) = default;
    PyFeedback( PyFeedback&& ) = default;
    PyFeedback &operator=( const PyFeedback& ) = default;
    PyFeedback &operator=( PyFeedback&& ) = default;

    std::optional<std::unordered_map<int, u8>> GetBBTrace(void);
    std::optional<std::unordered_map<int, u8>> GetAFLTrace(void);

private:
    std::optional<std::unordered_map<int, u8>> bb_trace;
    std::optional<std::unordered_map<int, u8>> afl_trace;
};

