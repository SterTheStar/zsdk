/**
 * @file embedded_font.hpp
 * @brief Embedded 8x8 monochrome bitmap font (ASCII 0x20..0x7F, 768 bytes in ROM).
 *
 * Zero external file dependencies; used by DebugOverlay to render text in OpenGL ES.
 */

#ifndef ZSDK_DEBUG_EMBEDDED_FONT_HPP
#define ZSDK_DEBUG_EMBEDDED_FONT_HPP

#include "zsdk/core/config.hpp"

namespace zsdk {
namespace debug {

/** @brief Number of printable ASCII characters (0x20 ' ' through 0x7F). */
static const uint32_t kFontCharCount = 96u;
static const uint32_t kFontCharWidth = 8u;
static const uint32_t kFontCharHeight = 8u;

/**
 * @brief Returns the 8-byte row bitmap for ASCII character @p ch (clamped to 0x20..0x7F).
 */
const uint8_t* GetEmbeddedFontGlyph(char ch);

} // namespace debug
} // namespace zsdk

#endif /* ZSDK_DEBUG_EMBEDDED_FONT_HPP */
