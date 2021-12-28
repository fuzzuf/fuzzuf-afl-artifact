#include "CLI/FuzzerBuilderRegister.hpp"
#include "CLI/Fuzzers/AFL/BuildAFLFuzzerFromArgs.hpp"
#include "CLI/GlobalArgs.hpp"
#include "CLI/ParseGlobalOptionsForFuzzer.hpp"
#include "Exceptions.hpp"
#include "Logger/LogFileLogger.hpp"
#include "Logger/Logger.hpp"
#include "Logger/StdoutLogger.hpp"

#include <iostream>

void usage(const char *argv_0) {
    std::cerr << "Example usage:" << std::endl;
    std::cerr << "\t" << argv_0 << " afl --in_dir=test/put_binaries/libjpeg/seeds -- test/put_binaries/libjpeg/libjpeg_turbo_fuzzer @@" << std::endl;
    exit(1);
}

int main(int argc, const char *argv[]) {
    try {
        // コマンドラインオプションをパースするまでLoggerが確定しないので、標準出力へのログ出力を明示的に有効にする
        StdoutLogger::Enable();

        // ファザーが指定されていないのでUsageを表示して終了する
        if (argc < 2) {
            usage(argv[0]);
        }

        GlobalFuzzerOptions global_options;
        
        // いまのところサブコマンドでファザー以外が指定される予定がないため、雑にサブコマンドをパースする
        global_options.fuzzer = argv[1]; 

        // コマンドラインからファジングキャンペーン全体の設定を取得する
        // NOTE: argv[0] とサブコマンドを読み飛ばす点に注意
        GlobalArgs global_args = {
            .argc = argc - 2,
            .argv = &argv[2]
        };
        FuzzerArgs fuzzer_args = ParseGlobalOptionsForFuzzer(global_args, /* &mut */ global_options);

        if (global_options.help) {
            // ParseGlobalOptionsForFuzzer() 内で help メッセージを表示済みなのでそのまま終了する
            return 0;
        }

        // コマンドラインにしたがい、ログを取得・保存するインスタンス logger を初期化する
        StdoutLogger::Disable();
        if (global_options.logger == Logger::Stdout) {
            StdoutLogger::Enable();
        } else if (global_options.logger == Logger::LogFile) {
            if (global_options.log_file.has_value()) {
                LogFileLogger::Init(global_options.log_file.value());
            } else {
                throw exceptions::cli_error("LogFile logger is specified, but log_file is not specified. May be logic bug", __FILE__, __LINE__);
            }
        } else {
            throw exceptions::cli_error("Unsupported logger: " + to_string(global_options.logger), __FILE__, __LINE__);
        }
        
        // コマンドラインで指定されたファザーをコマンドライン通りに用意する
        auto fuzzer = FuzzerBuilderRegister::Get(global_options.fuzzer)(fuzzer_args, global_options);

        // TODO: シグナルハンドラの設定を実装
        // 将来的には、 fuzzer->ReceiveStopSignal() とか呼ぶのはCLIのシグナルハンドラの責務にしたい

        // ファジングキャンペーンはっじまっるよー
        // TODO: ファジングキャンペーンのタイムアウトは未実装。 雑に `while (true)` なのは許して
        while (true) {
            fuzzer->OneLoop();
            // 必要に応じて、OneLoopごとにHookを呼んでもいいかも
        }
    } catch (const exceptions::wyvern_runtime_error &e) {
        // エラーハンドリングは、最上位モジュールであるCLIが行う
        std::cerr << "[!] " << e.what() << std::endl;
        std::cerr << "\tat " << e.file << ":" << e.line << std::endl;

        return -1; // エラーで落ちたことをexit codeで知らせる
    } catch (const exceptions::wyvern_logic_error &e) {
        // エラーハンドリングは、最上位モジュールであるCLIが行う
        std::cerr << "[!] " << e.what() << std::endl;
        std::cerr << "\tat " << e.file << ":" << e.line << std::endl;

        return -1; // エラーで落ちたことをexit codeで知らせる
    } catch (const std::exception &e) {
        // エラーハンドリングは、最上位モジュールであるCLIが行う
        std::cerr << "[!] " << e.what() << std::endl;

        return -1; // エラーで落ちたことをexit codeで知らせる
    }
    
    return 0;
}
