#pragma once

#include <vector>
#include <string>
#include <optional>
#include "Utils/Filesystem.hpp"
#include "Utils/Common.hpp"

struct AFLSetting {
    explicit AFLSetting(
        const std::vector<std::string> &argv,
        const std::string &in_dir,
        const std::string &out_dir,
        u32 exec_timelimit_ms,
        u64 exec_memlimit,
        bool forksrv,
        bool dumb_mode,
        int cpuid_to_bind
    );

    ~AFLSetting();

    const std::vector<std::string> argv;
    const fs::path in_dir;
    const fs::path out_dir;
    const u32 exec_timelimit_ms;
    const u64 exec_memlimit; 
    const bool forksrv;
    const bool dumb_mode;
    const bool simple_files = false;
    const bool ignore_finds = false;
    const int cpuid_to_bind;
};
