#include "Python/PythonTestcase.hpp"
#include "Feedback/PersistentMemoryFeedback.hpp"

PythonTestcase::PythonTestcase(
    std::shared_ptr<OnMemoryExecInput> input,
    PersistentMemoryFeedback&& afl_feed,
    PersistentMemoryFeedback&&  bb_feed
) : input(input),
    afl_feed(std::move(afl_feed)),
    bb_feed(std::move(bb_feed)) {}

PythonTestcase::~PythonTestcase() {}
