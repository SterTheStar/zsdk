/**
 * @file frustum.hpp
 * @brief Zero-allocation 3D View Frustum Culling (96 bytes on stack).
 */

#ifndef ZSDK_GRAPHICS_FRUSTUM_HPP
#define ZSDK_GRAPHICS_FRUSTUM_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

struct Plane {
    float a, b, c, d;
};

/**
 * @brief 6-plane view frustum for fast sphere/AABB culling on ARM11.
 */
class Frustum {
public:
    /** @brief Builds frustum from a column-major 4x4 View-Projection matrix. */
    void ExtractFromViewProj(const float* vp16);

    /** @brief Builds frustum directly from camera basis + perspective parameters. */
    void ExtractFromCamera(const Vec3& eye, const Vec3& target, const Vec3& up,
                           float fovYDegrees, float aspect, float nearZ, float farZ);

    bool TestSphere(const Vec3& center, float radius) const;
    bool TestAabb(const Vec3& minPt, const Vec3& maxPt) const;

private:
    Plane m_planes[6];
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_FRUSTUM_HPP */
