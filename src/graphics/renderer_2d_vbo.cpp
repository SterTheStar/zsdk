/**
 * @file renderer_2d_vbo.cpp
 * @brief Interleaved VBO/raw vertex array binding and 2D HUD/quad rendering.
 */

#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/texture.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/embedded_font.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "gl_platform.hpp"
#include <math.h>

#if defined(__has_include)
    #if __has_include("zsdk/graphics/renderer_types.hpp")
        #include "zsdk/graphics/renderer_types.hpp"
        #define ZSDK_HAS_RENDERER_TYPES_HPP 1
    #endif
#endif

#ifndef ZSDK_HAS_RENDERER_TYPES_HPP
namespace zsdk {
namespace graphics {
struct InterleavedLayout {
    int32_t strideBytes;
    int32_t posOffset;
    uint8_t posComponents;
    int32_t normalOffset;
    int32_t colorOffset;
    uint8_t colorComponents;
    int32_t uvOffset;
    uint8_t uvComponents;
};
} // namespace graphics
} // namespace zsdk
#endif

namespace zsdk {
namespace graphics {

namespace {

static const uint32_t kFontAtlasVramBytes = 128u * 64u;
GLuint g_rendererFontTex = 0u;

void EnsureRendererFontTexture() {
    if (g_rendererFontTex != 0u) {
        return;
    }

    static uint8_t atlas[128 * 64];
    for (int32_t i = 0; i < 128 * 64; ++i) {
        atlas[i] = 0u;
    }

    for (uint32_t c = 0u; c < debug::kFontCharCount; ++c) {
        const uint8_t* glyph = debug::GetEmbeddedFontGlyph(static_cast<char>(0x20u + c));
        const int32_t gx = static_cast<int32_t>(c & 15u) * 8;
        const int32_t gy = static_cast<int32_t>(c >> 4u) * 8;
        for (int32_t row = 0; row < 8; ++row) {
            const uint8_t bits = glyph[row];
            for (int32_t col = 0; col < 8; ++col) {
                atlas[(gy + row) * 128 + (gx + col)] = ((bits & (1u << col)) != 0u) ? 255u : 0u;
            }
        }
    }

    glGenTextures(1, &g_rendererFontTex);
    if (g_rendererFontTex != 0u) {
        glBindTexture(GL_TEXTURE_2D, g_rendererFontTex);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLfloat>(GL_NEAREST));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLfloat>(GL_NEAREST));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLfloat>(GL_CLAMP_TO_EDGE));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLfloat>(GL_CLAMP_TO_EDGE));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 128, 64, 0,
                     GL_ALPHA, GL_UNSIGNED_BYTE, atlas);
        debug::MemoryTracker::RecordVramAlloc(kFontAtlasVramBytes);
    }
}

inline const void* ByteOffsetPtr(int32_t offsetBytes) {
    return reinterpret_cast<const void*>(static_cast<uintptr_t>(offsetBytes));
}

} // namespace

void Renderer::BindInterleavedBuffer(const MeshBuffer& vbo, const InterleavedLayout& layout) {
    if (!vbo.IsValid() || layout.strideBytes < 0) {
        return;
    }

    const GLsizei stride = static_cast<GLsizei>(layout.strideBytes);
    vbo.Bind();

    if (layout.posOffset >= 0 && layout.posComponents > 0u) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(static_cast<GLint>(layout.posComponents), GL_FLOAT,
                        stride, ByteOffsetPtr(layout.posOffset));
    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (layout.normalOffset >= 0) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, ByteOffsetPtr(layout.normalOffset));
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (layout.colorOffset >= 0 && layout.colorComponents > 0u) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(static_cast<GLint>(layout.colorComponents), GL_UNSIGNED_BYTE,
                       stride, ByteOffsetPtr(layout.colorOffset));
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    if (layout.uvOffset >= 0 && layout.uvComponents > 0u) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(static_cast<GLint>(layout.uvComponents), GL_FLOAT,
                          stride, ByteOffsetPtr(layout.uvOffset));
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    MeshBuffer::Unbind(BufferType::Vertex);
}

void Renderer::BindRawVertexArrays(const void* positions, uint8_t posComponents,
                                   const void* normals, const void* colors,
                                   const void* texCoords, int32_t strideBytes) {
    if (strideBytes < 0) {
        return;
    }

    /* Ensure client-side RAM pointers are not interpreted as VBO offsets. */
    MeshBuffer::Unbind(BufferType::Vertex);
    const GLsizei stride = static_cast<GLsizei>(strideBytes);

    if (positions != nullptr && posComponents > 0u) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(static_cast<GLint>(posComponents), GL_FLOAT, stride, positions);
    } else {
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (normals != nullptr) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, normals);
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (colors != nullptr) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, colors);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    if (texCoords != nullptr) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, texCoords);
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}

