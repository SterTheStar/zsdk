/**
 * @file renderer.cpp
 * @brief Core OpenGL ES 1.1 viewport, clear, matrix, fog, attribute, and draw-call helpers.
 *
 * Lighting, advanced raster state, and 2D/interleaved helpers are implemented in
 * renderer_lighting.cpp, renderer_state.cpp, and renderer_2d_vbo.cpp.
 */

#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/logger.hpp"
#include "gl_platform.hpp"

#include <math.h>

namespace zsdk {
namespace graphics {

namespace {

struct RenderStateCache {
    int8_t  blendMode;
    int8_t  depthTest;
    int8_t  cullFace;
    int8_t  texture2D;
    bool    viewportValid;
    int32_t vpX;
    int32_t vpY;
    int32_t vpWidth;
    int32_t vpHeight;
};

RenderStateCache g_stateCache = {-1, -1, -1, -1, false, 0, 0, 0, 0};
int32_t g_modelViewDepth = 1;
bool    g_warnedMvOverflow = false;

GLenum ToGlPrimitive(PrimitiveType p) {
    switch (p) {
        case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveType::Lines:         return GL_LINES;
        case PrimitiveType::Points:        return GL_POINTS;
        case PrimitiveType::Triangles:
        default:                           return GL_TRIANGLES;
    }
}

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

} // namespace

void Renderer::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (g_stateCache.viewportValid &&
        g_stateCache.vpX == x &&
        g_stateCache.vpY == y &&
        g_stateCache.vpWidth == width &&
        g_stateCache.vpHeight == height) {
        return;
    }
    g_stateCache.viewportValid = true;
    g_stateCache.vpX           = x;
    g_stateCache.vpY           = y;
    g_stateCache.vpWidth       = width;
    g_stateCache.vpHeight      = height;
    glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void Renderer::SetScissor(bool enabled, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(x), static_cast<GLint>(y),
                  static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void Renderer::Clear(const Color& color, bool clearDepth, bool clearStencil) {
    const float inv = 1.0f / 255.0f;
    glClearColor(static_cast<GLfloat>(color.r) * inv,
                 static_cast<GLfloat>(color.g) * inv,
                 static_cast<GLfloat>(color.b) * inv,
                 static_cast<GLfloat>(color.a) * inv);
    GLbitfield mask = GL_COLOR_BUFFER_BIT;
    if (clearDepth)   mask |= GL_DEPTH_BUFFER_BIT;
    if (clearStencil) mask |= GL_STENCIL_BUFFER_BIT;
    glClear(mask);
}

void Renderer::ResetStateCache() {
    g_stateCache.blendMode     = -1;
    g_stateCache.depthTest     = -1;
    g_stateCache.cullFace      = -1;
    g_stateCache.texture2D     = -1;
    g_stateCache.viewportValid = false;
}

void Renderer::SetBlendMode(BlendMode mode) {
    const int8_t val = static_cast<int8_t>(mode);
    if (g_stateCache.blendMode == val) {
        return;
    }
    g_stateCache.blendMode = val;
    switch (mode) {
        case BlendMode::Opaque:
            glDisable(GL_BLEND);
            break;
        case BlendMode::Alpha:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
    }
}

void Renderer::SetDepthTest(bool enabled) {
    const int8_t val = enabled ? 1 : 0;
    if (g_stateCache.depthTest == val) {
        return;
    }
    g_stateCache.depthTest = val;
    if (enabled) glEnable(GL_DEPTH_TEST);
    else         glDisable(GL_DEPTH_TEST);
}

void Renderer::SetDepthFunc(CompareFunc func) {
    glDepthFunc(ToGlCompare(func));
}

void Renderer::SetDepthMask(bool writeEnabled) {
    glDepthMask(writeEnabled ? GL_TRUE : GL_FALSE);
}

void Renderer::SetAlphaTest(bool enabled, CompareFunc func, float ref) {
    if (enabled) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(ToGlCompare(func), ref);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
}

void Renderer::SetCullFace(bool enabled) {
    const int8_t val = enabled ? 1 : 0;
    if (g_stateCache.cullFace == val) {
        return;
    }
    g_stateCache.cullFace = val;
    if (enabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::SetStencilTest(bool enabled) {
    if (enabled) glEnable(GL_STENCIL_TEST);
    else         glDisable(GL_STENCIL_TEST);
}

void Renderer::SetOrtho(float left, float right, float bottom, float top, float nearZ, float farZ) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(left, right, bottom, top, nearZ, farZ);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::SetFrustum(float left, float right, float bottom, float top, float nearZ, float farZ) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(left, right, bottom, top, nearZ, farZ);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::SetPerspective(float fovYDegrees, float aspect, float nearZ, float farZ) {
    const float kDegToRad = 3.14159265f / 180.0f;
    const float halfH = tanf(fovYDegrees * 0.5f * kDegToRad) * nearZ;
    const float halfW = halfH * aspect;
    SetFrustum(-halfW, halfW, -halfH, halfH, nearZ, farZ);
}

void Renderer::ResetModelView() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    g_modelViewDepth = 1;
}

void Renderer::PushMatrix() {
    if (g_modelViewDepth >= 16 && !g_warnedMvOverflow) {
        g_warnedMvOverflow = true;
        ZSDK_LOG_WARN("GFX", "MODELVIEW stack depth > 16 (Zeebo GLES limit)");
    }
    ++g_modelViewDepth;
    glPushMatrix();
}

void Renderer::PopMatrix() {
    if (g_modelViewDepth > 1) {
        --g_modelViewDepth;
    }
    glPopMatrix();
}

void Renderer::LoadMatrix(const float* mat4x4ColumnMajor) {
    if (mat4x4ColumnMajor) {
        glLoadMatrixf(mat4x4ColumnMajor);
    }
}

void Renderer::MultMatrix(const float* mat4x4ColumnMajor) {
    if (mat4x4ColumnMajor) {
        glMultMatrixf(mat4x4ColumnMajor);
    }
}

void Renderer::Translate(float x, float y, float z) {
    glTranslatef(x, y, z);
}

void Renderer::Rotate(float angleDegrees, float x, float y, float z) {
    glRotatef(angleDegrees, x, y, z);
}

void Renderer::Scale(float x, float y, float z) {
    glScalef(x, y, z);
}

void Renderer::SetFog(bool enabled) {
    if (enabled) glEnable(GL_FOG);
    else         glDisable(GL_FOG);
}

void Renderer::ConfigureFog(FogMode mode, const Color& color, float start, float end, float density) {
    const float inv = 1.0f / 255.0f;
    const GLfloat fogColor[4] = {color.r * inv, color.g * inv, color.b * inv, color.a * inv};

    GLfloat glMode = GL_LINEAR;
    if (mode == FogMode::Exp)       glMode = GL_EXP;
    else if (mode == FogMode::Exp2) glMode = GL_EXP2;

    glFogf(GL_FOG_MODE, glMode);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
    glFogf(GL_FOG_DENSITY, density);
}

void Renderer::SetTexture2DEnabled(bool enabled) {
    const int8_t val = enabled ? 1 : 0;
    if (g_stateCache.texture2D == val) {
        return;
    }
    g_stateCache.texture2D = val;
    if (enabled) glEnable(GL_TEXTURE_2D);
    else         glDisable(GL_TEXTURE_2D);
}

void Renderer::SetColor(const Color& color) {
    glColor4ub(color.r, color.g, color.b, color.a);
}

void Renderer::SetLineWidth(float width) {
    glLineWidth(width);
}

void Renderer::SetPointSize(float size) {
    glPointSize(size);
}

void Renderer::BindPositionBuffer(const MeshBuffer& vbo, uint8_t components) {
    glEnableClientState(GL_VERTEX_ARRAY);
    vbo.Bind();
    glVertexPointer(static_cast<GLint>(components), GL_FLOAT, 0, nullptr);
    MeshBuffer::Unbind(BufferType::Vertex);
}

void Renderer::BindNormalBuffer(const MeshBuffer& vbo) {
    glEnableClientState(GL_NORMAL_ARRAY);
    vbo.Bind();
    glNormalPointer(GL_FLOAT, 0, nullptr);
    MeshBuffer::Unbind(BufferType::Vertex);
}

void Renderer::BindColorBuffer(const MeshBuffer& vbo, uint8_t components) {
    glEnableClientState(GL_COLOR_ARRAY);
    vbo.Bind();
    glColorPointer(static_cast<GLint>(components), GL_UNSIGNED_BYTE, 0, nullptr);
    MeshBuffer::Unbind(BufferType::Vertex);
}

void Renderer::BindTexCoordBuffer(const MeshBuffer& vbo, uint8_t components) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    vbo.Bind();
    glTexCoordPointer(static_cast<GLint>(components), GL_FLOAT, 0, nullptr);
    MeshBuffer::Unbind(BufferType::Vertex);
}

void Renderer::UnbindVertexAttributes() {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Renderer::DrawArrays(PrimitiveType type, int32_t firstVertex, int32_t vertexCount) {
    glDrawArrays(ToGlPrimitive(type), static_cast<GLint>(firstVertex),
                 static_cast<GLsizei>(vertexCount));
    debug::DebugOverlay::RecordDrawCall(vertexCount > 0 ? static_cast<uint32_t>(vertexCount) : 0u);
}

void Renderer::DrawElementsU16(PrimitiveType type, int32_t indexCount, const uint16_t* indices) {
    glDrawElements(ToGlPrimitive(type), static_cast<GLsizei>(indexCount),
                   GL_UNSIGNED_SHORT, indices);
    debug::DebugOverlay::RecordDrawCall(indexCount > 0 ? static_cast<uint32_t>(indexCount) : 0u);
}

} // namespace graphics
} // namespace zsdk
