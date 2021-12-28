#include <vector>
#include <string>
#include <optional>
#include "Algorithms/AFL/AFLSetting.hpp"

AFLSetting::AFLSetting(
    const std::vector<std::string> &argv,
    const std::string &in_dir,
    const std::string &out_dir,
    u32 exec_timelimit_ms,
    u64 exec_memlimit,
    bool forksrv,
    bool dumb_mode,
    int cpuid_to_bind
) : 
    argv( argv ),
    in_dir( in_dir ),
    out_dir( out_dir ),
    exec_timelimit_ms( exec_timelimit_ms ),
    exec_memlimit( exec_memlimit ),
    forksrv( forksrv ),
    dumb_mode( dumb_mode ),
    cpuid_to_bind( cpuid_to_bind ) {}

AFLSetting::~AFLSetting() {}
