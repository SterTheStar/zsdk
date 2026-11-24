/**
 * @file sprite2d.hpp
 * @brief 2D Hardware Blitting (GL_OES_draw_texture), BREW IBitmap, ISprite, and IFont wrappers.
 */

#ifndef ZSDK_GRAPHICS_SPRITE2D_HPP
#define ZSDK_GRAPHICS_SPRITE2D_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/graphics/texture.hpp"

namespace zsdk {
namespace graphics {

/**
 * @brief Square sprite / tile dimensions supported by BREW ISprite.
 */
enum class SpriteSize : uint8_t {
    Size8x8   = 0,
    Size16x16 = 1,
    Size32x32 = 2,
    Size64x64 = 3
};

/**
 * @brief Transform bitmask flags for SpriteCommand (matches AEESpriteCmd::unTransform).
 */
namespace SpriteTransform {
    constexpr uint16_t None            = 0x0000u;
    constexpr uint16_t Rotate90        = 0x0001u;
    constexpr uint16_t Rotate180       = 0x0002u;
    constexpr uint16_t Rotate270       = 0x0003u;
    constexpr uint16_t FlipX           = 0x0004u;
    constexpr uint16_t FlipY           = 0x0006u;
    constexpr uint16_t ScaleEighth     = 0x0008u;
    constexpr uint16_t ScaleQuarter    = 0x0010u;
    constexpr uint16_t ScaleHalf       = 0x0018u;
    constexpr uint16_t Scale2x         = 0x0020u;
    constexpr uint16_t Scale4x         = 0x0028u;
    constexpr uint16_t Scale8x         = 0x0030u;
    constexpr uint16_t MatrixTransform = 0x8000u;
} // namespace SpriteTransform

/**
 * @brief System font presets matching BREW AEECLSID_FONTSYS* class IDs.
 */
enum class SystemFont : uint8_t {
    Normal = 0,
    Large,
    Bold,
    Italic,
    BoldItalic,
    LargeItalic
};

/**
 * @brief Command descriptor for SpriteEngine2D::DrawSprites().
 */
struct SpriteCommand {
    int16_t  x;
    int16_t  y;
    uint16_t transform;     /**< Combination of SpriteTransform::* flags */
    uint8_t  matrixIndex;   /**< Index into transform matrix table if MatrixTransform is set */
    uint8_t  spriteIndex;   /**< Vertical tile index in sprite buffer (y = spriteIndex * size) */
    SpriteSize size;
    bool     transparent;   /**< True for color-key transparent blit, false for opaque */
    uint8_t  layer;         /**< Draw layer 0..3, or 0xFF to hide */
};

/**
 * @brief Tile map descriptor for SpriteEngine2D::DrawTiles().
 */
struct TileMap2D {
    uint16_t*  mapArray;    /**< Row-major tile entries (bits 9-0 index, bit 12 transparent, bits 15-13 xform) */
    uint32_t   flags;       /**< Bit 0: wrap around (MAP_FLAG_WRAP) */
    int32_t    x;
    int32_t    y;
    uint16_t   widthInTiles;
    uint16_t   heightInTiles;
    SpriteSize tileSize;
};

/**
 * @brief RAII wrapper over BREW IBitmap / IDIB for 2D offscreen surfaces and blitting.
 */
class Bitmap2D : public NonCopyable {
public:
    Bitmap2D();
    ~Bitmap2D();

    /**
     * @brief Creates an offscreen DIB bitmap via IDisplay::CreateDIBitmap.
     * @param platformContext Opaque IShell*.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param colorDepth Bits per pixel (default 16 for RGB565).
     */
    Result Create(void* platformContext, uint16_t width, uint16_t height, uint8_t colorDepth = 16u);

    /** @brief Creates a bitmap compatible with @p referenceBitmap. */
    Result CreateCompatible(const Bitmap2D& referenceBitmap, uint16_t width, uint16_t height);

    /** @brief Wraps the primary device screen bitmap (IDisplay::GetDeviceBitmap). */
    Result AcquireDeviceBitmap(void* platformContext);

    /** @brief Releases the underlying IBitmap*. */
    void Destroy();

    /** @brief Fills a rectangle with @p color. */
    Result FillRect(const Rect& rect, const Color& color);

    /** @brief Sets the color-key transparency color for subsequent transparent blits. */
    Result SetTransparencyColor(const Color& color);

