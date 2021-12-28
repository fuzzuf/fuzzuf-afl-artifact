#pragma once

#include <memory>
#include <unordered_map>
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

// InplaceMemoryFeedbackはライフタイムがExecutorに縛られているため、それを回避したい場合こちらを使う
// InplaceMemoryFeedback::ConvertToPersistentで変換可能
class PersistentMemoryFeedback {
public:
    PersistentMemoryFeedback();

    // prohibit copies
    PersistentMemoryFeedback(const PersistentMemoryFeedback&) = delete;
    PersistentMemoryFeedback& operator=(const PersistentMemoryFeedback&) = delete;

    // allow moves
    PersistentMemoryFeedback(PersistentMemoryFeedback&&);
    PersistentMemoryFeedback& operator=(PersistentMemoryFeedback&&);

    explicit PersistentMemoryFeedback(const u8* mem, u32 len);

    u32 CalcCksum32() const;
    u32 CountNonZeroBytes() const;

    // 主にPythonFuzzer向けのメソッド。cppのTODO参照
    std::unordered_map<int, u8> GetTrace(void);

    std::unique_ptr<u8[]> mem;
    u32 len;

    // 主にPythonFuzzer向けの要素。cppのTODO参照
    std::unordered_map<int, u8> trace;
};
