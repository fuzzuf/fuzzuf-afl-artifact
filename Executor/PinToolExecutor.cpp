#include <cstddef>
#include <cassert>
#include <memory>
#include "Options.hpp"
#include "Exceptions.hpp"
#include "Executor/Executor.hpp"
#include "Executor/PinToolExecutor.hpp"
#include "Utils/Workspace.hpp"
#include "Utils/Common.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"
#include "Feedback/PUTExitReasonType.hpp"
#include "Logger/Logger.hpp"

bool PinToolExecutor::has_setup_sighandlers = false;

PinToolExecutor* PinToolExecutor::active_instance = nullptr;

// 前提: 
//    - path_to_write_inputで指定されるパスにファイルが作成できる状態になっていること
PinToolExecutor::PinToolExecutor(  
    const fs::path &path_to_pin_exec,
    const std::vector<std::string> &pargv,             
    const std::vector<std::string> &argv,
    u32 exec_timelimit_ms,
    u64 exec_memlimit,
    const fs::path &path_to_write_input    
) :
    Executor( argv, exec_timelimit_ms, exec_memlimit, path_to_write_input.string() ),
    path_str_to_pin_exec( path_to_pin_exec.string() ),    
    pargv( pargv ),
    // cargv, stdin_modeはSetCArgvAndDecideInputModeで設定
    // path_str_to_pin_exec.c_str()をcargvが参照するが、
    // fs::path::c_strはlifetimeが不定な可能性があり避ける
    child_timed_out( false )
{
    if (!has_setup_sighandlers) {
        // 当座はグローバルにシグナルハンドラをセットするので、これはstaticなメソッド
        // なのでhas_setup_handlersがtrueなら二度とセットする必要がない
        SetupSignalHandlers();
        has_setup_sighandlers = true;
    }

    // シグナルハンドラが参照できるように、自分自身をstaticなポインタに入れておく
    // 以下のassert文を入れることができない
    // assert(active_instance == nullptr);
    active_instance = this;

    SetCArgvAndDecideInputMode();
    OpenExecutorDependantFiles();
}

PinToolExecutor::~PinToolExecutor() {
    if (input_fd != -1) {
        Util::CloseFile(input_fd);
        input_fd = -1;
    }
    if (null_fd != -1) {
        Util::CloseFile(null_fd);
        null_fd = -1;
    }    
}

void PinToolExecutor::SetCArgvAndDecideInputMode() {
    assert(!argv.empty()); // 流石におかしい
    
    stdin_mode = true; // if we find @@, then assign false to stdin_mode

    // Intel Pinに与えるオプションの追加 基本的には-tのみ
    cargv.emplace_back(path_str_to_pin_exec.c_str());
    cargv.emplace_back("-t");

    // Intel Pin プラグインに与えるオプションの追加
    for (const auto& v : pargv ) {
        cargv.emplace_back(v.c_str());
    }
    
    // PUTのコマンドラインオプションは"--"以降に追加
    cargv.emplace_back("--");

    for (const auto& v : argv ) {
        if ( v == "@@" ) {
            stdin_mode = false;
            cargv.emplace_back(path_str_to_write_input.c_str());
        } else {
            cargv.emplace_back(v.c_str());
        }
    }
    cargv.emplace_back(nullptr);
}

// staticなメソッド
// 前提:
//  - この関数が呼び出される時点で、active_instanceが必ず非nullptrになっている
//  - active_instanceはExecutorのインスタンスの有効なアドレスを保持すること
//  - 自プロセスにグローバルタイマーが存在し、SIGALRMシグナルにより本関数が呼ばれること
// 責務：
//  - SIGALRMに対するシグナルハンドラとして動作する
//      - 呼び出されたら、PUTが実行時間超過したものとしてactive_instance->child_pidをkillする
//      - フラグactive_instance->child_timed_outをセットする
void PinToolExecutor::AlarmHandler(int signum) {
    assert (signum == SIGALRM);
    assert (active_instance != nullptr);
    active_instance->KillChildWithoutWait();
    active_instance->child_timed_out = true;
}

