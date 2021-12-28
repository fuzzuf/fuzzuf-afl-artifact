#include "Python/PyFeedback.hpp"

PyFeedback::PyFeedback(
  const std::optional<std::unordered_map<int, u8>> bb_trace,
  const std::optional<std::unordered_map<int, u8>> afl_trace
) :
  bb_trace( bb_trace ),
  afl_trace( afl_trace ) {}

std::optional<std::unordered_map<int, u8>> PyFeedback::GetBBTrace(void) {
  return bb_trace;
}

std::optional<std::unordered_map<int, u8>> PyFeedback::GetAFLTrace(void) {
  return afl_trace;
}
