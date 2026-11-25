/**
 * @file config.hpp
 * @brief Compile-time configuration, platform detection, and feature flags for ZSDK.
 *
 * ZSDK is a zero-overhead Platform Abstraction Layer (PAL) over Qualcomm BREW 4.x
 * and the Zeebo console extensions (OpenGL ES 1.1, EGL, IHID, Z-Pad).
 */

#ifndef ZSDK_CORE_CONFIG_HPP
#define ZSDK_CORE_CONFIG_HPP

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Platform / Backend Detection
 * ========================================================================== */

#if !defined(ZSDK_PLATFORM_ZEEBO) && !defined(ZSDK_PLATFORM_WINDOWS)
    #if defined(_WIN32) && !defined(AEE_SIMULATOR)
        /* Native PC/Windows backend (for future host builds) */
        #define ZSDK_PLATFORM_WINDOWS 1
        #define ZSDK_PLATFORM_ZEEBO   0
    #else
        /* Zeebo hardware or BREW Simulator (ARM11 / BREW 4.0.2) */
        #define ZSDK_PLATFORM_ZEEBO   1
        #define ZSDK_PLATFORM_WINDOWS 0
    #endif
#endif

#ifndef ZSDK_PLATFORM_ZEEBO
    #define ZSDK_PLATFORM_ZEEBO 0
#endif

#ifndef ZSDK_PLATFORM_WINDOWS
    #define ZSDK_PLATFORM_WINDOWS 0
#endif

/* ============================================================================
 * Hardware Budget Defaults (Zeebo: ARM11 @ 528MHz, 32 MB RAM/VRAM)
 * ========================================================================== */

#ifndef ZSDK_DEFAULT_MEMORY_LIMIT_BYTES
    /** @brief Default total RAM/VRAM budget used by the profiler (32 MiB). */
    #define ZSDK_DEFAULT_MEMORY_LIMIT_BYTES (32u * 1024u * 1024u)
#endif

#ifndef ZSDK_MAX_GAMEPADS
    /** @brief Maximum simultaneous controllers supported (P1..P4). */
    #define ZSDK_MAX_GAMEPADS 4u
#endif

#ifndef ZSDK_MAX_SFX_CHANNELS
    /**
     * @brief Concurrent SFX voices in AudioEngine. The documented BREW mixer recipe is
     *        1 MIDI/MMF/PMD stream + up to 4 QCP/AMR/ADPCM streams (all 4 of the same type),
     *        so Zeebo defaults to 4; the PC backend can mix more.
     */
    #if ZSDK_PLATFORM_ZEEBO
        #define ZSDK_MAX_SFX_CHANNELS 4u
    #else
        #define ZSDK_MAX_SFX_CHANNELS 8u
    #endif
#endif

#ifndef ZSDK_NAND_WRITE_GUARD
    /**
     * @brief Blocks write/delete/mkdir operations outside fs:/card0/ and fs:/~/
     *        to protect the Zeebo's primary system NAND from accidental writes.
     */
    #define ZSDK_NAND_WRITE_GUARD 1
#endif

/* ============================================================================
 * Debug Overlay & Memory Tracker Feature Flags
 * ========================================================================== */

#ifndef ZSDK_ENABLE_DEBUG_OVERLAY
    #if defined(NDEBUG) && !defined(ZSDK_FORCE_DEBUG_OVERLAY)
        #define ZSDK_ENABLE_DEBUG_OVERLAY 0
    #else
        #define ZSDK_ENABLE_DEBUG_OVERLAY 1
    #endif
#endif

#ifndef ZSDK_ENABLE_MEMORY_TRACKING
    #define ZSDK_ENABLE_MEMORY_TRACKING ZSDK_ENABLE_DEBUG_OVERLAY
#endif

#ifndef ZSDK_DEBUG_LOG_CAPACITY
    /** @brief Number of recent log lines kept in the in-memory ring buffer. */
    #define ZSDK_DEBUG_LOG_CAPACITY 32u
#endif

#ifndef ZSDK_DEBUG_LOG_LINE_LEN
    /** @brief Maximum characters per log line (including null terminator). */
    #define ZSDK_DEBUG_LOG_LINE_LEN 96u
#endif

/* ============================================================================
 * Compiler & Optimization Hints (No Exceptions, No RTTI)
 * ========================================================================== */

#if defined(_MSC_VER)
    #define ZSDK_FORCE_INLINE __forceinline
    #define ZSDK_NO_INLINE    __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__) || defined(__ARMCC_VERSION)
    #define ZSDK_FORCE_INLINE inline __attribute__((always_inline))
    #define ZSDK_NO_INLINE    __attribute__((noinline))
#else
    #define ZSDK_FORCE_INLINE inline
    #define ZSDK_NO_INLINE
#endif

#define ZSDK_UNUSED(x) ((void)(x))

#endif /* ZSDK_CORE_CONFIG_HPP */
