#pragma once

#include <vector>
#include <string>
#include "Utils/Filesystem.hpp"
#include "Utils/Common.hpp"

struct PythonSetting {
    explicit PythonSetting(
        const std::vector<std::string> &argv,
        const std::string &in_dir,
        const std::string &out_dir,
        u32 exec_timelimit_ms,
        u64 exec_memlimit,
        bool forksrv,
        bool need_afl_cov,
        bool need_bb_cov
    );

    ~PythonSetting();

    const std::vector<std::string> argv;
    const fs::path in_dir;
    const fs::path out_dir;
    const u32 exec_timelimit_ms;
    const u64 exec_memlimit; 
    const bool forksrv;
    const bool need_afl_cov;
    const bool need_bb_cov;
};
