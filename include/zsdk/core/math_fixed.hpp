/**
 * @file math_fixed.hpp
 * @brief Zero-overhead 16.16 fixed-point math helpers for ARM11 / OpenGL ES 1.x.
 */

#ifndef ZSDK_CORE_MATH_FIXED_HPP
#define ZSDK_CORE_MATH_FIXED_HPP

#include "zsdk/core/config.hpp"

namespace zsdk {

/** @brief 16.16 signed fixed-point scalar (binary-compatible with GLfixed). */
typedef int32_t Fixed16;

static const Fixed16 kFixedOne  = 0x00010000;
static const Fixed16 kFixedHalf = 0x00008000;

ZSDK_FORCE_INLINE Fixed16 IntToFixed(int32_t v) {
    return v << 16;
}

ZSDK_FORCE_INLINE int32_t FixedToInt(Fixed16 f) {
    return f >> 16;
}

ZSDK_FORCE_INLINE Fixed16 FloatToFixed(float v) {
    return static_cast<Fixed16>(v * 65536.0f);
}

ZSDK_FORCE_INLINE float FixedToFloat(Fixed16 f) {
    return static_cast<float>(f) * (1.0f / 65536.0f);
}

ZSDK_FORCE_INLINE Fixed16 FixedMul(Fixed16 a, Fixed16 b) {
    return static_cast<Fixed16>((static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> 16);
}

ZSDK_FORCE_INLINE Fixed16 FixedDiv(Fixed16 a, Fixed16 b) {
    return static_cast<Fixed16>((static_cast<int64_t>(a) << 16) / static_cast<int64_t>(b));
}

} // namespace zsdk

#endif /* ZSDK_CORE_MATH_FIXED_HPP */
