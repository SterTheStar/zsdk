/**
 * @file sprite_batch.cpp
 * @brief Zero-allocation 2D Sprite & Rect Batcher optimized for 32 MB RAM / ARM11.
 */

#include "zsdk/graphics/sprite_batch.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/texture.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "gl_platform.hpp"
#include <math.h>

namespace zsdk {
namespace graphics {

SpriteBatch2D::SpriteBatch2D()
    : m_vertices(nullptr)
    , m_indices(nullptr)
    , m_maxSprites(0u)
    , m_queuedSprites(0u)
    , m_currentTexId(0u)
    , m_inPass(false) {
}

SpriteBatch2D::~SpriteBatch2D() {
    Shutdown();
}

Result SpriteBatch2D::Initialize(uint16_t maxSprites) {
    if (m_vertices != nullptr) {
        Shutdown();
    }
    if (maxSprites == 0u) {
        maxSprites = 128u;
    }

    const uint32_t vertexBytes = static_cast<uint32_t>(maxSprites) * 4u * static_cast<uint32_t>(sizeof(VertexP3C4U2));
    const uint32_t indexBytes  = static_cast<uint32_t>(maxSprites) * 6u * static_cast<uint32_t>(sizeof(uint16_t));

    m_vertices = static_cast<VertexP3C4U2*>(
        debug::MemoryTracker::Alloc(debug::MemorySubsystem::Graphics, vertexBytes));
    m_indices = static_cast<uint16_t*>(
        debug::MemoryTracker::Alloc(debug::MemorySubsystem::Graphics, indexBytes));

    if (m_vertices == nullptr || m_indices == nullptr) {
        Shutdown();
        return Result::OutOfMemory;
    }

    for (uint16_t i = 0u; i < maxSprites; ++i) {
        const uint16_t vBase = static_cast<uint16_t>(i * 4u);
        const uint32_t iBase = static_cast<uint32_t>(i) * 6u;
        m_indices[iBase + 0u] = vBase;
        m_indices[iBase + 1u] = static_cast<uint16_t>(vBase + 1u);
        m_indices[iBase + 2u] = static_cast<uint16_t>(vBase + 2u);
        m_indices[iBase + 3u] = static_cast<uint16_t>(vBase + 2u);
        m_indices[iBase + 4u] = static_cast<uint16_t>(vBase + 1u);
        m_indices[iBase + 5u] = static_cast<uint16_t>(vBase + 3u);
    }

    m_maxSprites    = maxSprites;
    m_queuedSprites = 0u;
    m_currentTexId  = 0u;
    m_inPass        = false;
    return Result::Ok;
}

void SpriteBatch2D::Shutdown() {
    if (m_vertices != nullptr) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_vertices);
        m_vertices = nullptr;
    }
    if (m_indices != nullptr) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_indices);
        m_indices = nullptr;
    }
    m_maxSprites    = 0u;
    m_queuedSprites = 0u;
    m_currentTexId  = 0u;
    m_inPass        = false;
}

void SpriteBatch2D::Begin(int32_t viewportWidth, int32_t viewportHeight) {
    Renderer::Begin2D(viewportWidth, viewportHeight);
    m_queuedSprites = 0u;
    m_currentTexId  = 0u;
    m_inPass        = true;
}

void SpriteBatch2D::BeginVirtual(float virtualWidth, float virtualHeight,
                                  int32_t realWidth, int32_t realHeight) {
    Renderer::Begin2DVirtual(virtualWidth, virtualHeight, realWidth, realHeight);
    m_queuedSprites = 0u;
    m_currentTexId  = 0u;
    m_inPass        = true;
}

void SpriteBatch2D::End() {
    if (!m_inPass) {
        return;
    }
    Flush();
    m_inPass = false;
    Renderer::End2D();
}

void SpriteBatch2D::Flush() {
    if (m_queuedSprites == 0u || m_vertices == nullptr || m_indices == nullptr) {
        return;
    }

    MeshBuffer::Unbind(BufferType::Vertex);
    MeshBuffer::Unbind(BufferType::Index);

    if (m_currentTexId != 0u) {
        Renderer::SetTexture2DEnabled(true);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_currentTexId));
    } else {
        Renderer::SetTexture2DEnabled(false);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    const GLsizei stride = static_cast<GLsizei>(sizeof(VertexP3C4U2));
    glVertexPointer(3, GL_FLOAT, stride, &m_vertices[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, &m_vertices[0].r);
    glTexCoordPointer(2, GL_FLOAT, stride, &m_vertices[0].u);

    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_queuedSprites) * 6,
                   GL_UNSIGNED_SHORT,
                   m_indices);
    debug::DebugOverlay::RecordDrawCall(static_cast<uint32_t>(m_queuedSprites) * 6u);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    m_queuedSprites = 0u;
}

