#pragma once

#include <memory>
#include <functional>
#include "Utils/Common.hpp"

// Executorが今回のPUT実行のために生成したfdをfeedbackとして返す場合に使うクラス
// つまり、fdは再利用されることがなく、feedbackの受け取り手が好きに使っていい（closeしてもよい）場合を想定
class DisposableFdFeedback {
public:
    DisposableFdFeedback();

    DisposableFdFeedback(const DisposableFdFeedback&);
    DisposableFdFeedback& operator=(const DisposableFdFeedback&);

    DisposableFdFeedback(int fd);

    void Read(void *buf, u32 len);
    u32 ReadTimed(void *buf, u32 len, u32 timeout_ms);
    void Write(void *buf, u32 len);

    // TODO: いくら捨てて良いfdでも好き勝手にコピーされcloseされないのはまずいかも？
    // BorrowedFdFeedbackと同様にコピー不可にしつつ、destructorでclose(fd)してもいいかも
    int fd;
};
