#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include "Utils/Common.hpp"
#include "Python/PyFeedback.hpp"

class PySeed {
public:
    PySeed(
      const u64 id,
      const std::vector<u8> buf,
      const std::optional<std::unordered_map<int, u8>> bb_trace,
      const std::optional<std::unordered_map<int, u8>> afl_trace
    );
    PySeed( const PySeed& ) = delete;
    PySeed( PySeed&& ) = default;
    PySeed &operator=( const PySeed& ) = delete;
    PySeed &operator=( PySeed&& ) = default;

    std::vector<u8> GetBuf(void) const;
    PyFeedback GetFeedback(void) const;
    u64 GetID(void) const;

private:
    u64 id;
    std::vector<u8> buf;
    PyFeedback pyfeedback;
};
