#pragma once

#include <vector>
#include <string>
#include "Utils/Common.hpp"
#include "Fuzzer/Fuzzer.hpp"

class AFLFuzzerStub : public Fuzzer {
public:
    // テスト用途であるため、メンバー変数を公開して楽をする
    const std::vector<std::string> argv;
    const std::string in_dir;
    const std::string out_dir;
    const u32 exec_timelimit_ms;
    const u32 exec_memlimit;
    const bool forksrv;

    // コストラクタの引数は AFLFuzzer クラスのそれと同一であること
    AFLFuzzerStub(
        const std::vector<std::string> argv,
        const std::string in_dir,
        const std::string out_dir,
        u32 exec_timelimit_ms,
        u32 exec_memlimit,
        bool forksrv
    ) : 
        argv(argv),
        in_dir(in_dir),
        out_dir(out_dir),
        exec_timelimit_ms(exec_timelimit_ms),
        exec_memlimit(exec_memlimit),
        forksrv(forksrv)
    {}

    void ReceiveStopSignal(void) {}
};