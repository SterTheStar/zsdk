/**
 * @file display2d.hpp
 * @brief Lightweight 2D Display / Bitmap abstraction over BREW IDisplay (AEEDisp.h).
 */

#ifndef ZSDK_GRAPHICS_DISPLAY2D_HPP
#define ZSDK_GRAPHICS_DISPLAY2D_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

/**
 * @brief 2D fallback/splash renderer wrapping BREW IDisplay*.
 */
class Display2D : public NonCopyable {
public:
    Display2D();
    ~Display2D();

    Result Initialize(void* platformContext);
    void   Shutdown();

    void Clear(const Color& color);
    void FillRect(const Rect& rect, const Color& color);
    void DrawText(int32_t x, int32_t y, const char* asciiText, const Color& color);
    void Present();

    int32_t GetWidth() const;
    int32_t GetHeight() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_DISPLAY2D_HPP */
