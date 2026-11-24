/**
 * @file image_decoder.hpp
 * @brief Hardware/BREW image decoding (PNG, JPEG, BMP) into raw pixel buffers.
 *
 * Wraps Qualcomm BREW IImageDecoder / IForceFeed and IImage / IBitmap / IDIB
 * to decode compressed image assets into GPU-ready pixel buffers.
 */

#ifndef ZSDK_GRAPHICS_IMAGE_DECODER_HPP
#define ZSDK_GRAPHICS_IMAGE_DECODER_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/graphics/texture.hpp"

namespace zsdk {
namespace graphics {

/**
 * @brief Supported encoded image container formats.
 */
enum class ImageFormat : uint8_t {
    Auto = 0,   /**< Detect format from magic header bytes */
    Png,        /**< PNG (AEECLSID_PNGDecoder) */
    Jpeg,       /**< JPEG (AEECLSID_JPEGDecoder) */
    Bmp         /**< BMP (AEECLSID_BMPDecoder) */
};

/**
 * @brief Decoded pixel buffer ready for Texture::Create() or CPU blitting.
 *
 * Pixel memory is allocated via debug::MemoryTracker under MemorySubsystem::Graphics.
 * Release with ImageDecoder::FreeImage().
 */
struct DecodedImage {
    uint8_t*      pixels;
    uint16_t      width;
    uint16_t      height;
    uint32_t      dataSize;
    TextureFormat format;
    bool          hasAlpha;
};

/**
 * @brief Decodes PNG, JPEG, and BMP images into raw pixel buffers using BREW decoders.
 */
class ImageDecoder : public NonCopyable {
public:
    ImageDecoder();
    ~ImageDecoder();

    /**
     * @brief Initializes the decoder with the BREW IShell* platform context.
     * @param platformContext Opaque IShell* (from Application::GetNativePlatformContext()).
     * @return Result::Ok on success, or an error code.
     */
    Result Initialize(void* platformContext);

    /** @brief Releases internal references. */
    void Shutdown();

    /**
     * @brief Decodes an encoded image from memory into @p outImage.
     * @param data Pointer to encoded PNG/JPEG/BMP bytes.
     * @param size Size of @p data in bytes.
     * @param outImage Receives the allocated pixel buffer and metadata.
     * @param format Container format or ImageFormat::Auto to sniff magic bytes.
     * @param desiredFormat Output pixel format (RGBA8888, RGB888, or RGB565).
     * @return Result::Ok on success, or an error code.
     */
    Result DecodeMemory(const void* data, uint32_t size, DecodedImage& outImage,
                        ImageFormat format = ImageFormat::Auto,
                        TextureFormat desiredFormat = TextureFormat::RGBA8888);

    /**
     * @brief Decodes an image file from the BREW filesystem using IImage.
     * @param filePath Null-terminated path to the image file.
     * @param outImage Receives the allocated pixel buffer and metadata.
     * @param desiredFormat Output pixel format (RGBA8888, RGB888, or RGB565).
     * @return Result::Ok on success, or an error code.
     */
    Result DecodeFile(const char* filePath, DecodedImage& outImage,
                      TextureFormat desiredFormat = TextureFormat::RGBA8888);

    /** @brief Frees a pixel buffer allocated by DecodeMemory() or DecodeFile(). */
    static void FreeImage(DecodedImage& image);

    /** @brief Sniffs ImageFormat from magic header bytes. */
    static ImageFormat DetectFormat(const void* data, uint32_t size);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_IMAGE_DECODER_HPP */
