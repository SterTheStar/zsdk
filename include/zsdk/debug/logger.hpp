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

/** @brief Extra log destination; receives the formatted line (no trailing newline). */
typedef void (*LogSinkFn)(LogLevel level, const char* line, void* userData);

static const uint8_t kMaxLogSinks = 4u;

/**
 * @brief Static zero-allocation logger (BREW DBGPRINTF / stdout + DebugOverlay + optional sinks).
 */
class Logger {
public:
    static void Log(LogLevel level, const char* tag, const char* fmt, ...);

    /** @brief Adds an extra log sink (max kMaxLogSinks). False if full or duplicate. */
    static bool AddSink(LogSinkFn fn, void* userData);
    static bool RemoveSink(LogSinkFn fn, void* userData);

    /** @brief Lines below @p level are dropped everywhere (default: Info). */
    static void     SetMinLevel(LogLevel level);
    static LogLevel GetMinLevel();
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
