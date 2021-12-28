#include "Logger/LogFileLogger.hpp"
#include "Logger/Logger.hpp"
#include "Utils/Filesystem.hpp"
#include <fstream>

namespace LogFileLogger {
    // Do not expose variables to outside of this module
    bool stream_to_log_file = false;
    std::ofstream log_file;

    void Println(std::string message) {
        if (stream_to_log_file) {
            log_file << message << std::endl;
        }
    }

    void Init(fs::path log_file_path) {
        stream_to_log_file = true;
        log_file = std::ofstream(log_file_path.native());

        DEBUG("[*] LogFileLogger::Init(): LogFileLogger = { stream_to_log_file=%s, log_file_path=%s }", 
            stream_to_log_file ? "true" : "false",
            log_file_path.string().c_str()
            );
    }
}