#include "Feedback/FileFeedback.hpp"

#include <cstddef>
#include <functional>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Utils/Filesystem.hpp"
#include "Feedback/PUTExitReasonType.hpp"

FileFeedback::FileFeedback() {}

FileFeedback::FileFeedback(
    fs::path feed_path,
    std::shared_ptr<u8> executor_lock
) : feed_path( feed_path ),
    executor_lock( executor_lock ) {}

FileFeedback::FileFeedback(FileFeedback&& orig)
    : feed_path( orig.feed_path ),
      executor_lock( std::move(orig.executor_lock) ) {}

FileFeedback& FileFeedback::operator=(FileFeedback&& orig) {
    std::swap(feed_path, orig.feed_path);
    std::swap(executor_lock, orig.executor_lock);

    return *this;
}

// This is static method
// the argument name is commented out to suppress unused-value-warning
void FileFeedback::DiscardActive(FileFeedback /* unused_and_discarded_arg */) {
    // Do nothing.
    // At the end of this function, the argument unused_and_discarded_arg will be destructed
    // This is what this function means
}