// staticなメソッド
// 責務：
//  - シグナルに対してfuzzufのプロセスがどう対応するのかを規定する
//  - シグナルハンドラを必要とする場合はシグナルハンドラをセットする
// FIXME: シグナルに対する設定はプロセス全体で共通となるため、
// このプロセスで動作するすべてのfuzzerインスタンスが影響を受ける。
// したがって場合によってはプロセスの設計を変更する可能性があり、
// 現時点では単一のfuzzerインスタンスしか同時には利用しないという前提のもとで
// 暫定的な対処としてこのstaticメソッドを入れている。
void PinToolExecutor::SetupSignalHandlers() {
    struct sigaction sa;

    sa.sa_handler = NULL;
    sa.sa_flags = SA_RESTART;
    sa.sa_sigaction = NULL;

    sigemptyset(&sa.sa_mask);

// 現状fuzzufでは以下のシグナルをどう処理するか等の取り決めがないので一旦無視
#if 0
    /* Various ways of saying "stop". */

    sa.sa_handler = handle_stop_sig;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Window resize */

    sa.sa_handler = handle_resize;
    sigaction(SIGWINCH, &sa, NULL);

    /* SIGUSR1: skip entry */

    sa.sa_handler = handle_skipreq;
    sigaction(SIGUSR1, &sa, NULL);
#endif

    /* Things we don't care about. */

    sa.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = PinToolExecutor::AlarmHandler;
    sigaction(SIGALRM, &sa, NULL);
}

