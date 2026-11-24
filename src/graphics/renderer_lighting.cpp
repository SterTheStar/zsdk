/**
 * @file renderer_lighting.cpp
 * @brief Camera LookAt matrix, fixed-function lighting, and material helpers for Renderer.
 */

#include "zsdk/graphics/renderer.hpp"
#include "gl_platform.hpp"

#include <math.h>

namespace zsdk {
namespace graphics {

#ifndef ZSDK_GRAPHICS_RENDERER_TYPES_HPP
/** @brief Fixed-function light source types. */
enum class LightType : uint8_t {
    Directional = 0,
    Point,
    Spot
};

/** @brief Extended light configuration descriptor for Renderer::ConfigureLightEx. */
struct LightDesc {
    LightType type = LightType::Point;
    Vec3 position{0.0f, 0.0f, 1.0f};
    Vec3 direction{0.0f, 0.0f, -1.0f};
    Color ambient{0, 0, 0, 255};
    Color diffuse{255, 255, 255, 255};
    Color specular{255, 255, 255, 255};
    float constantAttenuation = 1.0f;
    float linearAttenuation = 0.0f;
    float quadraticAttenuation = 0.0f;
    float spotCutoff = 180.0f;
    float spotExponent = 0.0f;
};
#endif

namespace {

void ColorToGlFloat4(const Color& c, GLfloat out[4]) {
    const float inv = 1.0f / 255.0f;
    out[0] = static_cast<GLfloat>(c.r) * inv;
    out[1] = static_cast<GLfloat>(c.g) * inv;
    out[2] = static_cast<GLfloat>(c.b) * inv;
    out[3] = static_cast<GLfloat>(c.a) * inv;
}

} // namespace

void Renderer::LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const float fx = target.x - eye.x;
    const float fy = target.y - eye.y;
    const float fz = target.z - eye.z;
    const float fLenSq = fx * fx + fy * fy + fz * fz;
    if (fLenSq <= 1e-12f) {
        return;
    }
    const float invFLen = 1.0f / sqrtf(fLenSq);
    const float fNormX = fx * invFLen;
    const float fNormY = fy * invFLen;
    const float fNormZ = fz * invFLen;

    const float sx = fNormY * up.z - fNormZ * up.y;
    const float sy = fNormZ * up.x - fNormX * up.z;
    const float sz = fNormX * up.y - fNormY * up.x;
    const float sLenSq = sx * sx + sy * sy + sz * sz;
    if (sLenSq <= 1e-12f) {
        return;
    }
    const float invSLen = 1.0f / sqrtf(sLenSq);
    const float sNormX = sx * invSLen;
    const float sNormY = sy * invSLen;
    const float sNormZ = sz * invSLen;

    const float ux = sNormY * fNormZ - sNormZ * fNormY;
    const float uy = sNormZ * fNormX - sNormX * fNormZ;
    const float uz = sNormX * fNormY - sNormY * fNormX;

    const GLfloat view[16] = {
        sNormX,  ux, -fNormX, 0.0f,
        sNormY,  uy, -fNormY, 0.0f,
        sNormZ,  uz, -fNormZ, 0.0f,
        -(sNormX * eye.x + sNormY * eye.y + sNormZ * eye.z),
        -(ux * eye.x + uy * eye.y + uz * eye.z),
        (fNormX * eye.x + fNormY * eye.y + fNormZ * eye.z),
        1.0f
    };

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view);
}

void Renderer::SetNormalizeNormals(bool enabled) {
    if (enabled) glEnable(GL_NORMALIZE);
    else         glDisable(GL_NORMALIZE);
}

void Renderer::SetGlobalAmbientLight(const Color& color) {
    GLfloat amb[4];
    ColorToGlFloat4(color, amb);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
}

void Renderer::SetShadeModel(ShadeModel model) {
    glShadeModel((model == ShadeModel::Flat) ? GL_FLAT : GL_SMOOTH);
}

void Renderer::SetLighting(bool enabled) {
    if (enabled) {
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
    } else {
        glDisable(GL_LIGHTING);
    }
}

void Renderer::SetLightEnabled(uint8_t lightIndex, bool enabled) {
    const GLenum lightId = GL_LIGHT0 + static_cast<GLenum>(lightIndex & 7u);
    if (enabled) glEnable(lightId);
    else         glDisable(lightId);
}

void Renderer::ConfigureLight(uint8_t lightIndex, const Vec3& position,
                              const Color& ambient, const Color& diffuse, const Color& specular) {
    const GLenum lightId = GL_LIGHT0 + static_cast<GLenum>(lightIndex & 7u);
    GLfloat amb[4];
    GLfloat diff[4];
    GLfloat spec[4];
    ColorToGlFloat4(ambient, amb);
    ColorToGlFloat4(diffuse, diff);
    ColorToGlFloat4(specular, spec);
    const GLfloat pos[4] = {position.x, position.y, position.z, 1.0f};

    glEnable(lightId);
    glLightfv(lightId, GL_AMBIENT, amb);
    glLightfv(lightId, GL_DIFFUSE, diff);
    glLightfv(lightId, GL_SPECULAR, spec);
    glLightfv(lightId, GL_POSITION, pos);
}

void Renderer::ConfigureLightEx(uint8_t lightIndex, const LightDesc& desc) {
    const GLenum lightId = GL_LIGHT0 + static_cast<GLenum>(lightIndex & 7u);
    GLfloat amb[4];
    GLfloat diff[4];
    GLfloat spec[4];
    ColorToGlFloat4(desc.ambient, amb);
    ColorToGlFloat4(desc.diffuse, diff);
    ColorToGlFloat4(desc.specular, spec);

    GLfloat pos[4];
    if (desc.type == LightType::Directional) {
        pos[0] = desc.direction.x;
        pos[1] = desc.direction.y;
        pos[2] = desc.direction.z;
        pos[3] = 0.0f;
    } else {
        pos[0] = desc.position.x;
        pos[1] = desc.position.y;
        pos[2] = desc.position.z;
        pos[3] = 1.0f;
    }

    glEnable(lightId);
    glLightfv(lightId, GL_AMBIENT, amb);
    glLightfv(lightId, GL_DIFFUSE, diff);
    glLightfv(lightId, GL_SPECULAR, spec);
    glLightfv(lightId, GL_POSITION, pos);

    glLightf(lightId, GL_CONSTANT_ATTENUATION, desc.constAtten);
    glLightf(lightId, GL_LINEAR_ATTENUATION, desc.linearAtten);
    glLightf(lightId, GL_QUADRATIC_ATTENUATION, desc.quadAtten);

    if (desc.type == LightType::Spot) {
        const GLfloat spotDir[3] = {desc.direction.x, desc.direction.y, desc.direction.z};
        glLightfv(lightId, GL_SPOT_DIRECTION, spotDir);
        glLightf(lightId, GL_SPOT_CUTOFF, desc.spotCutoffDeg);
        glLightf(lightId, GL_SPOT_EXPONENT, desc.spotExponent);
    } else {
        glLightf(lightId, GL_SPOT_CUTOFF, 180.0f);
    }
}

void Renderer::SetMaterial(const Color& ambient, const Color& diffuse, const Color& specular, float shininess) {
    GLfloat amb[4];
    GLfloat diff[4];
    GLfloat spec[4];
    ColorToGlFloat4(ambient, amb);
    ColorToGlFloat4(diffuse, diff);
    ColorToGlFloat4(specular, spec);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

} // namespace graphics
} // namespace zsdk