void Renderer::Begin2D(int32_t viewportWidth, int32_t viewportHeight) {
    EnsureRendererFontTexture();
    MeshBuffer::Unbind(BufferType::Vertex);

    SetViewport(0, 0, viewportWidth, viewportHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0.0f, static_cast<GLfloat>(viewportWidth),
             static_cast<GLfloat>(viewportHeight), 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(GL_MODULATE));
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_VERTEX_ARRAY);
    ResetStateCache();
}

void Renderer::Begin2DVirtual(float virtualWidth, float virtualHeight,
                              int32_t realWidth, int32_t realHeight) {
    EnsureRendererFontTexture();
    MeshBuffer::Unbind(BufferType::Vertex);

    if (realWidth <= 0 || realHeight <= 0 || virtualWidth <= 0.0f || virtualHeight <= 0.0f) {
        Begin2D(realWidth > 0 ? realWidth : 1, realHeight > 0 ? realHeight : 1);
        return;
    }

    const float realAspect = static_cast<float>(realWidth) / static_cast<float>(realHeight);
    const float virtAspect = virtualWidth / virtualHeight;

    int32_t vpX = 0;
    int32_t vpY = 0;
    int32_t vpW = realWidth;
    int32_t vpH = realHeight;

    if (realAspect > virtAspect) {
        vpW = static_cast<int32_t>(static_cast<float>(realHeight) * virtAspect + 0.5f);
        vpX = (realWidth - vpW) / 2;
    } else if (realAspect < virtAspect) {
        vpH = static_cast<int32_t>(static_cast<float>(realWidth) / virtAspect + 0.5f);
        vpY = (realHeight - vpH) / 2;
    }

    SetViewport(vpX, vpY, vpW, vpH);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0.0f, static_cast<GLfloat>(virtualWidth),
             static_cast<GLfloat>(virtualHeight), 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(GL_MODULATE));
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_VERTEX_ARRAY);
    ResetStateCache();
}

void Renderer::End2D() {
    SetTexture2DEnabled(false);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    ResetStateCache();
}

Rect Renderer::GetSafeAreaRect(int32_t viewportWidth, int32_t viewportHeight, int32_t marginPx) {
    if (marginPx < 0) {
        marginPx = 0;
    }
    const int32_t w = (viewportWidth > marginPx * 2) ? (viewportWidth - marginPx * 2) : 0;
    const int32_t h = (viewportHeight > marginPx * 2) ? (viewportHeight - marginPx * 2) : 0;
    return Rect{marginPx, marginPx, w, h};
}

void Renderer::DrawRect2D(float x, float y, float width, float height, const Color& color) {
    const GLfloat verts[8] = {
        x,         y,
        x + width, y,
        x,         y + height,
        x + width, y + height
    };
    SetTexture2DEnabled(false);
    glColor4ub(color.r, color.g, color.b, color.a);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    debug::DebugOverlay::RecordDrawCall(4u);
}

