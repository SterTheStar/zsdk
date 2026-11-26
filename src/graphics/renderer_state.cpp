/**
 * @file renderer_state.cpp
 * @brief OpenGL ES 1.1 rasterization, stencil, clip-plane, texture-unit, and fixed-point transform state.
 */

#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/renderer_types.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/core/math_fixed.hpp"
#include "gl_platform.hpp"
#if ZSDK_PLATFORM_ZEEBO
    #include <gles/egl.h>
#endif

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
#if ZSDK_PLATFORM_ZEEBO
    /* Zeebo Developer Guide Table 7-1: User Clip Planes are not supported on Q3Dimension. */
    ZSDK_UNUSED(planeIndex);
    ZSDK_UNUSED(enabled);
    ZSDK_UNUSED(a);
    ZSDK_UNUSED(b);
    ZSDK_UNUSED(c);
    ZSDK_UNUSED(d);
#else
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
#endif
}

void Renderer::SetActiveTextureUnit(uint8_t unit) {
    const GLenum texUnit = GL_TEXTURE0 + static_cast<GLenum>(unit);
    glActiveTexture(texUnit);
    glClientActiveTexture(texUnit);
}

void Renderer::SetTextureEnvMode(TexEnvMode mode) {
    if (mode == TexEnvMode::Combine) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(0x8570 /* GL_COMBINE */));
    } else if (mode == TexEnvMode::Dot3) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(0x8570 /* GL_COMBINE */));
        glTexEnvf(GL_TEXTURE_ENV, 0x8571 /* GL_COMBINE_RGB */, static_cast<GLfloat>(0x86AE /* GL_DOT3_RGB */));
    } else {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(ToGlTexEnvMode(mode)));
    }
}

void Renderer::SetPointSpritesEnabled(bool enabled) {
    const GLenum kPointSprite = 0x8861; /* GL_POINT_SPRITE_OES */
    const GLenum kCoordReplace = 0x8862; /* GL_COORD_REPLACE_OES */
    if (enabled) {
        glEnable(kPointSprite);
        glTexEnvf(kPointSprite, kCoordReplace, static_cast<GLfloat>(GL_TRUE));
    } else {
        glTexEnvf(kPointSprite, kCoordReplace, static_cast<GLfloat>(GL_FALSE));
        glDisable(kPointSprite);
    }
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

void Renderer::SetDither(bool enabled) {
    if (enabled) glEnable(GL_DITHER);
    else         glDisable(GL_DITHER);
}

void Renderer::SetPerspectiveCorrectionFast(bool fast) {
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, fast ? GL_FASTEST : GL_NICEST);
}

void Renderer::SetBlendEquation(BlendEquationMode mode) {
    GLenum glMode = 0x8006u; /* GL_FUNC_ADD */
    switch (mode) {
        case BlendEquationMode::Subtract:        glMode = 0x800Au; break; /* GL_FUNC_SUBTRACT */
        case BlendEquationMode::ReverseSubtract: glMode = 0x800Bu; break; /* GL_FUNC_REVERSE_SUBTRACT */
        case BlendEquationMode::Min:             glMode = 0x8007u; break; /* GL_MIN */
        case BlendEquationMode::Max:             glMode = 0x8008u; break; /* GL_MAX */
        case BlendEquationMode::Add:
        default:                                 glMode = 0x8006u; break;
    }

#if ZSDK_PLATFORM_ZEEBO
    typedef void (*PfnBlendEq)(GLenum);
    static bool s_resolved = false;
    static PfnBlendEq s_fn = nullptr;
    if (!s_resolved) {
        s_fn = reinterpret_cast<PfnBlendEq>(eglGetProcAddress("glBlendEquationOES"));
        if (!s_fn) {
            s_fn = reinterpret_cast<PfnBlendEq>(eglGetProcAddress("glBlendEquationEXT"));
        }
        s_resolved = true;
    }
    if (s_fn) {
        s_fn(glMode);
    }
#else
    typedef void (APIENTRY *PfnBlendEq)(GLenum);
    static bool s_resolved = false;
    static PfnBlendEq s_fn = nullptr;
    if (!s_resolved) {
        s_fn = reinterpret_cast<PfnBlendEq>(wglGetProcAddress("glBlendEquation"));
        if (!s_fn) {
            s_fn = reinterpret_cast<PfnBlendEq>(wglGetProcAddress("glBlendEquationEXT"));
        }
        s_resolved = true;
    }
    if (s_fn) {
        s_fn(glMode);
    }
#endif
}

