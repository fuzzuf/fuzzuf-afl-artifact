#include "Feedback/DisposableFdFeedback.hpp"

#include <cstddef>
#include <functional>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

DisposableFdFeedback::DisposableFdFeedback() : fd(-1) {}

DisposableFdFeedback::DisposableFdFeedback(int fd) : fd( fd ) {}

DisposableFdFeedback::DisposableFdFeedback(const DisposableFdFeedback& orig)
    : fd( orig.fd ) {}

DisposableFdFeedback& DisposableFdFeedback::operator=(
    const DisposableFdFeedback& orig
) {
    fd = orig.fd;

    return *this;
}

void DisposableFdFeedback::Read(void *buf, u32 len) {
    Util::ReadFile(fd, buf, len);
}

u32 DisposableFdFeedback::ReadTimed(void *buf, u32 len, u32 timeout_ms) {
    return Util::ReadFileTimed(fd, buf, len, timeout_ms);
}

void DisposableFdFeedback::Write(void *buf, u32 len) {
    Util::WriteFile(fd, buf, len);
}