// 前提：
//  - input_fd は PinToolExecutor::SetupIO() で設定されたファイルを指すファイルディスクリプタであること
// 責務：
//  - (1) input_fd が指すファイルの中身がファズ（長さが len である buf の中身）と一致すること
//  - (2) 以下の要件を満たしたプロセスを実行すること（以降、これを満たすプロセスを「当該プロセス」と表記する）
//      - コンストラクタ引数argv で指定されたコマンドおよびコマンド引数を与えること
//      - TODO: 環境変数についての要件を明記（Instrument toolに関わる事項。今挙げているのは主要なもののみなので、今後機能追加していったりすると増える可能性あり）
//      - もし Executor::stdin_mode が true であるとき、input_fd を標準入力とすること
//  - (3) 当該プロセスは引数 timeout_ms で指定された時間で実行が中止されること。ただし、この値が0である場合には、メンバ変数 exec_timelimit_ms の値にしたがい制限時間を決める
//  - (4) 本メソッドを終了するとき、当該プロセスは自発的に、または第三者のシグナルを契機に終了していること
//  - (5) child_pid に当該プロセスのプロセスIDを代入すること
//      - この責務の必要性は、本メソッド以外の第三者が当該プロセスを終了できるようにするため。
//        今後は第三者が当該プロセスを終了することはあるか？
//        今後の拡張性のためこの責務を残しておく
void PinToolExecutor::Run(const u8 *buf, u32 len, u32 timeout_ms) {
    // locked until std::shared_ptr<u8> lock is used in other places
    while (lock.use_count() > 1) {
        usleep(100);
    }

    // if timeout_ms is 0, then we use exec_timelimit_ms;
    if (timeout_ms == 0) timeout_ms = exec_timelimit_ms;
    
    WriteTestInputToFile(buf, len);

    //#if 0
    DEBUG("Run: ");
    DEBUG("%s ", cargv[0]);
    std::for_each( cargv.begin(), cargv.end(), []( const char* v ) { DEBUG("%s ", v); } );
    DEBUG("\n")
    //#endif
    
    child_pid = Util::Fork();
    if (child_pid < 0) ERROR("fork() failed");
    if (!child_pid) {

        struct rlimit r;
        if (exec_memlimit) {
            r.rlim_max = r.rlim_cur = ((rlim_t)exec_memlimit) << 20;
        #ifdef RLIMIT_AS
            setrlimit(RLIMIT_AS, &r); /* Ignore errors */
        #else
            setrlimit(RLIMIT_DATA, &r); /* Ignore errors */
        #endif /* ^RLIMIT_AS */
        }

        r.rlim_max = r.rlim_cur = 0;

        setrlimit(RLIMIT_CORE, &r); /* Ignore errors */
            
        /* Isolate the process and configure standard descriptors. If out_file is
            specified, stdin is /dev/null; otherwise, out_fd is cloned instead. */
        setsid();

        // stdout, stderr は読み捨てる
        dup2(null_fd, 1);
        dup2(null_fd, 2);

        if (stdin_mode) {
            dup2(input_fd, 0);
        } else {
            // stdin は /dev/null を割り当てる（「無」の入力を与える）
            dup2(null_fd, 0);
        }
            
        execv(cargv[0], (char**)cargv.data());

        exit(0);
    }

    // PUTがhangしたかどうかのフラグを初期化
    // シグナルハンドラ内でセットされる
    int put_status; // PUT's status(retrieved via waitpid)
    child_timed_out = false;    //TODO: 

    static struct itimerval it;

    // タイマーをセットし、timeoutした場合にはSIGALRMが発生するようにする
    // SIGALRMのハンドラはPinToolExecutor::AlarmHandlerに設定されており、
    // 内部でPUTがキルされる
    // ただしexec_timelimit_msが0に設定されている場合はSIGALRMを設定しない
    if (exec_timelimit_ms) {
        it.it_value.tv_sec = (timeout_ms / 1000);
        it.it_value.tv_usec = (timeout_ms % 1000) * 1000;
        setitimer(ITIMER_REAL, &it, NULL);
    }

    if (waitpid(child_pid, &put_status, 0) <= 0) ERROR("waitpid() failed");

        // タイマーをリセット
    if (exec_timelimit_ms) {        
        it.it_value.tv_sec = 0;
        it.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &it, NULL);
    }

    if (!WIFSTOPPED(put_status)) child_pid = 0; 
    DEBUG("Exec Status %d (pid %d)\n", put_status, child_pid);

    /* Any subsequent operations on trace_bits must not be moved by the
        compiler below this point. Past this location, trace_bits[] behave
        very normally and do not have to be treated as volatile. */

    MEM_BARRIER();

    last_exit_reason = PUTExitReasonType::FAULT_NONE;
    last_signal = 0;

    /* Report outcome to caller. */
    if (WIFSIGNALED(put_status)) {
        last_signal = WTERMSIG(put_status);
        
        if (child_timed_out && last_signal == SIGKILL)
            last_exit_reason = PUTExitReasonType::FAULT_TMOUT;
        else 
            last_exit_reason = PUTExitReasonType::FAULT_CRASH;
 
        return ;
    }

    last_exit_reason = PUTExitReasonType::FAULT_NONE;
    return;    
}

FileFeedback PinToolExecutor::GetFileFeedback(fs::path feed_path) {
    return FileFeedback(feed_path, lock);
}

ExitStatusFeedback PinToolExecutor::GetExitStatusFeedback() {
    return ExitStatusFeedback(last_exit_reason, last_signal);
}

// this function may be called in signal handlers.
// use only async-signal-safe functions inside.
// basically, we care about only the case where NativeLinuxExecutor::Run is running.
// in that case, we should kill the child process(of PUT) so that NativeLinuxExecutor could halt without waiting the timeout.
// if this function is called during the call of other functions, then the child process is not active.
// we can call KillChildWithoutWait() anyways because the function checks if the child process is active.
void PinToolExecutor::ReceiveStopSignal(void) {
    // kill is async-signal-safe
    // the child process is active only in NativeLinuxExecutor::Run and Run always uses waitpid, so we don't need to use waitpid here
    KillChildWithoutWait();
}
