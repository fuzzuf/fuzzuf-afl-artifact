#include <vector>
#include <string>
#include "Python/PythonSetting.hpp"

PythonSetting::PythonSetting(
    const std::vector<std::string> &argv,
    const std::string &in_dir,
    const std::string &out_dir,
    u32 exec_timelimit_ms,
    u64 exec_memlimit,
    bool forksrv,
    bool need_afl_cov,
    bool need_bb_cov
) : 
    argv( argv ),
    in_dir( in_dir ),
    out_dir( out_dir ),
    exec_timelimit_ms( exec_timelimit_ms ),
    exec_memlimit( exec_memlimit ),
    forksrv( forksrv ),
    need_afl_cov( need_afl_cov ),
    need_bb_cov( need_bb_cov ) {}

PythonSetting::~PythonSetting() {}
