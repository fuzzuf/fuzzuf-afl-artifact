#include "Logger/Logger.hpp"
#include "Exceptions.hpp"

RunLevel runlevel = FUZZUF_DEFAULT_RUNLEVEL;

std::string to_string(Logger v) {
    switch (v) {
        case Logger::LogFile:
            return "LogFile";
        case Logger::Flc:
            return "Flc";
        default:
            throw exceptions::logger_error(Util::StrPrintf("Unknown Logger enum value: %d", v), __FILE__, __LINE__);
    }
}