    /**
     * @brief Blits this bitmap onto @p dst using IBitmap::BltIn.
     * @param dst Destination bitmap.
     * @param dstX Destination X coordinate.
     * @param dstY Destination Y coordinate.
     * @param srcRect Optional source sub-rectangle (nullptr = full bitmap).
     * @param transparent Use AEE_RO_TRANSPARENT if true, else AEE_RO_COPY.
     */
    Result BlitTo(Bitmap2D& dst, int32_t dstX, int32_t dstY,
                  const Rect* srcRect = nullptr, bool transparent = false) const;

    uint16_t GetWidth() const;
    uint16_t GetHeight() const;
    bool     IsValid() const;

    /** @brief Returns the underlying BREW IBitmap* as an opaque pointer. */
    void* GetNativeBitmap() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Hardware-accelerated 2D sprite & tile-map engine wrapping BREW ISprite (AEESprite.h).
 */
class SpriteEngine2D : public NonCopyable {
public:
    SpriteEngine2D();
    ~SpriteEngine2D();

    /** @brief Instantiates AEECLSID_SPRITE using @p platformContext (IShell*). */
    Result Initialize(void* platformContext);

    /** @brief Releases ISprite*. */
    void Shutdown();

    /** @brief Sets the destination IBitmap* for sprite/tile rendering. */
    Result SetDestination(Bitmap2D& dst);

    /** @brief Binds a vertical strip sprite sheet for @p size. */
    Result SetSpriteBuffer(SpriteSize size, Bitmap2D& buffer);

    /** @brief Binds a vertical strip tile sheet for @p size. */
    Result SetTileBuffer(SpriteSize size, Bitmap2D& buffer);

    /** @brief Draws @p count sprites in a single ISPRITE_DrawSprites() batch. */
    Result DrawSprites(const SpriteCommand* cmds, uint32_t count);

    /** @brief Draws @p count tile maps in a single ISPRITE_DrawTiles() batch. */
    Result DrawTiles(const TileMap2D* maps, uint32_t count);

    bool IsValid() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Text measurement & rendering wrapper over BREW IFont (AEEFont.h).
 */
class Font2D : public NonCopyable {
public:
    Font2D();
    ~Font2D();

    /** @brief Creates a BREW system font (AEECLSID_FONTSYS*). */
    Result Initialize(void* platformContext, SystemFont font = SystemFont::Normal);

    /** @brief Releases IFont*. */
    void Shutdown();

    /** @brief Draws ASCII text onto @p dst bitmap. */
    Result DrawText(Bitmap2D& dst, int32_t x, int32_t y, const char* asciiText,
                    const Color& fg, const Color& bg,
                    const Rect* clipRect = nullptr, bool transparent = true) const;

    /** @brief Measures how many characters fit in @p maxWidth and total pixel width. */
    Result MeasureText(const char* asciiText, int32_t maxWidth,
                       int32_t& outCharsFit, int32_t& outPixelWidth) const;

    /** @brief Queries font ascent and descent in pixels. */
    Result GetMetrics(int16_t& outAscent, int16_t& outDescent) const;

    bool IsValid() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief OpenGL ES 1.1 hardware 2D blitter using GL_OES_draw_texture (glDrawTexfOES).
 */
class HardwareBlitter2D {
public:
    /** @brief Resolves glDrawTexfOES via eglGetProcAddress; returns true if supported. */
    static bool IsSupported();

    /**
     * @brief Draws a textured 2D quad directly in screen coordinates via glDrawTexfOES.
     * @param texture Bound/valid Texture to draw.
     * @param dstX Screen X coordinate (origin at bottom-left in GLES OES_draw_texture).
     * @param dstY Screen Y coordinate.
     * @param dstW Destination width in pixels.
     * @param dstH Destination height in pixels.
     * @param srcCrop Optional texel crop rect {x, y, width, height}; nullptr uses full texture.
     * @param z Depth coordinate in [0, 1] (default 0.0f).
     * @return Result::Ok on success, Result::NotSupported if OES_draw_texture is unavailable.
     */
    static Result DrawTexture(const Texture& texture,
                              float dstX, float dstY, float dstW, float dstH,
                              const Rect* srcCrop = nullptr, float z = 0.0f);

    /**
     * @brief Draws a tinted textured 2D quad via glDrawTexfOES with glColor4ub modulation.
     */
    static Result DrawTextureTinted(const Texture& texture,
                                    float dstX, float dstY, float dstW, float dstH,
                                    const Color& tint,
                                    const Rect* srcCrop = nullptr, float z = 0.0f);
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_SPRITE2D_HPP */
