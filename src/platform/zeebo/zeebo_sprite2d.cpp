/**
 * @file zeebo_sprite2d.cpp
 * @brief Qualcomm BREW / Zeebo IBitmap, ISprite, IFont, and glDrawTexfOES 2D blitting.
 */

#include "zsdk/graphics/sprite2d.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEDisp.h"
#include "AEEBitmap.h"
#include "AEEIDIB.h"
#include "AEESprite.h"
#include "AEEFont.h"
#include "AEEClassIDs.h"
#include <gles/gles_1_1/gl.h>
#include <gles/egl.h>
#include <gles/glESext.h>

namespace zsdk {
namespace graphics {

namespace {

AEECLSID MapSystemFontClsid(SystemFont font) {
    switch (font) {
        case SystemFont::Large:       return AEECLSID_FONTSYSLARGE;
        case SystemFont::Bold:        return AEECLSID_FONTSYSBOLD;
        case SystemFont::Italic:      return AEECLSID_FONTSYSITALIC;
        case SystemFont::BoldItalic:  return AEECLSID_FONTSYSBOLDITALIC;
        case SystemFont::LargeItalic: return AEECLSID_FONTSYSLARGEITALIC;
        case SystemFont::Normal:
        default:                      return AEECLSID_FONTSYSNORMAL;
    }
}

typedef void (GL_APIENTRY* DrawTexfOesFn)(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (GL_APIENTRY* TexParameterivFn)(GLenum, GLenum, const GLint*);

DrawTexfOesFn ResolveDrawTexfOES() {
    static bool s_resolved = false;
    static DrawTexfOesFn s_fn = nullptr;
    if (!s_resolved) {
        s_fn = reinterpret_cast<DrawTexfOesFn>(eglGetProcAddress("glDrawTexfOES"));
        s_resolved = true;
    }
    return s_fn;
}

TexParameterivFn ResolveTexParameteriv() {
    static bool s_resolved = false;
    static TexParameterivFn s_fn = nullptr;
    if (!s_resolved) {
        s_fn = reinterpret_cast<TexParameterivFn>(eglGetProcAddress("glTexParameteriv"));
        s_resolved = true;
    }
    return s_fn;
}

} // namespace

/* ============================================================================
 * Bitmap2D
 * ========================================================================== */

struct Bitmap2D::Impl {
    IBitmap* bitmap;
    uint16_t width;
    uint16_t height;
};

Bitmap2D::Bitmap2D()
    : m_impl(nullptr) {
}

Bitmap2D::~Bitmap2D() {
    Destroy();
}

Result Bitmap2D::Create(void* platformContext, uint16_t width, uint16_t height, uint8_t colorDepth) {
    if (m_impl) {
        Destroy();
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || width == 0u || height == 0u) {
        return Result::InvalidArg;
    }

    IDisplay* display = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_DISPLAY,
                              reinterpret_cast<void**>(&display)) != AEE_SUCCESS || !display) {
        return Result::GraphicsError;
    }

    IDIB* dib = nullptr;
    const int res = IDISPLAY_CreateDIBitmap(display, &dib, colorDepth, width, height);
    IDISPLAY_Release(display);

    if (res != AEE_SUCCESS || !dib) {
        return Result::OutOfMemory;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Graphics, sizeof(Impl)));
    if (!m_impl) {
        IDIB_Release(dib);
        return Result::OutOfMemory;
    }

    m_impl->bitmap = IDIB_to_IBitmap(dib);
    m_impl->width  = width;
    m_impl->height = height;
    return Result::Ok;
}

Result Bitmap2D::CreateCompatible(const Bitmap2D& referenceBitmap, uint16_t width, uint16_t height) {
    if (m_impl) {
        Destroy();
    }
    if (!referenceBitmap.m_impl || !referenceBitmap.m_impl->bitmap || width == 0u || height == 0u) {
        return Result::InvalidArg;
    }

    IBitmap* bmp = nullptr;
    if (IBITMAP_CreateCompatibleBitmap(referenceBitmap.m_impl->bitmap, &bmp, width, height) != AEE_SUCCESS || !bmp) {
        return Result::OutOfMemory;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Graphics, sizeof(Impl)));
    if (!m_impl) {
        IBITMAP_Release(bmp);
        return Result::OutOfMemory;
    }

    m_impl->bitmap = bmp;
    m_impl->width  = width;
    m_impl->height = height;
    return Result::Ok;
}

