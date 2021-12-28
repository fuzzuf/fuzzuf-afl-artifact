#pragma once
#include <string>

namespace StdoutLogger {
    void Println(std::string message);

    void Enable();
    void Disable();
}