#pragma once

#include <memory>
#include <functional>
#include "Utils/Common.hpp"
#include "Utils/Filesystem.hpp"

// Executorがファイル内にfeedbackを書き込んで返す場合に使うクラス
// このクラスのインスタンスが生きている間、インスタンスが参照しているファイルを管理しているExecutorは新しいPUTの実行ができない
// インスタンスを破棄したい場合は、FileFeedback::DiscardActiveを使うこと
class FileFeedback {
public:
    // prohibit copy constructors,
    // because copying this class can easily and accidentally make executor locked
    // the "active" instance should be at most only one always
    FileFeedback(const FileFeedback&) = delete;
    FileFeedback& operator=(const FileFeedback&) = delete;

    // we have no choice but to define the constructor without arguments
    // because sometimes we need to retrieve new instances via arguments like 'FileFeedback &new_feed'
    FileFeedback();

    FileFeedback(FileFeedback &&);
    FileFeedback& operator=(FileFeedback &&);

    // TODO: should we pass "raw file" which is already opened 
    // instead of passing a path like this?
    FileFeedback(fs::path feed_path, std::shared_ptr<u8> executor_lock);

    // If you want to discard the active instance to start a new execution, 
    // then use this like FileFeedback::DiscardActive(std::move(feed))
    static void DiscardActive(FileFeedback /* unused_arg */);

    fs::path feed_path;

private:
    std::shared_ptr<u8> executor_lock;
};
