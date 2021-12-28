#pragma once

#include <cstddef>
#include <cassert>
#include <memory>
#include <vector>
#include <string>
#include "Utils/Filesystem.hpp"
#include "Exceptions.hpp"
#include "Executor/Executor.hpp"
#include "Utils/Common.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"

namespace fuzzuf::executor {

struct child_state_t {
  int exec_result;
  int exec_errno;
};

}

// ネイティブのLinux環境（ファザーのトレーサーとファズ対象が同じLinux環境）でのファズ実行を実現するクラス
//
// 責務：
//  - クラスメンバー Executor::argv はファジング対象のプロセスを実行に必要な情報（e.g. コマンド、コマンド引数）を具備すること
//
// 将来的に入れたい責務：
//  - 自クラスの生存期間とメンバー変数の値が有効である期間が一致していること【現状確認が間に合わないのでTODOとする】
//      - これは堅牢性の担保のため
class NativeLinuxExecutor : public Executor {
public:
    static constexpr int INVALID_SHMID = -1; 

    // Constant values used as "cpuid_to_bind", one of the arguments of the constructor
    static constexpr int CPUID_DO_NOT_BIND    = -2;
    static constexpr int CPUID_BIND_WHICHEVER = -1;

    // コンストラクタ越しに渡される設定を保持するメンバたち
    const bool forksrv;

    const bool need_afl_cov;
    const bool need_bb_cov;

    // NativeLinuxExecutorは高速化のためにこのプロセス（やPUTのプロセス）が実行されるCPUのコアを指定しても良い。
    // 指定されている場合は、0-originでそのidが入る。そうでない場合は、std::nullopt
    std::optional<int> binded_cpuid; 

    const bool uses_asan = false; // 将来的にはオプションになり得るが、現状これを有効にすることは想定されていない

    const int cpu_core_count;

    // NativeLinuxExecutor::INVALID_SHMIDが入っている場合は有効なIDを保持していないことを意味する
    int bb_shmid;  
    int afl_shmid; 

    int forksrv_pid;
    int forksrv_read_fd;
    int forksrv_write_fd;

    u8 *bb_trace_bits;
    u8 *afl_trace_bits;

    bool child_timed_out;

    static bool has_setup_sighandlers;
    // シグナルハンドラが利用する、現在生きているexecutorインスタンスへのポインタ。
    // そのようなものがなければnullptrになる。
    // 「同時に複数のfuzzerインスタンスを利用することがない」を暫定的に前提としていることに注意。
    static NativeLinuxExecutor *active_instance;

    NativeLinuxExecutor(  
        const std::vector<std::string> &argv,
        u32 exec_timelimit_ms,
        u64 exec_memlimit,
        bool forksrv,
        const fs::path &path_to_write_input,
        bool need_afl_cov,
        bool need_bb_cov,
        int cpuid_to_bind
    );
    ~NativeLinuxExecutor();

    NativeLinuxExecutor( const NativeLinuxExecutor& ) = delete;
    NativeLinuxExecutor( NativeLinuxExecutor&& ) = delete;
    NativeLinuxExecutor &operator=( const NativeLinuxExecutor& ) = delete;
    NativeLinuxExecutor &operator=( NativeLinuxExecutor&& ) = delete;
    NativeLinuxExecutor() = delete;

    // Common methods among children on Executor classes
    // 基底クラスで宣言をして、各クラスで定義するだけにしたいが、書き方がよくわからずorz
    void Run(const u8 *buf, u32 len, u32 timeout_ms=0);
    void ReceiveStopSignal(void);

    // Environment-epecific methods
    InplaceMemoryFeedback GetAFLFeedback();
    InplaceMemoryFeedback GetBBFeedback();
    ExitStatusFeedback GetExitStatusFeedback();

    void TerminateForkServer();
    void SetCArgvAndDecideInputMode();    
    void SetupSharedMemories();
    void ResetSharedMemories();
    void EraseSharedMemories();
    void SetupEnvironmentVariablesForTarget();
    void SetupForkServer();    

    static void SetupSignalHandlers();
    static void AlarmHandler(int signum);

private:    
    PUTExitReasonType last_exit_reason;
    u8 last_signal;    
};
