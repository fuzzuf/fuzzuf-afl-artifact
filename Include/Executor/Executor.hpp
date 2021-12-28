#pragma once

#include <memory>
#include "Utils/Common.hpp"
#include "Feedback/PUTExitReasonType.hpp"

// あらゆる実行環境を抽象化し、ファズの実行を実現する基底クラス
// `なんとかExecutor` を派生するときは本クラスを継承してください。
// 以下に述べる事項は、本クラスおよびそれを継承するクラスに適用されます。
//
// 責務：
//  - 自クラスおよび継承したクラスのメンバー変数をすべて初期化すること
//  - 初期化後、各メンバー変数は有効な値であること
class Executor {
public:

    // コンストラクタ越しに渡される設定を保持するメンバたち
    const std::vector<std::string> argv; 
    u32 exec_timelimit_ms;
    u64 exec_memlimit;

    std::vector<const char*> cargv; // argvをchar*に変換したもの。char*が指すアドレスはc_str()で取得するものなので、argvのライフタイムが終わる時、これは参照してはいけなくなる

    // ファイル越しに入力を受け付けるPUTに対して渡すファイルパス（つまりExecutorはこのパスにファイルを作るので注意）
    const std::string path_str_to_write_input;

    int child_pid;    

    int input_fd; // Used for stdin mode in Executor::Run()
    int null_fd;
    bool stdin_mode; // FIXME: struct NativeLinuxExecutorArgs{cargv, stdin_mode} にしてconstにしてもいいかも

    Executor(  
        const std::vector<std::string> &argv,
        u32 exec_timelimit_ms,
        u64 exec_memlimit,
        const std::string path_str_to_write_input
    );
    virtual ~Executor() {};

    Executor( const Executor& ) = delete;
    Executor( Executor&& ) = delete;
    Executor &operator=( const Executor& ) = delete;
    Executor &operator=( Executor&& ) = delete;
    Executor() = delete;

    /// 各実行環境で使用する共通インターフェイスを定義
    // 入力を受け取って実際にPUTを実行する
    virtual void Run(const u8 *buf, u32 len, u32 timeout_ms=0) = 0;

    void KillChildWithoutWait();

    // SIGTERMシグナルが来たなどで早く停止すべき状況になった時に呼ばれる
    virtual void ReceiveStopSignal(void) = 0;

    void OpenExecutorDependantFiles();
    void WriteTestInputToFile(const u8 *buf, u32 len);

protected:
    std::shared_ptr<u8> lock;
};
