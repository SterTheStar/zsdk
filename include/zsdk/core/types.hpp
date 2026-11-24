/**
 * @file types.hpp
 * @brief Core fixed-width types, result codes, math primitives, and RAII helpers.
 *
 * Designed for ARM11 / BREW environments without C++ exceptions or RTTI.
 */

#ifndef ZSDK_CORE_TYPES_HPP
#define ZSDK_CORE_TYPES_HPP

#include "zsdk/core/config.hpp"

namespace zsdk {

/**
 * @brief Standardized status codes returned across all ZSDK subsystems.
 *
 * Negative values indicate errors; zero indicates success.
 * Mapped internally from BREW AEE_SUCCESS / E* error codes (AEEError.h / AEEStdErr.h).
 */
enum class Result : int32_t {
    Ok                  = 0,
    Error               = -1,
    InvalidArg          = -2,
    OutOfMemory         = -3,
    NotInitialized      = -4,
    AlreadyInitialized  = -5,
    NotSupported        = -6,
    NotFound            = -7,
    WouldBlock          = -8,
    Timeout             = -9,
    IoError             = -10,
    DeviceDisconnected  = -11,
    GraphicsError       = -12,
    AudioError          = -13,
    NetworkError        = -14
};

/** @brief Returns true if the Result represents success. */
ZSDK_FORCE_INLINE bool Succeeded(Result r) {
    return static_cast<int32_t>(r) >= 0;
}

/** @brief Returns true if the Result represents an error. */
ZSDK_FORCE_INLINE bool Failed(Result r) {
    return static_cast<int32_t>(r) < 0;
}

/**
 * @brief Base class to disallow copy construction and copy assignment without overhead.
 */
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

/** @brief 2D floating-point vector. */
struct Vec2 {
    float x;
    float y;
};

/** @brief 3D floating-point vector (used for accelerometer, gyro, and 3D math). */
struct Vec3 {
    float x;
    float y;
    float z;
};

/** @brief Integer rectangle in screen/pixel coordinates. */
struct Rect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

/** @brief Packed 32-bit RGBA color. */
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    static ZSDK_FORCE_INLINE Color Rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Color{r, g, b, a};
    }
};

} // namespace zsdk

#endif /* ZSDK_CORE_TYPES_HPP */