void SpriteBatch2D::DrawQuad(const Texture* tex, float x, float y, float width, float height,
                             float u0, float v0, float u1, float v1, const Color& tint) {
    if (!m_inPass || m_vertices == nullptr) {
        return;
    }

    const uint32_t texId = (tex != nullptr && tex->IsValid()) ? tex->GetNativeId() : 0u;
    if ((m_queuedSprites > 0u && texId != m_currentTexId) || m_queuedSprites >= m_maxSprites) {
        Flush();
    }
    m_currentTexId = texId;

    VertexP3C4U2* v = m_vertices + static_cast<uint32_t>(m_queuedSprites) * 4u;
    const float x1 = x + width;
    const float y1 = y + height;

    v[0].x = x;  v[0].y = y;  v[0].z = 0.0f; v[0].r = tint.r; v[0].g = tint.g; v[0].b = tint.b; v[0].a = tint.a; v[0].u = u0; v[0].v = v0;
    v[1].x = x1; v[1].y = y;  v[1].z = 0.0f; v[1].r = tint.r; v[1].g = tint.g; v[1].b = tint.b; v[1].a = tint.a; v[1].u = u1; v[1].v = v0;
    v[2].x = x;  v[2].y = y1; v[2].z = 0.0f; v[2].r = tint.r; v[2].g = tint.g; v[2].b = tint.b; v[2].a = tint.a; v[2].u = u0; v[2].v = v1;
    v[3].x = x1; v[3].y = y1; v[3].z = 0.0f; v[3].r = tint.r; v[3].g = tint.g; v[3].b = tint.b; v[3].a = tint.a; v[3].u = u1; v[3].v = v1;

    ++m_queuedSprites;
}

void SpriteBatch2D::DrawRotated(const Texture* tex, float centerX, float centerY,
                                float width, float height, float angleRad,
                                float u0, float v0, float u1, float v1, const Color& tint) {
    if (!m_inPass || m_vertices == nullptr) {
        return;
    }

    const uint32_t texId = (tex != nullptr && tex->IsValid()) ? tex->GetNativeId() : 0u;
    if ((m_queuedSprites > 0u && texId != m_currentTexId) || m_queuedSprites >= m_maxSprites) {
        Flush();
    }
    m_currentTexId = texId;

    const float hw = width * 0.5f;
    const float hh = height * 0.5f;
    const float c = cosf(angleRad);
    const float s = sinf(angleRad);

    const float dxC = hw * c;
    const float dxS = hw * s;
    const float dyC = hh * c;
    const float dyS = hh * s;

    VertexP3C4U2* v = m_vertices + static_cast<uint32_t>(m_queuedSprites) * 4u;

    /* (-hw, -hh), (+hw, -hh), (-hw, +hh), (+hw, +hh) rotated by angleRad */
    v[0].x = centerX - dxC + dyS; v[0].y = centerY - dxS - dyC;
    v[1].x = centerX + dxC + dyS; v[1].y = centerY + dxS - dyC;
    v[2].x = centerX - dxC - dyS; v[2].y = centerY - dxS + dyC;
    v[3].x = centerX + dxC - dyS; v[3].y = centerY + dxS + dyC;

    v[0].z = 0.0f; v[0].r = tint.r; v[0].g = tint.g; v[0].b = tint.b; v[0].a = tint.a; v[0].u = u0; v[0].v = v0;
    v[1].z = 0.0f; v[1].r = tint.r; v[1].g = tint.g; v[1].b = tint.b; v[1].a = tint.a; v[1].u = u1; v[1].v = v0;
    v[2].z = 0.0f; v[2].r = tint.r; v[2].g = tint.g; v[2].b = tint.b; v[2].a = tint.a; v[2].u = u0; v[2].v = v1;
    v[3].z = 0.0f; v[3].r = tint.r; v[3].g = tint.g; v[3].b = tint.b; v[3].a = tint.a; v[3].u = u1; v[3].v = v1;

    ++m_queuedSprites;
}

void SpriteBatch2D::DrawRect(float x, float y, float width, float height, const Color& color) {
    DrawQuad(nullptr, x, y, width, height, 0.0f, 0.0f, 0.0f, 0.0f, color);
}

} // namespace graphics
} // namespace zsdk
