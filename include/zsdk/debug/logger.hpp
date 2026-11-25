/**
 * @file logger.hpp
 * @brief Lightweight logging routed to BREW DBGPRINTF and the in-game DebugOverlay.
 */

#ifndef ZSDK_DEBUG_LOGGER_HPP
#define ZSDK_DEBUG_LOGGER_HPP

#include "zsdk/core/config.hpp"

#include <stdarg.h>

namespace zsdk {
namespace debug {

enum class LogLevel : uint8_t {
    Info = 0,
    Warn,
    Error
};

/**
 * @brief Static zero-allocation logger.
 */
class Logger {
public:
    static void Log(LogLevel level, const char* tag, const char* fmt, ...);
    static void LogV(LogLevel level, const char* tag, const char* fmt, va_list args);
};

} // namespace debug
} // namespace zsdk

#include "zsdk/core/config.hpp"

#if ZSDK_ENABLE_LOGGING
    #define ZSDK_LOG_INFO(tag, ...)  ::zsdk::debug::Logger::Log(::zsdk::debug::LogLevel::Info,  tag, __VA_ARGS__)
    #define ZSDK_LOG_WARN(tag, ...)  ::zsdk::debug::Logger::Log(::zsdk::debug::LogLevel::Warn,  tag, __VA_ARGS__)
    #define ZSDK_LOG_ERROR(tag, ...) ::zsdk::debug::Logger::Log(::zsdk::debug::LogLevel::Error, tag, __VA_ARGS__)
#else
    #define ZSDK_LOG_INFO(tag, ...)  ((void)0)
    #define ZSDK_LOG_WARN(tag, ...)  ((void)0)
    #define ZSDK_LOG_ERROR(tag, ...) ::zsdk::debug::Logger::Log(::zsdk::debug::LogLevel::Error, tag, __VA_ARGS__)
#endif

#endif /* ZSDK_DEBUG_LOGGER_HPP */
