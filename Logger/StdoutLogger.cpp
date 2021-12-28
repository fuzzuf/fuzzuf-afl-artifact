#include "Logger/StdoutLogger.hpp"
#include <iostream>

namespace StdoutLogger {
    // Do not expose variables to outside of this module
    bool stream_to_stdout = true; // Default

    void Println(std::string message) {
        if (stream_to_stdout) {
            std::cout << message << std::endl;
        }
    }

    void Enable() {
        stream_to_stdout = true;
    }

    void Disable() {
        stream_to_stdout = false;

        // まだ出力されてないメッセージはいまのうちに吐き出しておく
        std::cout << std::flush;
    }
}