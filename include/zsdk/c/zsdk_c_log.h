/**
 * @file zsdk_c_log.h
 * @brief C99 ABI for ZSDK zero-allocation logging (routes to BREW __DBGPRINTF & DebugOverlay).
 */

#ifndef ZSDK_C_LOG_H
#define ZSDK_C_LOG_H

#include "zsdk/c/zsdk_c_types.h"
#include <stdarg.h>

ZSDK_C_EXTERN_BEGIN

/**
 * @brief Logs a printf-formatted message through zsdk::debug::Logger.
 */
ZSDK_C_API void zsdk_log(zsdk_log_level_t level, const char* tag, const char* fmt, ...);

/**
 * @brief va_list variant of zsdk_log().
 */
ZSDK_C_API void zsdk_log_v(zsdk_log_level_t level, const char* tag, const char* fmt, va_list args);

/**
 * @brief Logs a pre-formatted plain string (safe for FFI callers like Lua/Rust without varargs).
 */
ZSDK_C_API void zsdk_log_msg(zsdk_log_level_t level, const char* tag, const char* message);

ZSDK_C_EXTERN_END

#ifndef ZSDK_ENABLE_LOGGING
    #define ZSDK_ENABLE_LOGGING 1
#endif

#if ZSDK_ENABLE_LOGGING
    #define ZSDK_C_LOG_INFO(tag, ...)  zsdk_log(ZSDK_LOG_LEVEL_INFO,  (tag), __VA_ARGS__)
    #define ZSDK_C_LOG_WARN(tag, ...)  zsdk_log(ZSDK_LOG_LEVEL_WARN,  (tag), __VA_ARGS__)
    #define ZSDK_C_LOG_ERROR(tag, ...) zsdk_log(ZSDK_LOG_LEVEL_ERROR, (tag), __VA_ARGS__)
#else
    #define ZSDK_C_LOG_INFO(tag, ...)  ((void)0)
    #define ZSDK_C_LOG_WARN(tag, ...)  ((void)0)
    #define ZSDK_C_LOG_ERROR(tag, ...) zsdk_log(ZSDK_LOG_LEVEL_ERROR, (tag), __VA_ARGS__)
#endif

#endif /* ZSDK_C_LOG_H */
