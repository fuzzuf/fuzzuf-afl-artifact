#pragma once

#include <string>
#include "Utils/Common.hpp"
#include "Logger/LogFileLogger.hpp"
#include "Logger/StdoutLogger.hpp"

enum Logger {
    Stdout,
    LogFile,
    Flc, // 将来対応予定？
};

std::string to_string(Logger v);

enum RunLevel {
    MODE_RELEASE, MODE_DEBUG,
};

extern RunLevel runlevel;

#define MSG(...) std::printf(__VA_ARGS__)
#define FUZZUF_FORMAT( temp, ... ) \
  { \
    temp.assign( 4000, ' ' ); \
    auto size = snprintf( temp.data(), temp.size(), __VA_ARGS__ ); \
    temp.resize( size ); \
    temp.shrink_to_fit(); \
  }

#define IS_DEBUG_MODE() (runlevel >= RunLevel::MODE_DEBUG)
#define DEBUG(...) { \
    if ( runlevel >= RunLevel::MODE_DEBUG || ::Util::has_logger() ) { \
        std::string temp; \
        FUZZUF_FORMAT( temp, __VA_ARGS__ ) \
        if (runlevel >= RunLevel::MODE_DEBUG) \
            StdoutLogger::Println( temp ); \
            LogFileLogger::Println( temp ); \
        if( ::Util::has_logger() ) \
            ::Util::log( "log.debug.debug", nlohmann::json( {{"message",std::move(temp)},{"file",__FILE__},{"line",__LINE__}} ), []( auto ){} ); \
    }\
    }

#define ERROR(...) {\
    fflush(stdout); \
    std::string temp; \
    FUZZUF_FORMAT( temp, __VA_ARGS__ ) \
    MSG("\n[-]  SYSTEM ERROR : "); \
    std::puts( temp.c_str() ); \
    MSG("\n    Stop location : %s(), %s:%u\n", \
         __FUNCTION__, __FILE__, __LINE__); \
    MSG("       OS message : %s\n", std::strerror(errno)); \
    if( ::Util::has_logger() ) \
        ::Util::log( "log.error.system_error", nlohmann::json( {{"message",std::move(temp)},{"file",__FILE__},{"line",__LINE__},{"errno",errno},{"strerror",std::strerror(errno)}} ) ); \
    std::exit(1); \
    }
