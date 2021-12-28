#include "Feedback/BorrowedFdFeedback.hpp"

#include <cstddef>
#include <functional>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

BorrowedFdFeedback::BorrowedFdFeedback() : fd(-1) {}

BorrowedFdFeedback::BorrowedFdFeedback(
    int fd, 
    std::shared_ptr<u8> executor_lock
) : fd( fd ),
    executor_lock( executor_lock ) {}

BorrowedFdFeedback::BorrowedFdFeedback(BorrowedFdFeedback&& orig)
    : fd( orig.fd ),
      executor_lock( std::move(orig.executor_lock) ) {}

BorrowedFdFeedback& BorrowedFdFeedback::operator=(BorrowedFdFeedback&& orig) {
    std::swap(fd, orig.fd);
    std::swap(executor_lock, orig.executor_lock);

    return *this;
}

void BorrowedFdFeedback::Read(void *buf, u32 len) {
    Util::ReadFile(fd, buf, len);
}

u32 BorrowedFdFeedback::ReadTimed(void *buf, u32 len, u32 timeout_ms) {
    return Util::ReadFileTimed(fd, buf, len, timeout_ms);
}

void BorrowedFdFeedback::Write(void *buf, u32 len) {
    Util::WriteFile(fd, buf, len);
}

// This is static method
// the argument name is commented out to suppress unused-value-warning
void BorrowedFdFeedback::DiscardActive(BorrowedFdFeedback /* unused_and_discarded_arg */) {
    // Do nothing.
    // At the end of this function, the argument unused_and_discarded_arg will be destructed
    // This is what this function means
}
