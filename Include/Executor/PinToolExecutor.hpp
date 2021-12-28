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
#include "Feedback/FileFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"

// Intel Pinでのファズ実行を実現するクラス（ファザーのトレーサーとファズ対象は同じ環境である必要がある）
// 例えばWindowsのIntel PinトレーサーはWindowsバイナリしかファズ実行できない。
//
// 責務：
//  - クラスメンバー Executor::argv はファジング対象のプロセスの実行に必要な情報（e.g. コマンド、コマンド引数）を具備すること
//  - クラスメンバー Executor::pargv はpintoolの実行に必要な情報（e.g. コマンド、コマンド引数）を具備すること
//
// 将来的に入れたい責務：
//  - 自クラスの生存期間とメンバー変数の値が有効である期間が一致していること【現状確認が間に合わないのでTODOとする】
//      - これは堅牢性の担保のため
class PinToolExecutor : public Executor {
public:

    // コンストラクタ越しに渡される設定を保持するメンバたち
    const std::string path_str_to_pin_exec; // Intel Pinバイナリのパス    
    const std::vector<std::string> pargv; // Intel Pin プラグインに指定するコマンドライン引数

    bool child_timed_out;

    // シグナルハンドラが利用する、現在生きているexecutorインスタンスへのポインタ。
    // そのようなものがなければnullptrになる。
    // 「同時に複数のfuzzerインスタンスを利用することがない」を暫定的に前提としていることに注意。
    static PinToolExecutor *active_instance;

    static bool has_setup_sighandlers;

    PinToolExecutor(  
        const fs::path &path_to_pin_exec,
        const std::vector<std::string> &pargv,             
        const std::vector<std::string> &argv,
        u32 exec_timelimit_ms,
        u64 exec_memlimit,
        const fs::path &path_to_write_input
    );
    ~PinToolExecutor();

    PinToolExecutor( const PinToolExecutor& ) = delete;
    PinToolExecutor( PinToolExecutor&& ) = delete;
    PinToolExecutor &operator=( const PinToolExecutor& ) = delete;
    PinToolExecutor &operator=( PinToolExecutor&& ) = delete;
    PinToolExecutor() = delete;

    void SetPlugin(const fs::path &path_to_pin_plugin, const std::vector<std::string> &argv);

    // Common methods among children on Executor classes
    // 基底クラスで宣言をして、各クラスで定義するだけにしたいが、書き方がよくわからずorz
    void Run(const u8 *buf, u32 len, u32 timeout_ms=0);
    void ReceiveStopSignal(void);

    // Environment-epecific methods
    FileFeedback GetFileFeedback(fs::path feed_path);
    ExitStatusFeedback GetExitStatusFeedback();

    void SetCArgvAndDecideInputMode();

    static void SetupSignalHandlers();
    static void AlarmHandler(int signum);

private:    
    PUTExitReasonType last_exit_reason;
    u8 last_signal;      
};
