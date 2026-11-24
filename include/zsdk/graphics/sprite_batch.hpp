/**
 * @file sprite_batch.hpp
 * @brief Zero-allocation 2D Sprite & Rect Batcher optimized for 32 MB RAM / ARM11.
 *
 * Accumulates quads sharing the same Texture and submits them in a single
 * glDrawElements() call on Flush() / End(), drastically reducing CPU draw-call overhead.
 */

#ifndef ZSDK_GRAPHICS_SPRITE_BATCH_HPP
#define ZSDK_GRAPHICS_SPRITE_BATCH_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/graphics/vertex_formats.hpp"

namespace zsdk {
namespace graphics {

class Texture;

/**
 * @brief Lightweight 2D batcher with a single bounded allocation on Initialize().
 */
class SpriteBatch2D : public NonCopyable {
public:
    SpriteBatch2D();
    ~SpriteBatch2D();

    /**
     * @brief Allocates vertex/index buffers for up to @p maxSprites (default 128 = ~13.5 KB RAM).
     */
    Result Initialize(uint16_t maxSprites = 128u);
    void   Shutdown();

    void Begin(int32_t viewportWidth, int32_t viewportHeight);
    void BeginVirtual(float virtualWidth, float virtualHeight, int32_t realWidth, int32_t realHeight);
    void End();

    /** @brief Submits all queued quads immediately without ending the 2D pass. */
    void Flush();

    void DrawQuad(const Texture* tex, float x, float y, float width, float height,
                  float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f,
                  const Color& tint = Color::Rgba(255, 255, 255, 255));

    void DrawRotated(const Texture* tex, float centerX, float centerY,
                     float width, float height, float angleRad,
                     float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f,
                     const Color& tint = Color::Rgba(255, 255, 255, 255));

    void DrawRect(float x, float y, float width, float height, const Color& color);

private:
    VertexP3C4U2* m_vertices;
    uint16_t*     m_indices;
    uint16_t      m_maxSprites;
    uint16_t      m_queuedSprites;
    uint32_t      m_currentTexId;
    bool          m_inPass;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_SPRITE_BATCH_HPP */