void Renderer::SetMatrixPaletteEnabled(bool enabled) {
    const GLenum kMatrixPalette = 0x8840u; /* GL_MATRIX_PALETTE_OES */
    if (enabled) glEnable(kMatrixPalette);
    else         glDisable(kMatrixPalette);
}

void Renderer::LoadPaletteMatrixFromModelView(uint32_t matrixIndex) {
#if ZSDK_PLATFORM_ZEEBO
    typedef void (*PfnCurrPal)(GLuint);
    typedef void (*PfnLoadPal)(void);
    static bool s_resolved = false;
    static PfnCurrPal s_curr = nullptr;
    static PfnLoadPal s_load = nullptr;
    if (!s_resolved) {
        s_curr = reinterpret_cast<PfnCurrPal>(eglGetProcAddress("glCurrentPaletteMatrixOES"));
        s_load = reinterpret_cast<PfnLoadPal>(eglGetProcAddress("glLoadPaletteFromModelViewMatrixOES"));
        s_resolved = true;
    }
    if (s_curr && s_load) {
        s_curr(static_cast<GLuint>(matrixIndex));
        s_load();
    }
#else
    ZSDK_UNUSED(matrixIndex);
#endif
}

void Renderer::BindSkinningBuffers(const MeshBuffer& weightVbo, uint8_t weightsPerVertex,
                                   const MeshBuffer& indexVbo, uint8_t indicesPerVertex) {
#if ZSDK_PLATFORM_ZEEBO
    typedef void (*PfnWeightPtr)(GLint, GLenum, GLsizei, const GLvoid*);
    typedef void (*PfnIdxPtr)(GLint, GLenum, GLsizei, const GLvoid*);
    static bool s_resolved = false;
    static PfnWeightPtr s_weight = nullptr;
    static PfnIdxPtr    s_idx    = nullptr;
    if (!s_resolved) {
        s_weight = reinterpret_cast<PfnWeightPtr>(eglGetProcAddress("glWeightPointerOES"));
        s_idx    = reinterpret_cast<PfnIdxPtr>(eglGetProcAddress("glMatrixIndexPointerOES"));
        s_resolved = true;
    }
    if (s_weight && weightVbo.IsValid() && weightsPerVertex > 0u) {
        weightVbo.Bind();
        glEnableClientState(0x86ADu /* GL_WEIGHT_ARRAY_OES */);
        s_weight(static_cast<GLint>(weightsPerVertex), GL_FLOAT, 0, weightVbo.GetPointerAtOffset(0));
    }
    if (s_idx && indexVbo.IsValid() && indicesPerVertex > 0u) {
        indexVbo.Bind();
        glEnableClientState(0x8844u /* GL_MATRIX_INDEX_ARRAY_OES */);
        s_idx(static_cast<GLint>(indicesPerVertex), GL_UNSIGNED_BYTE, 0, indexVbo.GetPointerAtOffset(0));
    }
    MeshBuffer::Unbind(BufferType::Vertex);
#else
    ZSDK_UNUSED(weightVbo);
    ZSDK_UNUSED(weightsPerVertex);
    ZSDK_UNUSED(indexVbo);
    ZSDK_UNUSED(indicesPerVertex);
#endif
}

void Renderer::UnbindSkinningBuffers() {
#if ZSDK_PLATFORM_ZEEBO
    glDisableClientState(0x86ADu /* GL_WEIGHT_ARRAY_OES */);
    glDisableClientState(0x8844u /* GL_MATRIX_INDEX_ARRAY_OES */);
#endif
}

void Renderer::BindPointSizeBuffer(const MeshBuffer& vbo) {
#if ZSDK_PLATFORM_ZEEBO
    typedef void (*PfnPointSizePtr)(GLenum, GLsizei, const GLvoid*);
    static bool s_resolved = false;
    static PfnPointSizePtr s_fn = nullptr;
    if (!s_resolved) {
        s_fn = reinterpret_cast<PfnPointSizePtr>(eglGetProcAddress("glPointSizePointerOES"));
        s_resolved = true;
    }
    if (s_fn && vbo.IsValid()) {
        vbo.Bind();
        glEnableClientState(0x8B9Cu /* GL_POINT_SIZE_ARRAY_OES */);
        s_fn(GL_FLOAT, 0, vbo.GetPointerAtOffset(0));
        MeshBuffer::Unbind(BufferType::Vertex);
    }
#else
    ZSDK_UNUSED(vbo);
#endif
}

void Renderer::UnbindPointSizeBuffer() {
#if ZSDK_PLATFORM_ZEEBO
    glDisableClientState(0x8B9Cu /* GL_POINT_SIZE_ARRAY_OES */);
#endif
}

} // namespace graphics
} // namespace zsdk
