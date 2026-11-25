/**
 * @file texture.hpp
 * @brief RAII OpenGL ES 1.1 Texture with automatic VRAM accounting.
 */

#ifndef ZSDK_GRAPHICS_TEXTURE_HPP
#define ZSDK_GRAPHICS_TEXTURE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

enum class TextureFormat : uint8_t {
    Alpha8 = 0,     /**< GL_ALPHA, 1 byte/px */
    RGB565,         /**< GL_RGB + GL_UNSIGNED_SHORT_5_6_5, 2 bytes/px */
    RGBA4444,       /**< GL_RGBA + GL_UNSIGNED_SHORT_4_4_4_4, 2 bytes/px */
    RGBA5551,       /**< GL_RGBA + GL_UNSIGNED_SHORT_5_5_5_1, 2 bytes/px */
    RGB888,         /**< GL_RGB + GL_UNSIGNED_BYTE, 3 bytes/px */
    RGBA8888        /**< GL_RGBA + GL_UNSIGNED_BYTE, 4 bytes/px */
};

enum class TextureFilter : uint8_t {
    Nearest = 0,
    Linear
};

/**
 * @brief Lightweight RAII OpenGL ES texture handle.
 *
 * Automatically reports allocated/freed VRAM bytes to debug::MemoryTracker.
 */
class Texture : public NonCopyable {
public:
    Texture();
    ~Texture();

    /** @brief Uploads pixel data to GPU and records VRAM usage. */
    Result Create(uint16_t width, uint16_t height, TextureFormat format,
                  const void* pixelData, TextureFilter filter = TextureFilter::Linear);

    /**
     * @brief Uploads compressed texture data (e.g. ATI_TC) via glCompressedTexImage2D.
     * @param width Texture width in pixels.
     * @param height Texture height in pixels.
     * @param glInternalFormat Native GL compressed internal format enum.
     * @param imageSize Size of @p compressedData in bytes.
     * @param compressedData Pointer to compressed mip-0 data.
     * @param filter Minification/magnification filter.
     */
    Result CreateCompressed(uint16_t width, uint16_t height, uint32_t glInternalFormat,
                            uint32_t imageSize, const void* compressedData,
                            TextureFilter filter = TextureFilter::Linear);

    /**
     * @brief Creates a texture from an in-memory ATITC file (header + 4x4-block data), the
     *        Adreno 130's native compressed format (GL_ATI_texture_compression_atitc).
     *
     * Header (5 x uint32, little endian): signature 0xCCC40002, width, height, flags
     * (1 = RGB, 2 = RGBA, 4 = tiled), dataOffset. Tiled files are rejected (NotSupported).
     * Check GLContext::HasExtension("GL_ATI_texture_compression_atitc") first and fall back
     * to Create() when it returns false.
     */
    Result CreateFromAtitc(const void* fileData, uint32_t fileSize,
                           TextureFilter filter = TextureFilter::Linear);

    /** @brief Updates a sub-rectangle of an existing texture without reallocating VRAM (uncompressed textures only). */
    Result UpdateSubImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                          const void* pixelData);

    /**
     * @brief Generates mipmaps for this texture (glGenerateMipmapOES / GL_GENERATE_MIPMAP)
     *        and updates minification filter to mipmap filtering.
     */
    Result GenerateMipmaps(TextureFilter filter = TextureFilter::Linear);

    /**
     * @brief Reads pixels from the active framebuffer via glReadPixels.
     * @param x Bottom-left X coordinate in framebuffer pixels.
     * @param y Bottom-left Y coordinate in framebuffer pixels.
     * @param width Width of region to read.
     * @param height Height of region to read.
     * @param format Destination pixel format.
     * @param outPixels Caller-allocated buffer large enough for width * height * bpp.
     */
    static Result ReadFramebuffer(int32_t x, int32_t y, uint16_t width, uint16_t height,
                                  TextureFormat format, void* outPixels);

    /** @brief Deletes the GL texture and subtracts its VRAM from MemoryTracker. */
    void Destroy();

    /** @brief Binds this texture to GL_TEXTURE_2D (or unbinds if invalid). */
    void Bind(uint8_t textureUnit = 0) const;

    ZSDK_FORCE_INLINE uint32_t GetNativeId() const { return m_glId; }
    ZSDK_FORCE_INLINE uint16_t GetWidth() const { return m_width; }
    ZSDK_FORCE_INLINE uint16_t GetHeight() const { return m_height; }
    ZSDK_FORCE_INLINE uint32_t GetVramBytes() const { return m_vramBytes; }
    ZSDK_FORCE_INLINE bool     IsValid() const { return m_glId != 0u; }

private:
    uint32_t      m_glId;
    uint16_t      m_width;
    uint16_t      m_height;
    TextureFormat m_format;
    uint32_t      m_vramBytes;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_TEXTURE_HPP */
