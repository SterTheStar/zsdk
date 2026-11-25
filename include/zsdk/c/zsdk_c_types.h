/**
 * @file zsdk_c_types.h
 * @brief Stable C99 ABI types and linkage macros for ZSDK FFI & language bindings.
 */

#ifndef ZSDK_C_TYPES_H
#define ZSDK_C_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
    #define ZSDK_C_EXTERN_BEGIN extern "C" {
    #define ZSDK_C_EXTERN_END   }
#else
    #define ZSDK_C_EXTERN_BEGIN
    #define ZSDK_C_EXTERN_END
#endif

#if defined(_WIN32) && defined(ZSDK_BUILD_SHARED)
    #define ZSDK_C_API __declspec(dllexport)
#elif defined(_WIN32) && defined(ZSDK_USE_SHARED)
    #define ZSDK_C_API __declspec(dllimport)
#else
    #define ZSDK_C_API
#endif

ZSDK_C_EXTERN_BEGIN

/** @brief C ABI mirror of zsdk::Result (must match types.hpp 1:1). */
typedef enum zsdk_result_t {
    ZSDK_C_OK                   =  0,
    ZSDK_C_ERR_FAILED           = -1,
    ZSDK_C_ERR_INVALID_ARG      = -2,
    ZSDK_C_ERR_OUT_OF_MEMORY    = -3,
    ZSDK_C_ERR_NOT_INITIALIZED  = -4,
    ZSDK_C_ERR_ALREADY_INIT     = -5,
    ZSDK_C_ERR_NOT_SUPPORTED    = -6,
    ZSDK_C_ERR_NOT_FOUND        = -7,
    ZSDK_C_ERR_WOULD_BLOCK      = -8,
    ZSDK_C_ERR_TIMEOUT          = -9,
    ZSDK_C_ERR_IO               = -10,
    ZSDK_C_ERR_DEVICE_DISCONN   = -11,
    ZSDK_C_ERR_GRAPHICS         = -12,
    ZSDK_C_ERR_AUDIO            = -13,
    ZSDK_C_ERR_NETWORK          = -14
} zsdk_result_t;

/** @brief C ABI mirror of zsdk::debug::LogLevel. */
typedef enum zsdk_log_level_t {
    ZSDK_LOG_LEVEL_INFO  = 0,
    ZSDK_LOG_LEVEL_WARN  = 1,
    ZSDK_LOG_LEVEL_ERROR = 2
} zsdk_log_level_t;

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_TYPES_H */
