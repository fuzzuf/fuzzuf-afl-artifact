#include "Feedback/InplaceMemoryFeedback.hpp"

#include <cstddef>
#include <functional>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

InplaceMemoryFeedback::InplaceMemoryFeedback()
    : mem(nullptr), len(0) {}

InplaceMemoryFeedback::InplaceMemoryFeedback(
    u8* _mem, 
    u32 _len, 
    std::shared_ptr<u8> _executor_lock
) : mem( _mem ),
    len( _len ),
    executor_lock( _executor_lock ) {}

InplaceMemoryFeedback::InplaceMemoryFeedback(InplaceMemoryFeedback&& orig)
    : mem( orig.mem ),
      len( orig.len ),
      executor_lock( std::move(orig.executor_lock) ) {

    orig.mem = nullptr;
}

InplaceMemoryFeedback& InplaceMemoryFeedback::operator=(InplaceMemoryFeedback&& orig) {
    mem = orig.mem;
    orig.mem = nullptr;

    len = orig.len;

    std::swap(executor_lock, orig.executor_lock);

    return *this;
}

// FIXME: check reference count in all the functions below in debug mode

PersistentMemoryFeedback InplaceMemoryFeedback::ConvertToPersistent() const {
    if (mem == nullptr) return PersistentMemoryFeedback();
    return PersistentMemoryFeedback(mem, len);
}

u32 InplaceMemoryFeedback::CalcCksum32() const {
    return Util::Hash32(mem, len, AFLOption::HASH_CONST);
}

u32 InplaceMemoryFeedback::CountNonZeroBytes() const {
    return Util::CountBytes(mem, len);
}

void InplaceMemoryFeedback::ShowMemoryToFunc(
    const std::function<void(const u8*, u32)>& func
) const {
    func(mem, len);
}

void InplaceMemoryFeedback::ModifyMemoryWithFunc(
    const std::function<void(u8*, u32)>& func
) {
    func(mem, len);
}

// This is static method
// the argument name is commented out to suppress unused-value-warning
void InplaceMemoryFeedback::DiscardActive(InplaceMemoryFeedback /* unused_and_discarded_arg */) {
    // Do nothing.
    // At the end of this function, the argument unused_and_discarded_arg will be destructed
    // This is what this function means
}