void Renderer::DrawTexturedQuad2D(const Texture& tex, float x, float y, float width, float height,
                                  float u0, float v0, float u1, float v1, const Color& tint) {
    if (!tex.IsValid()) {
        return;
    }

    const GLfloat verts[8] = {
        x,         y,
        x + width, y,
        x,         y + height,
        x + width, y + height
    };
    const GLfloat uvs[8] = {
        u0, v0,
        u1, v0,
        u0, v1,
        u1, v1
    };

    SetTexture2DEnabled(true);
    tex.Bind(0u);
    glColor4ub(tint.r, tint.g, tint.b, tint.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    debug::DebugOverlay::RecordDrawCall(4u);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Renderer::DrawText2D(float x, float y, float scale, const char* text, const Color& color) {
    if (text == nullptr) {
        return;
    }
    if (g_rendererFontTex == 0u) {
        EnsureRendererFontTexture();
        if (g_rendererFontTex == 0u) {
            return;
        }
    }

    static const uint32_t kChunkChars = 16u;
    GLfloat verts[kChunkChars * 12];
    GLfloat uvs[kChunkChars * 12];

    const float adv = 8.0f * scale;
    const float du = 8.0f / 128.0f;
    const float dv = 8.0f / 64.0f;
    float cx = x;

    SetTexture2DEnabled(true);
    glBindTexture(GL_TEXTURE_2D, g_rendererFontTex);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glColor4ub(color.r, color.g, color.b, color.a);

    const char* p = text;
    while (*p != '\0') {
        uint32_t count = 0u;
        while (*p != '\0' && count < kChunkChars) {
            uint8_t uc = static_cast<uint8_t>(*p);
            if (uc < 0x20u || uc > 0x7Fu) {
                uc = 0x20u;
            }
            const uint32_t idx = static_cast<uint32_t>(uc - 0x20u);
            const float u0 = static_cast<float>(idx & 15u) * du;
            const float v0 = static_cast<float>(idx >> 4u) * dv;
            const float u1 = u0 + du;
            const float v1 = v0 + dv;
            const float x0 = cx;
            const float y0 = y;
            const float x1 = cx + adv;
            const float y1 = y + adv;
            const uint32_t vi = count * 12u;

            verts[vi + 0] = x0; verts[vi + 1] = y0;
            verts[vi + 2] = x1; verts[vi + 3] = y0;
            verts[vi + 4] = x0; verts[vi + 5] = y1;
            verts[vi + 6] = x1; verts[vi + 7] = y0;
            verts[vi + 8] = x1; verts[vi + 9] = y1;
            verts[vi + 10] = x0; verts[vi + 11] = y1;

            uvs[vi + 0] = u0; uvs[vi + 1] = v0;
            uvs[vi + 2] = u1; uvs[vi + 3] = v0;
            uvs[vi + 4] = u0; uvs[vi + 5] = v1;
            uvs[vi + 6] = u1; uvs[vi + 7] = v0;
            uvs[vi + 8] = u1; uvs[vi + 9] = v1;
            uvs[vi + 10] = u0; uvs[vi + 11] = v1;

            cx += adv;
            ++count;
            ++p;
        }
        if (count > 0u) {
            glVertexPointer(2, GL_FLOAT, 0, verts);
            glTexCoordPointer(2, GL_FLOAT, 0, uvs);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count * 6u));
            debug::DebugOverlay::RecordDrawCall(count * 6u);
        }
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Renderer::DrawBillboard3D(const Texture& tex, const Vec3& worldPos,
                               float width, float height,
                               const Vec3& cameraRight, const Vec3& cameraUp,
                               float u0, float v0, float u1, float v1, const Color& tint) {
    if (!tex.IsValid()) {
        return;
    }

    const float hw = width * 0.5f;
    const float hh = height * 0.5f;

    const float rx = cameraRight.x * hw;
    const float ry = cameraRight.y * hw;
    const float rz = cameraRight.z * hw;

    const float ux = cameraUp.x * hh;
    const float uy = cameraUp.y * hh;
    const float uz = cameraUp.z * hh;

    const GLfloat verts[12] = {
        worldPos.x - rx + ux, worldPos.y - ry + uy, worldPos.z - rz + uz, /* top-left */
        worldPos.x + rx + ux, worldPos.y + ry + uy, worldPos.z + rz + uz, /* top-right */
        worldPos.x - rx - ux, worldPos.y - ry - uy, worldPos.z - rz - uz, /* bottom-left */
        worldPos.x + rx - ux, worldPos.y + ry - uy, worldPos.z + rz - uz  /* bottom-right */
    };
    const GLfloat uvs[8] = {
        u0, v0,
        u1, v0,
        u0, v1,
        u1, v1
    };

    MeshBuffer::Unbind(BufferType::Vertex);
    SetTexture2DEnabled(true);
    tex.Bind(0u);
    glColor4ub(tint.r, tint.g, tint.b, tint.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    debug::DebugOverlay::RecordDrawCall(4u);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Renderer::DrawBlobShadow(const Vec3& groundPos, float radius, const Color& color) {
    static const int32_t kSegments = 8;
    static const float kTwoPi = 6.2831853f;

    GLfloat verts[(kSegments + 2) * 3];
    GLubyte colors[(kSegments + 2) * 4];

    const float y = groundPos.y + 0.01f;
    verts[0] = groundPos.x;
    verts[1] = y;
    verts[2] = groundPos.z;
    colors[0] = color.r;
    colors[1] = color.g;
    colors[2] = color.b;
    colors[3] = color.a;

    for (int32_t i = 0; i <= kSegments; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(kSegments)) * kTwoPi;
        const int32_t vi = (i + 1) * 3;
        const int32_t ci = (i + 1) * 4;
        verts[vi + 0] = groundPos.x + cosf(angle) * radius;
        verts[vi + 1] = y;
        verts[vi + 2] = groundPos.z + sinf(angle) * radius;
        colors[ci + 0] = color.r;
        colors[ci + 1] = color.g;
        colors[ci + 2] = color.b;
        colors[ci + 3] = 0u;
    }

    MeshBuffer::Unbind(BufferType::Vertex);
    SetTexture2DEnabled(false);
    SetBlendMode(BlendMode::Alpha);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    glDrawArrays(GL_TRIANGLE_FAN, 0, kSegments + 2);
    debug::DebugOverlay::RecordDrawCall(static_cast<uint32_t>(kSegments + 2));
    glDisableClientState(GL_COLOR_ARRAY);
}

uint32_t Renderer::GetSharedFontTextureId() {
    EnsureRendererFontTexture();
    return static_cast<uint32_t>(g_rendererFontTex);
}

void Renderer::ReleaseSharedFontTexture() {
    if (g_rendererFontTex != 0u) {
        glDeleteTextures(1, &g_rendererFontTex);
        debug::MemoryTracker::RecordVramFree(kFontAtlasVramBytes);
        g_rendererFontTex = 0u;
    }
}

} // namespace graphics
} // namespace zsdk
