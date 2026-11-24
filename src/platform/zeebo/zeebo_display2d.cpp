/**
 * @file zeebo_display2d.cpp
 * @brief Qualcomm BREW / Zeebo 2D IDisplay bridge (AEEDisp.h).
 */

#include "zsdk/graphics/display2d.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEDisp.h"

namespace zsdk {
namespace graphics {

struct Display2D::Impl {
    IDisplay* display;
    int32_t   width;
    int32_t   height;
};

Display2D::Display2D()
    : m_impl(nullptr) {
}

Display2D::~Display2D() {
    Shutdown();
}

Result Display2D::Initialize(void* platformContext) {
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
    m_impl->display = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_DISPLAY,
                              reinterpret_cast<void**>(&m_impl->display)) != AEE_SUCCESS || !m_impl->display) {
        Shutdown();
        return Result::GraphicsError;
    }

    AEEDeviceInfo di;
    MEMSET(&di, 0, sizeof(di));
    di.wStructSize = sizeof(di);
    ISHELL_GetDeviceInfo(shell, &di);
    m_impl->width  = static_cast<int32_t>(di.cxScreen);
    m_impl->height = static_cast<int32_t>(di.cyScreen);

    return Result::Ok;
}

void Display2D::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->display) {
        IDISPLAY_Release(m_impl->display);
        m_impl->display = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

void Display2D::Clear(const Color& color) {
    if (!m_impl || !m_impl->display) {
        return;
    }
    AEERect rc = {0, 0, static_cast<int16>(m_impl->width), static_cast<int16>(m_impl->height)};
    IDISPLAY_FillRect(m_impl->display, &rc, MAKE_RGB(color.r, color.g, color.b));
}

void Display2D::FillRect(const Rect& rect, const Color& color) {
    if (!m_impl || !m_impl->display) {
        return;
    }
    AEERect rc = {
        static_cast<int16>(rect.x),
        static_cast<int16>(rect.y),
        static_cast<int16>(rect.width),
        static_cast<int16>(rect.height)
    };
    IDISPLAY_FillRect(m_impl->display, &rc, MAKE_RGB(color.r, color.g, color.b));
}

void Display2D::DrawText(int32_t x, int32_t y, const char* asciiText, const Color& color) {
    if (!m_impl || !m_impl->display || !asciiText) {
        return;
    }
    AECHAR wbuf[128];
    STRTOWSTR(asciiText, wbuf, sizeof(wbuf));
    IDISPLAY_SetColor(m_impl->display, CLR_USER_TEXT, MAKE_RGB(color.r, color.g, color.b));
    IDISPLAY_DrawText(m_impl->display, AEE_FONT_NORMAL, wbuf, -1,
                      static_cast<int>(x), static_cast<int>(y), nullptr, IDF_TEXT_TRANSPARENT);
}

void Display2D::Present() {
    if (m_impl && m_impl->display) {
        IDISPLAY_Update(m_impl->display);
    }
}

int32_t Display2D::GetWidth() const {
    return m_impl ? m_impl->width : 0;
}

int32_t Display2D::GetHeight() const {
    return m_impl ? m_impl->height : 0;
}

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
