#pragma once

#include "Utils/Filesystem.hpp"

namespace LogFileLogger {
    void Println(std::string message);

    void Init(fs::path log_file_path);
}