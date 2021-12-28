#pragma once

#include <memory>
#include <functional>
#include "Utils/Common.hpp"
#include "Feedback/PersistentMemoryFeedback.hpp"

// Executorがinplaceに(共有)メモリ上のfeedbackを返す場合に使うfeedback
// このクラスのインスタンスが生きている間、インスタンスが参照しているメモリを持っているExecutorは新しいPUTの実行ができない
// インスタンスを破棄したい場合は、InplaceMemoryFeedback::DiscardActiveを使うこと
class InplaceMemoryFeedback {
public:
    // prohibit copy constructors.
    // because this class has the reference to raw shared memories, 
    // the "active" instance should be at most only one always
    InplaceMemoryFeedback(const InplaceMemoryFeedback&) = delete;
    InplaceMemoryFeedback& operator=(const InplaceMemoryFeedback&) = delete;

    // we have no choice but to define the constructor without arguments
    // because sometimes we need to retrieve new instances via arguments like 'InplaceMemoryFeedback &new_feed'
    InplaceMemoryFeedback();

    InplaceMemoryFeedback(InplaceMemoryFeedback &&);
    InplaceMemoryFeedback& operator=(InplaceMemoryFeedback &&);

    InplaceMemoryFeedback(u8* mem, u32 len, std::shared_ptr<u8> executor_lock);

    u32 CalcCksum32() const;
    u32 CountNonZeroBytes() const;

    void ShowMemoryToFunc(const std::function<void(const u8*, u32)>& func) const;

    void ModifyMemoryWithFunc(const std::function<void(u8*, u32)>& func);

    PersistentMemoryFeedback ConvertToPersistent() const;

    // If you want to discard the active instance to start a new execution, 
    // then use this like InplaceMemoryFeedback::DiscardActive(std::move(feed))
    static void DiscardActive(InplaceMemoryFeedback /* unused_arg */);

private:
    u8 *mem;
    u32 len;
    std::shared_ptr<u8> executor_lock;
};
