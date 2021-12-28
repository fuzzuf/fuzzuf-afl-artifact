#include "Feedback/PersistentMemoryFeedback.hpp"

#include <cstddef>
#include <unordered_map>
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

PersistentMemoryFeedback::PersistentMemoryFeedback()
    : mem(nullptr), len(0) {}

PersistentMemoryFeedback::PersistentMemoryFeedback(
    const u8* orig_mem,
    u32 len
) : mem( std::make_unique<u8[]>(len) ),
    len( len )
{
    std::copy(orig_mem, orig_mem+len, mem.get());
}

PersistentMemoryFeedback::PersistentMemoryFeedback(PersistentMemoryFeedback&& orig) 
    : mem( std::move(orig.mem) ),
      len( orig.len ),
      trace( std::move(orig.trace) ) {

    // now orig.trace are "valid but unspecified state" so we can call clear just in case
    orig.trace.clear();
    // for orig.mem, it is guaranteed it becomes nullptr
}

PersistentMemoryFeedback& PersistentMemoryFeedback::operator=(PersistentMemoryFeedback&& orig) {
    
    std::swap(mem, orig.mem);
    std::swap(trace, orig.trace);
    
    len = orig.len;
    orig.len = 0;

    orig.mem.reset();
    orig.trace.clear();

    return *this;
}

u32 PersistentMemoryFeedback::CalcCksum32() const {
    return Util::Hash32(mem.get(), len, AFLOption::HASH_CONST);
}

u32 PersistentMemoryFeedback::CountNonZeroBytes() const {
    return Util::CountBytes(mem.get(), len);
}

// PersistentMemoryFeedbackの保持しているmemを非零要素のみからなるmapに変換して返す
// memがnullptrのときは初期化直後のtrace（つまり空）を返すこととしているが、普通にエラー出しても良いかもしれない
// TODO: change the type of the return values from map to py::array_t
std::unordered_map<int, u8> PersistentMemoryFeedback::GetTrace(void) {
    if (!trace.empty() || mem == nullptr) return trace;

    for (u32 i=0; i<len; i++) {
        if (mem[i]) trace[i] = mem[i];
    }
    return trace;
}

