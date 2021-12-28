#include "Python/PySeed.hpp"

PySeed::PySeed(
   const u64 id,
   const std::vector<u8> buf,
   const std::optional<std::unordered_map<int, u8>> bb_trace,
   const std::optional<std::unordered_map<int, u8>> afl_trace
) :
  id ( id ),
  buf( buf ),
  pyfeedback(bb_trace, afl_trace) {}

std::vector<u8> PySeed::GetBuf(void) const {
  return buf;
}

PyFeedback PySeed::GetFeedback(void) const {
  return pyfeedback;
}

u64 PySeed::GetID(void) const {
  return id;
}
