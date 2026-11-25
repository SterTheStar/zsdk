/**
 * @file zsdk_c_log.cpp
 * @brief C ABI bridge for zsdk::debug::Logger.
 */

#include "zsdk/c/zsdk_c_log.h"
#include "zsdk/debug/logger.hpp"

namespace {

zsdk::debug::LogLevel ToCppLogLevel(zsdk_log_level_t level) {
    switch (level) {
        case ZSDK_LOG_LEVEL_WARN:  return zsdk::debug::LogLevel::Warn;
        case ZSDK_LOG_LEVEL_ERROR: return zsdk::debug::LogLevel::Error;
        case ZSDK_LOG_LEVEL_INFO:
        default:                   return zsdk::debug::LogLevel::Info;
    }
}

} // namespace

extern "C" {

void zsdk_log(zsdk_log_level_t level, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    zsdk::debug::Logger::LogV(ToCppLogLevel(level), tag, fmt, args);
    va_end(args);
}

void zsdk_log_v(zsdk_log_level_t level, const char* tag, const char* fmt, va_list args) {
    zsdk::debug::Logger::LogV(ToCppLogLevel(level), tag, fmt, args);
}

void zsdk_log_msg(zsdk_log_level_t level, const char* tag, const char* message) {
    zsdk::debug::Logger::Log(ToCppLogLevel(level), tag, "%s", message ? message : "");
}

} /* extern "C" */
