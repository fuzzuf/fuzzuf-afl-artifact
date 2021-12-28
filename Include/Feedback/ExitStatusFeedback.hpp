#pragma once

#include <memory>
#include <unordered_map>
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

class ExitStatusFeedback {
public:
    ExitStatusFeedback();
    
    ExitStatusFeedback(const ExitStatusFeedback&);
    ExitStatusFeedback& operator=(const ExitStatusFeedback&);

    explicit ExitStatusFeedback(PUTExitReasonType exit_reason, u8 signal);

    PUTExitReasonType exit_reason;
    u8 signal;
};
