/**
 * @file logger.cpp
 * @brief Zero-allocation logger routing to BREW __DBGPRINTF and DebugOverlay.
 */

#include "zsdk/debug/logger.hpp"
#include "zsdk/debug/debug_overlay.hpp"

#include <stdarg.h>
#include <stdio.h>

#if ZSDK_PLATFORM_ZEEBO
    #include "AEEStdLib.h"
#endif

namespace zsdk {
namespace debug {

void Logger::Log(LogLevel level, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(level, tag, fmt, args);
    va_end(args);
}

void Logger::LogV(LogLevel level, const char* tag, const char* fmt, va_list args) {
    char buffer[ZSDK_DEBUG_LOG_LINE_LEN];
    const char* prefix = "I";
    if (level == LogLevel::Warn) {
        prefix = "W";
    } else if (level == LogLevel::Error) {
        prefix = "E";
    }

    int written = snprintf(buffer, sizeof(buffer), "[%s/%s] ", prefix, tag ? tag : "ZSDK");
    if (written < 0) {
        written = 0;
    }
    if (static_cast<size_t>(written) < sizeof(buffer) - 1u) {
        vsnprintf(buffer + written, sizeof(buffer) - static_cast<size_t>(written), fmt ? fmt : "", args);
    }
    buffer[sizeof(buffer) - 1u] = '\0';

#if ZSDK_PLATFORM_ZEEBO
    __DBGPRINTF("%s", buffer);
#else
    printf("%s\n", buffer);
#endif

    DebugOverlay::PushLog(buffer);
}

} // namespace debug
} // namespace zsdk
