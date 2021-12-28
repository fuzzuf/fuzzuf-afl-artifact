#pragma once

#include <memory>

#include "ExecInput/OnMemoryExecInput.hpp"
#include "Feedback/PersistentMemoryFeedback.hpp"

struct PythonTestcase {
    explicit PythonTestcase(
        std::shared_ptr<OnMemoryExecInput> input,
        PersistentMemoryFeedback &&afl_feed,
        PersistentMemoryFeedback &&bb_feed
    );
    ~PythonTestcase();

    std::shared_ptr<OnMemoryExecInput> input;
    PersistentMemoryFeedback afl_feed;
    PersistentMemoryFeedback  bb_feed;
};
