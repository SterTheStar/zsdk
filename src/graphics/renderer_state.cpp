/**
 * @file renderer_state.cpp
 * @brief OpenGL ES 1.1 rasterization, stencil, clip-plane, texture-unit, and fixed-point transform state.
 */

#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/renderer_types.hpp"
#include "zsdk/core/math_fixed.hpp"
#include "gl_platform.hpp"

namespace zsdk {
namespace graphics {

namespace {

GLenum ToGlCompare(CompareFunc f) {
    switch (f) {
        case CompareFunc::Never:        return GL_NEVER;
        case CompareFunc::Less:         return GL_LESS;
        case CompareFunc::Equal:        return GL_EQUAL;
        case CompareFunc::LessEqual:    return GL_LEQUAL;
        case CompareFunc::Greater:      return GL_GREATER;
        case CompareFunc::NotEqual:     return GL_NOTEQUAL;
        case CompareFunc::GreaterEqual: return GL_GEQUAL;
        case CompareFunc::Always:
        default:                        return GL_ALWAYS;
    }
}

GLenum ToGlStencilOp(StencilOp op) {
    switch (op) {
        case StencilOp::Zero:    return GL_ZERO;
        case StencilOp::Replace: return GL_REPLACE;
        case StencilOp::Incr:    return GL_INCR;
        case StencilOp::Decr:    return GL_DECR;
        case StencilOp::Invert:  return GL_INVERT;
        case StencilOp::Keep:
        default:                 return GL_KEEP;
    }
}

GLint ToGlTexEnvMode(TexEnvMode mode) {
    switch (mode) {
        case TexEnvMode::Replace: return GL_REPLACE;
        case TexEnvMode::Decal:   return GL_DECAL;
        case TexEnvMode::Add:     return GL_ADD;
        case TexEnvMode::Modulate:
        default:                  return GL_MODULATE;
    }
}

} // namespace

void Renderer::SetStencilFunc(CompareFunc func, int32_t ref, uint32_t mask) {
    glStencilFunc(ToGlCompare(func), static_cast<GLint>(ref), static_cast<GLuint>(mask));
}

void Renderer::SetStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass) {
    glStencilOp(ToGlStencilOp(fail), ToGlStencilOp(zfail), ToGlStencilOp(zpass));
}

void Renderer::SetColorMask(bool r, bool g, bool b, bool a) {
    glColorMask(r ? GL_TRUE : GL_FALSE,
                g ? GL_TRUE : GL_FALSE,
                b ? GL_TRUE : GL_FALSE,
                a ? GL_TRUE : GL_FALSE);
}

void Renderer::SetPolygonOffset(bool enabled, float factor, float units) {
    if (enabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(static_cast<GLfloat>(factor), static_cast<GLfloat>(units));
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void Renderer::SetFrontFace(FrontFace face) {
    glFrontFace(face == FrontFace::Clockwise ? GL_CW : GL_CCW);
}

void Renderer::SetClipPlane(uint8_t planeIndex, bool enabled, float a, float b, float c, float d) {
    if (planeIndex >= 6u) {
        return;
    }
    const GLenum plane = GL_CLIP_PLANE0 + static_cast<GLenum>(planeIndex);
    if (enabled) {
        const GLfloat equation[4] = {
            static_cast<GLfloat>(a),
            static_cast<GLfloat>(b),
            static_cast<GLfloat>(c),
            static_cast<GLfloat>(d)
        };
        glClipPlanef(plane, equation);
        glEnable(plane);
    } else {
        glDisable(plane);
    }
}

void Renderer::SetActiveTextureUnit(uint8_t unit) {
    const GLenum texUnit = GL_TEXTURE0 + static_cast<GLenum>(unit);
    glActiveTexture(texUnit);
    glClientActiveTexture(texUnit);
}

void Renderer::SetTextureEnvMode(TexEnvMode mode) {
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, ToGlTexEnvMode(mode));
}

void Renderer::ResetTextureMatrix() {
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::TranslateTexture(float u, float v) {
    glMatrixMode(GL_TEXTURE);
    glTranslatef(static_cast<GLfloat>(u), static_cast<GLfloat>(v), 0.0f);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::ScaleTexture(float su, float sv) {
    glMatrixMode(GL_TEXTURE);
    glScalef(static_cast<GLfloat>(su), static_cast<GLfloat>(sv), 1.0f);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::TranslateFx(Fixed16 x, Fixed16 y, Fixed16 z) {
    glTranslatex(x, y, z);
}

void Renderer::RotateFx(Fixed16 angleDeg, Fixed16 x, Fixed16 y, Fixed16 z) {
    glRotatex(angleDeg, x, y, z);
}

void Renderer::ScaleFx(Fixed16 x, Fixed16 y, Fixed16 z) {
    glScalex(x, y, z);
}

} // namespace graphics
} // namespace zsdk
