#include "Feedback/ExitStatusFeedback.hpp"

#include <cstddef>
#include <unordered_map>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

ExitStatusFeedback::ExitStatusFeedback() {}

ExitStatusFeedback::ExitStatusFeedback(
    PUTExitReasonType exit_reason,
    u8 signal
) : exit_reason( exit_reason ),
    signal( signal ) {}

ExitStatusFeedback::ExitStatusFeedback(const ExitStatusFeedback& orig) 
    : exit_reason( orig.exit_reason ),
      signal( orig.signal ) {}

ExitStatusFeedback& ExitStatusFeedback::operator=(const ExitStatusFeedback& orig) {
    exit_reason = orig.exit_reason;
    signal = orig.signal;

    return *this;
}
