/**
 * @file zeebo_image_decoder.cpp
 * @brief Qualcomm BREW / Zeebo IImageDecoder & IImage pixel-buffer decoder.
 */

#include "zsdk/graphics/image_decoder.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEImage.h"
#include "AEEBitmap.h"
#include "AEEIDIB.h"
#include "IImageDecoder.h"
#include "IForceFeed.h"
#include "BMPDecoder.bid"
#include "JPEGDecoder.bid"
#include "PNGDecoder.bid"

namespace zsdk {
namespace graphics {

namespace {

AEECLSID MapDecoderClsid(ImageFormat fmt) {
    switch (fmt) {
        case ImageFormat::Png:  return AEECLSID_PNGDecoder;
        case ImageFormat::Jpeg: return AEECLSID_JPEGDecoder;
        case ImageFormat::Bmp:  return AEECLSID_BMPDecoder;
        case ImageFormat::Auto:
        default:                return 0u;
    }
}

uint32_t BytesPerPixel(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::RGB565:
        case TextureFormat::RGBA4444:
        case TextureFormat::RGBA5551:
            return 2u;
        case TextureFormat::RGB888:
            return 3u;
        case TextureFormat::Alpha8:
            return 1u;
        case TextureFormat::RGBA8888:
        default:
            return 4u;
    }
}

void WritePixel(uint8_t* dst, TextureFormat fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    switch (fmt) {
        case TextureFormat::RGB565: {
            const uint16_t v = static_cast<uint16_t>(
                ((static_cast<uint16_t>(r) & 0xF8u) << 8) |
                ((static_cast<uint16_t>(g) & 0xFCu) << 3) |
                (static_cast<uint16_t>(b) >> 3));
            dst[0] = static_cast<uint8_t>(v & 0xFFu);
            dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
            break;
        }
        case TextureFormat::RGBA4444: {
            const uint16_t v = static_cast<uint16_t>(
                ((static_cast<uint16_t>(r) & 0xF0u) << 8) |
                ((static_cast<uint16_t>(g) & 0xF0u) << 4) |
                (static_cast<uint16_t>(b) & 0xF0u) |
                (static_cast<uint16_t>(a) >> 4));
            dst[0] = static_cast<uint8_t>(v & 0xFFu);
            dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
            break;
        }
        case TextureFormat::RGBA5551: {
            const uint16_t v = static_cast<uint16_t>(
                ((static_cast<uint16_t>(r) & 0xF8u) << 8) |
                ((static_cast<uint16_t>(g) & 0xF8u) << 3) |
                ((static_cast<uint16_t>(b) & 0xF8u) >> 2) |
                ((a >= 128u) ? 1u : 0u));
            dst[0] = static_cast<uint8_t>(v & 0xFFu);
            dst[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
            break;
        }
        case TextureFormat::RGB888:
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            break;
        case TextureFormat::Alpha8:
            dst[0] = a;
            break;
        case TextureFormat::RGBA8888:
        default:
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst[3] = a;
            break;
    }
}

void UnpackDibPixel(const IDIB* dib, const uint8_t* row, uint16_t x,
                    uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a, bool hasTransparentColor) {
    a = 255u;
    switch (dib->nDepth) {
        case 8u: {
            const uint8_t idx = row[x];
            if (dib->pRGB && idx < dib->cntRGB) {
                const uint32_t pal = dib->pRGB[idx];
                // IDIB palette entries are 0x00RRGGBB
                r = static_cast<uint8_t>((pal >> 16) & 0xFFu);
                g = static_cast<uint8_t>((pal >> 8) & 0xFFu);
                b = static_cast<uint8_t>(pal & 0xFFu);
            } else {
                r = idx; g = idx; b = idx;
            }
            if (hasTransparentColor && static_cast<NativeColor>(idx) == dib->ncTransparent) {
                a = 0u;
            }
            break;
        }
        case 16u: {
            const uint16_t px = static_cast<uint16_t>(row[x * 2u]) |
                                (static_cast<uint16_t>(row[x * 2u + 1u]) << 8);
            if (dib->nColorScheme == IDIB_COLORSCHEME_555) {
                r = static_cast<uint8_t>(((px >> 10) & 0x1Fu) * 255u / 31u);
                g = static_cast<uint8_t>(((px >> 5) & 0x1Fu) * 255u / 31u);
                b = static_cast<uint8_t>((px & 0x1Fu) * 255u / 31u);
            } else if (dib->nColorScheme == IDIB_COLORSCHEME_444) {
                r = static_cast<uint8_t>(((px >> 8) & 0x0Fu) * 17u);
                g = static_cast<uint8_t>(((px >> 4) & 0x0Fu) * 17u);
                b = static_cast<uint8_t>((px & 0x0Fu) * 17u);
                a = static_cast<uint8_t>(((px >> 12) & 0x0Fu) * 17u);
            } else {
                // Default 565
                r = static_cast<uint8_t>(((px >> 11) & 0x1Fu) * 255u / 31u);
                g = static_cast<uint8_t>(((px >> 5) & 0x3Fu) * 255u / 63u);
                b = static_cast<uint8_t>((px & 0x1Fu) * 255u / 31u);
            }
            if (hasTransparentColor && static_cast<NativeColor>(px) == dib->ncTransparent) {
                a = 0u;
            }
            break;
        }
        case 24u: {
            const uint8_t* p = row + x * 3u;
            b = p[0];
            g = p[1];
            r = p[2];
            const NativeColor nc = static_cast<NativeColor>(b) |
                                   (static_cast<NativeColor>(g) << 8) |
                                   (static_cast<NativeColor>(r) << 16);
            if (hasTransparentColor && nc == dib->ncTransparent) {
                a = 0u;
            }
            break;
        }
        case 32u:
        default: {
            const uint8_t* p = row + x * 4u;
            b = p[0];
            g = p[1];
            r = p[2];
            a = (dib->nColorScheme == IDIB_COLORSCHEME_888) ? 255u : p[3];
            if (hasTransparentColor) {
                const NativeColor nc = static_cast<NativeColor>(p[0]) |
                                       (static_cast<NativeColor>(p[1]) << 8) |
                                       (static_cast<NativeColor>(p[2]) << 16) |
                                       (static_cast<NativeColor>(p[3]) << 24);
                if (nc == dib->ncTransparent) {
                    a = 0u;
                }
            }
            break;
        }
    }
}

Result ExtractBitmapPixels(IBitmap* bitmap, bool hasTransparentRop,
                           TextureFormat desiredFormat, DecodedImage& outImage) {
    if (!bitmap) {
        return Result::InvalidArg;
    }

    IDIB* dib = nullptr;
    if (IBITMAP_QueryInterface(bitmap, AEEIID_IDIB, reinterpret_cast<void**>(&dib)) == AEE_SUCCESS && dib) {
        const uint16_t w = dib->cx;
        const uint16_t h = dib->cy;
        if (w == 0u || h == 0u || !dib->pBmp) {
            IDIB_Release(dib);
            return Result::GraphicsError;
        }

        const uint32_t dstBpp = BytesPerPixel(desiredFormat);
        const uint32_t dataSize = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * dstBpp;
        uint8_t* dst = static_cast<uint8_t*>(
            debug::MemoryTracker::Alloc(debug::MemorySubsystem::Graphics, dataSize));
        if (!dst) {
            IDIB_Release(dib);
            return Result::OutOfMemory;
        }

        bool anyAlpha = false;
        const uint8_t* base = dib->pBmp;
        const int32_t pitch = static_cast<int32_t>(dib->nPitch);

        for (uint16_t y = 0u; y < h; ++y) {
            const uint8_t* srcRow = base + static_cast<int32_t>(y) * pitch;
            uint8_t* dstRow = dst + static_cast<uint32_t>(y) * static_cast<uint32_t>(w) * dstBpp;
            for (uint16_t x = 0u; x < w; ++x) {
                uint8_t r = 0u, g = 0u, b = 0u, a = 255u;
                UnpackDibPixel(dib, srcRow, x, r, g, b, a, hasTransparentRop);
                if (a < 255u) {
                    anyAlpha = true;
                }
                WritePixel(dstRow + static_cast<uint32_t>(x) * dstBpp, desiredFormat, r, g, b, a);
            }
        }

        IDIB_Release(dib);
        outImage.pixels   = dst;
        outImage.width    = w;
        outImage.height   = h;
        outImage.dataSize = dataSize;
        outImage.format   = desiredFormat;
        outImage.hasAlpha = anyAlpha;
        return Result::Ok;
    }

    // Fallback: IBitmap::GetPixel / NativeToRGB
    AEEBitmapInfo info;
    MEMSET(&info, 0, sizeof(info));
    if (IBITMAP_GetInfo(bitmap, &info, sizeof(info)) != AEE_SUCCESS ||
        info.cx == 0u || info.cy == 0u) {
        return Result::GraphicsError;
    }

    NativeColor transparentColor = 0u;
    const bool checkTransparent = hasTransparentRop &&
        (IBITMAP_GetTransparencyColor(bitmap, &transparentColor) == AEE_SUCCESS);

    const uint16_t w = static_cast<uint16_t>(info.cx);
    const uint16_t h = static_cast<uint16_t>(info.cy);
    const uint32_t dstBpp = BytesPerPixel(desiredFormat);
    const uint32_t dataSize = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * dstBpp;
    uint8_t* dst = static_cast<uint8_t*>(
        debug::MemoryTracker::Alloc(debug::MemorySubsystem::Graphics, dataSize));
    if (!dst) {
        return Result::OutOfMemory;
    }

    bool anyAlpha = false;
    for (uint16_t y = 0u; y < h; ++y) {
        uint8_t* dstRow = dst + static_cast<uint32_t>(y) * static_cast<uint32_t>(w) * dstBpp;
        for (uint16_t x = 0u; x < w; ++x) {
            NativeColor nc = 0u;
            IBITMAP_GetPixel(bitmap, x, y, &nc);
            const RGBVAL rgb = IBITMAP_NativeToRGB(bitmap, nc);
            const uint8_t r = static_cast<uint8_t>((rgb >> 8) & 0xFFu);
            const uint8_t g = static_cast<uint8_t>((rgb >> 16) & 0xFFu);
            const uint8_t b = static_cast<uint8_t>((rgb >> 24) & 0xFFu);
            uint8_t a = 255u;
            if (checkTransparent && nc == transparentColor) {
                a = 0u;
                anyAlpha = true;
            }
            WritePixel(dstRow + static_cast<uint32_t>(x) * dstBpp, desiredFormat, r, g, b, a);
        }
    }

    outImage.pixels   = dst;
    outImage.width    = w;
    outImage.height   = h;
    outImage.dataSize = dataSize;
    outImage.format   = desiredFormat;
    outImage.hasAlpha = anyAlpha;
    return Result::Ok;
}

} // namespace

struct ImageDecoder::Impl {
    IShell* shell;
};

ImageDecoder::ImageDecoder()
    : m_impl(nullptr) {
}

ImageDecoder::~ImageDecoder() {
    Shutdown();
}

Result ImageDecoder::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Graphics, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->shell = shell;
    return Result::Ok;
}

void ImageDecoder::Shutdown() {
    if (!m_impl) {
        return;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

ImageFormat ImageDecoder::DetectFormat(const void* data, uint32_t size) {
    if (!data || size < 2u) {
        return ImageFormat::Auto;
    }
    const uint8_t* b = static_cast<const uint8_t*>(data);

    // PNG: 89 50 4E 47
    if (size >= 4u && b[0] == 0x89u && b[1] == 0x50u && b[2] == 0x4Eu && b[3] == 0x47u) {
        return ImageFormat::Png;
    }
    // JPEG: FF D8 FF
    if (size >= 3u && b[0] == 0xFFu && b[1] == 0xD8u && b[2] == 0xFFu) {
        return ImageFormat::Jpeg;
    }
    // BMP: 'B' 'M'
    if (b[0] == 0x42u && b[1] == 0x4Du) {
        return ImageFormat::Bmp;
    }
    return ImageFormat::Auto;
}

Result ImageDecoder::DecodeMemory(const void* data, uint32_t size, DecodedImage& outImage,
                                  ImageFormat format, TextureFormat desiredFormat) {
    MEMSET(&outImage, 0, sizeof(outImage));

    if (!m_impl || !m_impl->shell) {
        return Result::NotInitialized;
    }
    if (!data || size == 0u) {
        return Result::InvalidArg;
    }

    const ImageFormat resolved = (format == ImageFormat::Auto)
        ? DetectFormat(data, size)
        : format;
    const AEECLSID clsid = MapDecoderClsid(resolved);
    if (clsid == 0u) {
        return Result::NotSupported;
    }

    IImageDecoder* decoder = nullptr;
    if (ISHELL_CreateInstance(m_impl->shell, clsid,
                              reinterpret_cast<void**>(&decoder)) != AEE_SUCCESS || !decoder) {
        return Result::NotSupported;
    }

    IForceFeed* feed = nullptr;
    if (IImageDecoder_QueryInterface(decoder, AEEIID_IForceFeed,
                                     reinterpret_cast<void**>(&feed)) != AEE_SUCCESS || !feed) {
        IImageDecoder_Release(decoder);
        return Result::GraphicsError;
    }

    const int writeRes = IForceFeed_Write(feed, const_cast<void*>(data), static_cast<int>(size));
    IForceFeed_Release(feed);
    if (writeRes != AEE_SUCCESS) {
        IImageDecoder_Release(decoder);
        return Result::GraphicsError;
    }

    IBitmap* bitmap = nullptr;
    const int bmpRes = IImageDecoder_GetBitmap(decoder, &bitmap);
    const int rop = IImageDecoder_GetRop(decoder);
    IImageDecoder_Release(decoder);

    if (bmpRes != AEE_SUCCESS || !bitmap) {
        return Result::GraphicsError;
    }

    const bool hasTransparentRop = (rop == AEE_RO_TRANSPARENT);
    const Result res = ExtractBitmapPixels(bitmap, hasTransparentRop, desiredFormat, outImage);
    IBITMAP_Release(bitmap);
    return res;
}

Result ImageDecoder::DecodeFile(const char* filePath, DecodedImage& outImage,
                                TextureFormat desiredFormat) {
    MEMSET(&outImage, 0, sizeof(outImage));

    if (!m_impl || !m_impl->shell) {
        return Result::NotInitialized;
    }
    if (!filePath || filePath[0] == '\0') {
        return Result::InvalidArg;
    }

    IImage* image = ISHELL_LoadImage(m_impl->shell, filePath);
    if (!image) {
        return Result::NotFound;
    }

    IBitmap* bitmap = nullptr;
    int parmErr = AEE_SUCCESS;
    IImage_SetParm(image, IPARM_GETBITMAP,
                   reinterpret_cast<int>(&bitmap),
                   reinterpret_cast<int>(&parmErr));

    int rop = AEE_RO_COPY;
    IImage_SetParm(image, IPARM_GETROP, reinterpret_cast<int>(&rop), 0);

    if (parmErr != AEE_SUCCESS || !bitmap) {
        IImage_Release(image);
        return Result::GraphicsError;
    }

    const bool hasTransparentRop = (rop == AEE_RO_TRANSPARENT);
    const Result res = ExtractBitmapPixels(bitmap, hasTransparentRop, desiredFormat, outImage);
    IBITMAP_Release(bitmap);
    IImage_Release(image);
    return res;
}

void ImageDecoder::FreeImage(DecodedImage& image) {
    if (image.pixels) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, image.pixels);
        image.pixels = nullptr;
    }
    image.width    = 0u;
    image.height   = 0u;
    image.dataSize = 0u;
    image.hasAlpha = false;
}

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
