/**
 * @file frustum.cpp
 * @brief Zero-allocation 3D View Frustum Culling (96 bytes on stack).
 */

#include "zsdk/graphics/frustum.hpp"
#include <math.h>

namespace zsdk {
namespace graphics {

namespace {

inline float Dot3(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Sub3(const Vec3& a, const Vec3& b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 Add3(const Vec3& a, const Vec3& b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 Scale3(const Vec3& v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

inline Vec3 Cross3(const Vec3& a, const Vec3& b) {
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 Normalize3(const Vec3& v) {
    const float lenSq = Dot3(v, v);
    if (lenSq <= 1e-12f) {
        return Vec3{0.0f, 0.0f, 0.0f};
    }
    const float inv = 1.0f / sqrtf(lenSq);
    return Scale3(v, inv);
}

inline void NormalizePlane(Plane& p) {
    const float lenSq = p.a * p.a + p.b * p.b + p.c * p.c;
    if (lenSq > 1e-12f) {
        const float inv = 1.0f / sqrtf(lenSq);
        p.a *= inv;
        p.b *= inv;
        p.c *= inv;
        p.d *= inv;
    }
}

inline Plane MakePlane(const Vec3& normal, const Vec3& point) {
    const Vec3 n = Normalize3(normal);
    return Plane{n.x, n.y, n.z, -Dot3(n, point)};
}

} // namespace

void Frustum::ExtractFromViewProj(const float* m) {
    if (m == nullptr) {
        return;
    }

    /* Column-major 4x4 matrix: element(row, col) = m[col * 4 + row] */
    /* Left: row3 + row0 */
    m_planes[0] = Plane{m[3] + m[0], m[7] + m[4], m[11] + m[8],  m[15] + m[12]};
    /* Right: row3 - row0 */
    m_planes[1] = Plane{m[3] - m[0], m[7] - m[4], m[11] - m[8],  m[15] - m[12]};
    /* Bottom: row3 + row1 */
    m_planes[2] = Plane{m[3] + m[1], m[7] + m[5], m[11] + m[9],  m[15] + m[13]};
    /* Top: row3 - row1 */
    m_planes[3] = Plane{m[3] - m[1], m[7] - m[5], m[11] - m[9],  m[15] - m[13]};
    /* Near: row3 + row2 */
    m_planes[4] = Plane{m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]};
    /* Far: row3 - row2 */
    m_planes[5] = Plane{m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]};

    for (int32_t i = 0; i < 6; ++i) {
        NormalizePlane(m_planes[i]);
    }
}

void Frustum::ExtractFromCamera(const Vec3& eye, const Vec3& target, const Vec3& up,
                                float fovYDegrees, float aspect, float nearZ, float farZ) {
    const Vec3 forward = Normalize3(Sub3(target, eye));
    const Vec3 right   = Normalize3(Cross3(forward, up));
    const Vec3 camUp   = Cross3(right, forward);

    const float halfV = tanf(fovYDegrees * 0.5f * 0.01745329252f) * farZ;
    const float halfH = halfV * aspect;

    const Vec3 nearCenter = Add3(eye, Scale3(forward, nearZ));
    const Vec3 farCenter  = Add3(eye, Scale3(forward, farZ));
    const Vec3 farFromEye = Sub3(farCenter, eye);

    m_planes[0] = MakePlane(Cross3(Sub3(farFromEye, Scale3(right, halfH)), camUp), eye); /* Left */
    m_planes[1] = MakePlane(Cross3(camUp, Add3(farFromEye, Scale3(right, halfH))), eye); /* Right */
    m_planes[2] = MakePlane(Cross3(right, Sub3(farFromEye, Scale3(camUp, halfV))), eye); /* Bottom */
    m_planes[3] = MakePlane(Cross3(Add3(farFromEye, Scale3(camUp, halfV)), right), eye); /* Top */
    m_planes[4] = MakePlane(forward, nearCenter);                                         /* Near */
    m_planes[5] = MakePlane(Scale3(forward, -1.0f), farCenter);                           /* Far */
}

bool Frustum::TestSphere(const Vec3& center, float radius) const {
    for (int32_t i = 0; i < 6; ++i) {
        const Plane& p = m_planes[i];
        const float dist = p.a * center.x + p.b * center.y + p.c * center.z + p.d;
        if (dist < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::TestAabb(const Vec3& minPt, const Vec3& maxPt) const {
    for (int32_t i = 0; i < 6; ++i) {
        const Plane& p = m_planes[i];
        const float px = (p.a >= 0.0f) ? maxPt.x : minPt.x;
        const float py = (p.b >= 0.0f) ? maxPt.y : minPt.y;
        const float pz = (p.c >= 0.0f) ? maxPt.z : minPt.z;
        if (p.a * px + p.b * py + p.c * pz + p.d < 0.0f) {
            return false;
        }
    }
    return true;
}

} // namespace graphics
} // namespace zsdk
