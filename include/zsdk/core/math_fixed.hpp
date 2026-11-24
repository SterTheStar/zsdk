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

/**
 * @brief Fast 16.16 fixed-point sine for @p angleDeg in degrees (zero float ops on ARM11).
 *        Uses integer Bhaskara I approximation (max error < 0.0016, exact at 0/30/90/150/180).
 */
ZSDK_FORCE_INLINE Fixed16 FixedSinDeg(Fixed16 angleDeg) {
    static const int32_t k360Fx = 360 * 65536;
    static const int32_t k180Fx = 180 * 65536;
    int32_t a = angleDeg % k360Fx;
    if (a < 0) {
        a += k360Fx;
    }
    bool neg = false;
    if (a >= k180Fx) {
        a -= k180Fx;
        neg = true;
    }
    const int64_t x = static_cast<int64_t>(a >> 8); /* 0..46080 (8 fractional bits) */
    const int64_t p = x * (46080LL - x);
    const int64_t num = (4LL * p) << 16;
    const int64_t den = (40500LL << 16) - p;
    const Fixed16 res = (den > 0) ? static_cast<Fixed16>(num / den) : 0;
    return neg ? -res : res;
}

/** @brief Fast 16.16 fixed-point cosine for @p angleDeg in degrees (zero float ops on ARM11). */
ZSDK_FORCE_INLINE Fixed16 FixedCosDeg(Fixed16 angleDeg) {
    return FixedSinDeg(angleDeg + (90 * 65536));
}

} // namespace zsdk

#endif /* ZSDK_CORE_MATH_FIXED_HPP */
