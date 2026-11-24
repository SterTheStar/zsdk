/**
 * @file vertex_formats.hpp
 * @brief Standard cache-friendly interleaved vertex structs for OpenGL ES 1.1.
 *
 * Each struct is tightly packed at 4-byte alignment and provides a static
 * InterleavedLayout() helper for zero-overhead VBO binding.
 */

#ifndef ZSDK_GRAPHICS_VERTEX_FORMATS_HPP
#define ZSDK_GRAPHICS_VERTEX_FORMATS_HPP

#include "zsdk/graphics/renderer_types.hpp"
#include <stddef.h>

namespace zsdk {
namespace graphics {

/**
 * @brief Interleaved vertex with 3D position, 3D normal, and RGBA8 color (28 bytes).
 */
struct VertexP3N3C4 {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    /** @brief Returns the InterleavedLayout descriptor for VertexP3N3C4. */
    static ZSDK_FORCE_INLINE zsdk::graphics::InterleavedLayout InterleavedLayout() {
        zsdk::graphics::InterleavedLayout layout;
        layout.strideBytes     = static_cast<int32_t>(sizeof(VertexP3N3C4));
        layout.posOffset       = static_cast<int32_t>(offsetof(VertexP3N3C4, x));
        layout.posComponents   = 3u;
        layout.normalOffset    = static_cast<int32_t>(offsetof(VertexP3N3C4, nx));
        layout.colorOffset     = static_cast<int32_t>(offsetof(VertexP3N3C4, r));
        layout.colorComponents = 4u;
        layout.uvOffset        = -1;
        layout.uvComponents    = 0u;
        return layout;
    }
};

static_assert(sizeof(VertexP3N3C4) == 28, "VertexP3N3C4 must be tightly packed (28 bytes)");

/**
 * @brief Interleaved vertex with 3D position, 3D normal, and 2D texture coordinates (32 bytes).
 */
struct VertexP3N3U2 {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float u;
    float v;

    /** @brief Returns the InterleavedLayout descriptor for VertexP3N3U2. */
    static ZSDK_FORCE_INLINE zsdk::graphics::InterleavedLayout InterleavedLayout() {
        zsdk::graphics::InterleavedLayout layout;
        layout.strideBytes     = static_cast<int32_t>(sizeof(VertexP3N3U2));
        layout.posOffset       = static_cast<int32_t>(offsetof(VertexP3N3U2, x));
        layout.posComponents   = 3u;
        layout.normalOffset    = static_cast<int32_t>(offsetof(VertexP3N3U2, nx));
        layout.colorOffset     = -1;
        layout.colorComponents = 0u;
        layout.uvOffset        = static_cast<int32_t>(offsetof(VertexP3N3U2, u));
        layout.uvComponents    = 2u;
        return layout;
    }
};

static_assert(sizeof(VertexP3N3U2) == 32, "VertexP3N3U2 must be tightly packed (32 bytes)");

/**
 * @brief Interleaved vertex with 3D position, RGBA8 color, and 2D texture coordinates (24 bytes).
 */
struct VertexP3C4U2 {
    float x;
    float y;
    float z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    float u;
    float v;

    /** @brief Returns the InterleavedLayout descriptor for VertexP3C4U2. */
    static ZSDK_FORCE_INLINE zsdk::graphics::InterleavedLayout InterleavedLayout() {
        zsdk::graphics::InterleavedLayout layout;
        layout.strideBytes     = static_cast<int32_t>(sizeof(VertexP3C4U2));
        layout.posOffset       = static_cast<int32_t>(offsetof(VertexP3C4U2, x));
        layout.posComponents   = 3u;
        layout.normalOffset    = -1;
        layout.colorOffset     = static_cast<int32_t>(offsetof(VertexP3C4U2, r));
        layout.colorComponents = 4u;
        layout.uvOffset        = static_cast<int32_t>(offsetof(VertexP3C4U2, u));
        layout.uvComponents    = 2u;
        return layout;
    }
};

static_assert(sizeof(VertexP3C4U2) == 24, "VertexP3C4U2 must be tightly packed (24 bytes)");

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_VERTEX_FORMATS_HPP */