Result Bitmap2D::AcquireDeviceBitmap(void* platformContext) {
    if (m_impl) {
        Destroy();
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    IDisplay* display = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_DISPLAY,
                              reinterpret_cast<void**>(&display)) != AEE_SUCCESS || !display) {
        return Result::GraphicsError;
    }

    IBitmap* bmp = nullptr;
    const int res = IDISPLAY_GetDeviceBitmap(display, &bmp);
    IDISPLAY_Release(display);
    if (res != AEE_SUCCESS || !bmp) {
        return Result::GraphicsError;
    }

    AEEBitmapInfo info;
    MEMSET(&info, 0, sizeof(info));
    IBITMAP_GetInfo(bmp, &info, sizeof(info));

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Graphics, sizeof(Impl)));
    if (!m_impl) {
        IBITMAP_Release(bmp);
        return Result::OutOfMemory;
    }

    m_impl->bitmap = bmp;
    m_impl->width  = static_cast<uint16_t>(info.cx);
    m_impl->height = static_cast<uint16_t>(info.cy);
    return Result::Ok;
}

void Bitmap2D::Destroy() {
    if (!m_impl) {
        return;
    }
    if (m_impl->bitmap) {
        IBITMAP_Release(m_impl->bitmap);
        m_impl->bitmap = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

Result Bitmap2D::FillRect(const Rect& rect, const Color& color) {
    if (!m_impl || !m_impl->bitmap) {
        return Result::NotInitialized;
    }
    const NativeColor nc = IBITMAP_RGBToNative(m_impl->bitmap, MAKE_RGB(color.r, color.g, color.b));
    const AEERect rc = {
        static_cast<int16>(rect.x),
        static_cast<int16>(rect.y),
        static_cast<int16>(rect.width),
        static_cast<int16>(rect.height)
    };
    return (IBITMAP_FillRect(m_impl->bitmap, &rc, nc, AEE_RO_COPY) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result Bitmap2D::SetTransparencyColor(const Color& color) {
    if (!m_impl || !m_impl->bitmap) {
        return Result::NotInitialized;
    }
    const NativeColor nc = IBITMAP_RGBToNative(m_impl->bitmap, MAKE_RGB(color.r, color.g, color.b));
    return (IBITMAP_SetTransparencyColor(m_impl->bitmap, nc) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result Bitmap2D::BlitTo(Bitmap2D& dst, int32_t dstX, int32_t dstY,
                        const Rect* srcRect, bool transparent) const {
    if (!m_impl || !m_impl->bitmap || !dst.m_impl || !dst.m_impl->bitmap) {
        return Result::InvalidArg;
    }

    const int sx = srcRect ? static_cast<int>(srcRect->x) : 0;
    const int sy = srcRect ? static_cast<int>(srcRect->y) : 0;
    const int dx = srcRect ? static_cast<int>(srcRect->width) : static_cast<int>(m_impl->width);
    const int dy = srcRect ? static_cast<int>(srcRect->height) : static_cast<int>(m_impl->height);
    const AEERasterOp rop = transparent ? AEE_RO_TRANSPARENT : AEE_RO_COPY;

    return (IBITMAP_BltIn(dst.m_impl->bitmap, static_cast<int>(dstX), static_cast<int>(dstY),
                          dx, dy, m_impl->bitmap, sx, sy, rop) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

uint16_t Bitmap2D::GetWidth() const {
    return m_impl ? m_impl->width : 0u;
}

uint16_t Bitmap2D::GetHeight() const {
    return m_impl ? m_impl->height : 0u;
}

bool Bitmap2D::IsValid() const {
    return m_impl && m_impl->bitmap != nullptr;
}

void* Bitmap2D::GetNativeBitmap() const {
    return m_impl ? m_impl->bitmap : nullptr;
}

/* ============================================================================
 * SpriteEngine2D (ISprite)
 * ========================================================================== */

struct SpriteEngine2D::Impl {
    ISprite* sprite;
};

SpriteEngine2D::SpriteEngine2D()
    : m_impl(nullptr) {
}

SpriteEngine2D::~SpriteEngine2D() {
    Shutdown();
}

Result SpriteEngine2D::Initialize(void* platformContext) {
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
    m_impl->sprite = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_SPRITE,
                              reinterpret_cast<void**>(&m_impl->sprite)) != AEE_SUCCESS || !m_impl->sprite) {
        Shutdown();
        return Result::NotSupported;
    }

    return Result::Ok;
}

void SpriteEngine2D::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->sprite) {
        ISPRITE_Release(m_impl->sprite);
        m_impl->sprite = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

Result SpriteEngine2D::SetDestination(Bitmap2D& dst) {
    if (!m_impl || !m_impl->sprite) {
        return Result::NotInitialized;
    }
    IBitmap* bmp = static_cast<IBitmap*>(dst.GetNativeBitmap());
    if (!bmp) {
        return Result::InvalidArg;
    }
    return (ISPRITE_SetDestination(m_impl->sprite, bmp) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result SpriteEngine2D::SetSpriteBuffer(SpriteSize size, Bitmap2D& buffer) {
    if (!m_impl || !m_impl->sprite) {
        return Result::NotInitialized;
    }
    IBitmap* bmp = static_cast<IBitmap*>(buffer.GetNativeBitmap());
    return (ISPRITE_SetSpriteBuffer(m_impl->sprite, static_cast<uint8>(size), bmp) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result SpriteEngine2D::SetTileBuffer(SpriteSize size, Bitmap2D& buffer) {
    if (!m_impl || !m_impl->sprite) {
        return Result::NotInitialized;
    }
    IBitmap* bmp = static_cast<IBitmap*>(buffer.GetNativeBitmap());
    return (ISPRITE_SetTileBuffer(m_impl->sprite, static_cast<uint8>(size), bmp) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result SpriteEngine2D::DrawSprites(const SpriteCommand* cmds, uint32_t count) {
    if (!m_impl || !m_impl->sprite) {
        return Result::NotInitialized;
    }
    if (!cmds || count == 0u) {
        return Result::Ok;
    }

    constexpr uint32_t kStackCap = 64u;
    AEESpriteCmd stackBuf[kStackCap + 1u];
    AEESpriteCmd* aeeCmds = stackBuf;
    bool heapAllocated = false;

    if (count > kStackCap) {
        aeeCmds = static_cast<AEESpriteCmd*>(debug::MemoryTracker::Alloc(
            debug::MemorySubsystem::Graphics, (count + 1u) * sizeof(AEESpriteCmd)));
        if (!aeeCmds) {
            return Result::OutOfMemory;
        }
        heapAllocated = true;
    }

    MEMSET(aeeCmds, 0, (count + 1u) * sizeof(AEESpriteCmd));
    for (uint32_t i = 0u; i < count; ++i) {
        aeeCmds[i].x                 = cmds[i].x;
        aeeCmds[i].y                 = cmds[i].y;
        aeeCmds[i].unTransform       = cmds[i].transform;
        aeeCmds[i].unMatrixTransform = cmds[i].matrixIndex;
        aeeCmds[i].unSpriteIndex     = cmds[i].spriteIndex;
        aeeCmds[i].unSpriteSize      = static_cast<uint8>(cmds[i].size);
        aeeCmds[i].unComposite       = cmds[i].transparent ? 0x00u : 0xFFu;
        aeeCmds[i].unLayer           = cmds[i].layer;
    }
    aeeCmds[count].unSpriteSize = SPRITE_SIZE_END;

    const int res = ISPRITE_DrawSprites(m_impl->sprite, aeeCmds);
    if (heapAllocated) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, aeeCmds);
    }

    return (res == AEE_SUCCESS) ? Result::Ok : Result::GraphicsError;
}

Result SpriteEngine2D::DrawTiles(const TileMap2D* maps, uint32_t count) {
    if (!m_impl || !m_impl->sprite) {
        return Result::NotInitialized;
    }
    if (!maps || count == 0u) {
        return Result::Ok;
    }

    constexpr uint32_t kStackCap = 8u;
    AEETileMap stackBuf[kStackCap + 1u];
    AEETileMap* aeeMaps = stackBuf;
    bool heapAllocated = false;

    if (count > kStackCap) {
        aeeMaps = static_cast<AEETileMap*>(debug::MemoryTracker::Alloc(
            debug::MemorySubsystem::Graphics, (count + 1u) * sizeof(AEETileMap)));
        if (!aeeMaps) {
            return Result::OutOfMemory;
        }
        heapAllocated = true;
    }

    MEMSET(aeeMaps, 0, (count + 1u) * sizeof(AEETileMap));
    for (uint32_t i = 0u; i < count; ++i) {
        aeeMaps[i].pMapArray  = maps[i].mapArray;
        aeeMaps[i].unFlags    = maps[i].flags;
        aeeMaps[i].x          = maps[i].x;
        aeeMaps[i].y          = maps[i].y;
        aeeMaps[i].w          = maps[i].widthInTiles;
        aeeMaps[i].h          = maps[i].heightInTiles;
        aeeMaps[i].unTileSize = static_cast<uint8>(maps[i].tileSize);
    }
    aeeMaps[count].pMapArray = nullptr;

    const int res = ISPRITE_DrawTiles(m_impl->sprite, aeeMaps);
    if (heapAllocated) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, aeeMaps);
    }

    return (res == AEE_SUCCESS) ? Result::Ok : Result::GraphicsError;
}

bool SpriteEngine2D::IsValid() const {
    return m_impl && m_impl->sprite != nullptr;
}

/* ============================================================================
 * Font2D (IFont)
 * ========================================================================== */

struct Font2D::Impl {
    IFont* font;
};

Font2D::Font2D()
    : m_impl(nullptr) {
}

Font2D::~Font2D() {
    Shutdown();
}

Result Font2D::Initialize(void* platformContext, SystemFont font) {
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
    m_impl->font = nullptr;

    if (ISHELL_CreateInstance(shell, MapSystemFontClsid(font),
                              reinterpret_cast<void**>(&m_impl->font)) != AEE_SUCCESS || !m_impl->font) {
        Shutdown();
        return Result::NotSupported;
    }

    return Result::Ok;
}

void Font2D::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->font) {
        IFONT_Release(m_impl->font);
        m_impl->font = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

Result Font2D::DrawText(Bitmap2D& dst, int32_t x, int32_t y, const char* asciiText,
                        const Color& fg, const Color& bg,
                        const Rect* clipRect, bool transparent) const {
    if (!m_impl || !m_impl->font) {
        return Result::NotInitialized;
    }
    IBitmap* bmp = static_cast<IBitmap*>(dst.GetNativeBitmap());
    if (!bmp || !asciiText) {
        return Result::InvalidArg;
    }

    AECHAR wbuf[256];
    STRTOWSTR(asciiText, wbuf, sizeof(wbuf));

    const NativeColor ncFg = IBITMAP_RGBToNative(bmp, MAKE_RGB(fg.r, fg.g, fg.b));
    const NativeColor ncBg = IBITMAP_RGBToNative(bmp, MAKE_RGB(bg.r, bg.g, bg.b));

    AEERect rc;
    const AEERect* pClip = nullptr;
    if (clipRect) {
        rc.x  = static_cast<int16>(clipRect->x);
        rc.y  = static_cast<int16>(clipRect->y);
        rc.dx = static_cast<int16>(clipRect->width);
        rc.dy = static_cast<int16>(clipRect->height);
        pClip = &rc;
    }

    const uint32 flags = transparent ? IFONT_DRAW_TRANSPARENT : 0u;
    return (IFONT_DrawText(m_impl->font, bmp, static_cast<int>(x), static_cast<int>(y),
                           wbuf, -1, ncFg, ncBg, pClip, flags) == AEE_SUCCESS)
        ? Result::Ok
        : Result::GraphicsError;
}

Result Font2D::MeasureText(const char* asciiText, int32_t maxWidth,
                           int32_t& outCharsFit, int32_t& outPixelWidth) const {
    outCharsFit   = 0;
    outPixelWidth = 0;

    if (!m_impl || !m_impl->font) {
        return Result::NotInitialized;
    }
    if (!asciiText) {
        return Result::InvalidArg;
    }

    AECHAR wbuf[256];
    STRTOWSTR(asciiText, wbuf, sizeof(wbuf));

    int fits = 0;
    int pixels = 0;
    const int maxW = (maxWidth < 0) ? IFONT_MAXWIDTH : static_cast<int>(maxWidth);
    if (IFONT_MeasureText(m_impl->font, wbuf, -1, maxW, &fits, &pixels) != AEE_SUCCESS) {
        return Result::GraphicsError;
    }

    outCharsFit   = static_cast<int32_t>(fits);
    outPixelWidth = static_cast<int32_t>(pixels);
    return Result::Ok;
}

Result Font2D::GetMetrics(int16_t& outAscent, int16_t& outDescent) const {
    outAscent  = 0;
    outDescent = 0;

    if (!m_impl || !m_impl->font) {
        return Result::NotInitialized;
    }

    AEEFontInfo info;
    MEMSET(&info, 0, sizeof(info));
    if (IFONT_GetInfo(m_impl->font, &info, sizeof(info)) != AEE_SUCCESS) {
        return Result::GraphicsError;
    }

    outAscent  = info.nAscent;
    outDescent = info.nDescent;
    return Result::Ok;
}

bool Font2D::IsValid() const {
    return m_impl && m_impl->font != nullptr;
}

/* ============================================================================
 * HardwareBlitter2D (GL_OES_draw_texture)
 * ========================================================================== */

bool HardwareBlitter2D::IsSupported() {
    return ResolveDrawTexfOES() != nullptr;
}

Result HardwareBlitter2D::DrawTexture(const Texture& texture,
                                      float dstX, float dstY, float dstW, float dstH,
                                      const Rect* srcCrop, float z) {
    if (!texture.IsValid()) {
        return Result::InvalidArg;
    }
    DrawTexfOesFn drawTexf = ResolveDrawTexfOES();
    TexParameterivFn texParamiv = ResolveTexParameteriv();
    if (!drawTexf || !texParamiv) {
        return Result::NotSupported;
    }

    texture.Bind(0u);

    GLint crop[4];
    if (srcCrop) {
        crop[0] = static_cast<GLint>(srcCrop->x);
        crop[1] = static_cast<GLint>(srcCrop->y);
        crop[2] = static_cast<GLint>(srcCrop->width);
        crop[3] = static_cast<GLint>(srcCrop->height);
    } else {
        crop[0] = 0;
        crop[1] = 0;
        crop[2] = static_cast<GLint>(texture.GetWidth());
        crop[3] = static_cast<GLint>(texture.GetHeight());
    }
    texParamiv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);

    drawTexf(dstX, dstY, z, dstW, dstH);
    return (glGetError() == GL_NO_ERROR) ? Result::Ok : Result::GraphicsError;
}

Result HardwareBlitter2D::DrawTextureTinted(const Texture& texture,
                                            float dstX, float dstY, float dstW, float dstH,
                                            const Color& tint,
                                            const Rect* srcCrop, float z) {
    glColor4f(static_cast<GLfloat>(tint.r) / 255.0f,
              static_cast<GLfloat>(tint.g) / 255.0f,
              static_cast<GLfloat>(tint.b) / 255.0f,
              static_cast<GLfloat>(tint.a) / 255.0f);
    return DrawTexture(texture, dstX, dstY, dstW, dstH, srcCrop, z);
}

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
