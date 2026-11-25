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

namespace {
struct SinkSlot { LogSinkFn fn; void* user; };
SinkSlot g_sinks[kMaxLogSinks];
LogLevel g_minLevel = LogLevel::Info;
}

bool Logger::AddSink(LogSinkFn fn, void* userData) {
    if (!fn) return false;
    int freeIdx = -1;
    for (uint8_t i = 0u; i < kMaxLogSinks; ++i) {
        if (g_sinks[i].fn == fn && g_sinks[i].user == userData) return false;
        if (g_sinks[i].fn == nullptr && freeIdx < 0) freeIdx = i;
    }
    if (freeIdx < 0) return false;
    g_sinks[freeIdx].fn = fn;
    g_sinks[freeIdx].user = userData;
    return true;
}

bool Logger::RemoveSink(LogSinkFn fn, void* userData) {
    for (uint8_t i = 0u; i < kMaxLogSinks; ++i) {
        if (g_sinks[i].fn == fn && g_sinks[i].user == userData) {
            g_sinks[i].fn = nullptr;
            g_sinks[i].user = nullptr;
            return true;
        }
    }
    return false;
}

void Logger::SetMinLevel(LogLevel level) { g_minLevel = level; }
LogLevel Logger::GetMinLevel() { return g_minLevel; }

void Logger::Log(LogLevel level, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(level, tag, fmt, args);
    va_end(args);
}

void Logger::LogV(LogLevel level, const char* tag, const char* fmt, va_list args) {
    if (static_cast<uint8_t>(level) < static_cast<uint8_t>(g_minLevel)) {
        return;
    }
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

    for (uint8_t i = 0u; i < kMaxLogSinks; ++i) {
        if (g_sinks[i].fn != nullptr) {
            g_sinks[i].fn(level, buffer, g_sinks[i].user);
        }
    }
}

} // namespace debug
} // namespace zsdk
