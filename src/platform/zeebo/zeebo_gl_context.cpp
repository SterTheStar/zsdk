/**
 * @file zeebo_gl_context.cpp
 * @brief Qualcomm BREW / Zeebo EGL 1.1 & OpenGL ES 1.1 context bridge.
 *
 * Wraps EGL_Init, GLES_Init, IDisplay, IDIB, EGLDisplay, EGLSurface, and EGLContext.
 */

#include "zsdk/graphics/gl_context.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEDisp.h"
#include "AEEBitmap.h"
#include "EGL_1x.h"
#include "GLES_1x.h"
#include "AEEEGL10.h"
#include "AEEEGLOESSwapInterval.h"
#include "AEEEGLSurfaceManip.h"
#include "AEEEGLGetPowerLevel.h"
#include <gles/egl.h>
#include <gles/gles_1_1/gl.h>

extern "C" {
void ReleaseNBI(void) {}
void FreeExtList(void) {}
}

namespace zsdk {
namespace graphics {

struct GLContext::Impl {
    IShell*    shell;
    IEGL10*    iegl;
    IDisplay*  display;
    IBitmap*   deviceBitmap;
    IDIB*      dib;
    EGLDisplay eglDisplay;
    EGLSurface eglSurface;
    EGLContext eglContext;
    EGLConfig  eglConfig;
    int32_t    width;
    int32_t    height;
    int32_t    renderWidth;
    int32_t    renderHeight;
    bool       eglBindingsLoaded;
    bool       glesBindingsLoaded;
};

GLContext::GLContext()
    : m_impl(nullptr) {
}

GLContext::~GLContext() {
    Shutdown();
}

Result GLContext::Initialize(void* platformContext, const GLContextConfig& config) {
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
    MEMSET(m_impl, 0, sizeof(Impl));
    m_impl->shell = shell;
    m_impl->eglDisplay = EGL_NO_DISPLAY;
    m_impl->eglSurface = EGL_NO_SURFACE;
    m_impl->eglContext = EGL_NO_CONTEXT;

    if (EGL_Init(shell) != 0) {
        ZSDK_LOG_ERROR("GFX", "EGL_Init failed");
        Shutdown();
        return Result::GraphicsError;
    }
    m_impl->eglBindingsLoaded = true;

    if (GLES_Init(shell) != 0) {
        ZSDK_LOG_ERROR("GFX", "GLES_Init failed");
        Shutdown();
        return Result::GraphicsError;
    }
    m_impl->glesBindingsLoaded = true;

#ifndef AEECLSID_EGL
    #define AEECLSID_EGL 0x01014bc4
#endif

    ISHELL_CreateInstance(shell, AEECLSID_EGL, reinterpret_cast<void**>(&m_impl->iegl));

    if (ISHELL_CreateInstance(shell, AEECLSID_DISPLAY,
                              reinterpret_cast<void**>(&m_impl->display)) != AEE_SUCCESS || !m_impl->display) {
        ZSDK_LOG_ERROR("GFX", "Failed to create AEECLSID_DISPLAY");
        Shutdown();
        return Result::GraphicsError;
    }

    if (IDISPLAY_GetDeviceBitmap(m_impl->display, &m_impl->deviceBitmap) != SUCCESS ||
        !m_impl->deviceBitmap ||
        IBITMAP_QueryInterface(m_impl->deviceBitmap, AEECLSID_DIB,
                               reinterpret_cast<void**>(&m_impl->dib)) != AEE_SUCCESS || !m_impl->dib) {
        ZSDK_LOG_ERROR("GFX", "Failed to query IDIB from device bitmap");
        Shutdown();
        return Result::GraphicsError;
    }

    m_impl->width        = static_cast<int32_t>(m_impl->dib->cx);
    m_impl->height       = static_cast<int32_t>(m_impl->dib->cy);
    m_impl->renderWidth  = m_impl->width;
    m_impl->renderHeight = m_impl->height;

    m_impl->eglDisplay = eglGetDisplay(m_impl->display);
    if (m_impl->eglDisplay == EGL_NO_DISPLAY) {
        m_impl->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    EGLint major = 0, minor = 0;
    if (!eglInitialize(m_impl->eglDisplay, &major, &minor)) {
        ZSDK_LOG_ERROR("GFX", "eglInitialize failed");
        Shutdown();
        return Result::GraphicsError;
    }

    const EGLint attribs[] = {
        EGL_RED_SIZE,     static_cast<EGLint>(config.redBits),
        EGL_GREEN_SIZE,   static_cast<EGLint>(config.greenBits),
        EGL_BLUE_SIZE,    static_cast<EGLint>(config.blueBits),
        EGL_ALPHA_SIZE,   static_cast<EGLint>(config.alphaBits),
        EGL_DEPTH_SIZE,   static_cast<EGLint>(config.depthBits),
        EGL_STENCIL_SIZE, static_cast<EGLint>(config.stencilBits),
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLint numConfigs = 0;
    if (!eglChooseConfig(m_impl->eglDisplay, attribs, &m_impl->eglConfig, 1, &numConfigs) || numConfigs < 1) {
        ZSDK_LOG_ERROR("GFX", "eglChooseConfig failed");
        Shutdown();
        return Result::GraphicsError;
    }

    m_impl->eglSurface = eglCreateWindowSurface(m_impl->eglDisplay, m_impl->eglConfig,
                                                m_impl->dib, nullptr);
    if (m_impl->eglSurface == EGL_NO_SURFACE) {
        ZSDK_LOG_ERROR("GFX", "eglCreateWindowSurface failed");
        Shutdown();
        return Result::GraphicsError;
    }

    m_impl->eglContext = eglCreateContext(m_impl->eglDisplay, m_impl->eglConfig, EGL_NO_CONTEXT, nullptr);
    if (m_impl->eglContext == EGL_NO_CONTEXT) {
        ZSDK_LOG_ERROR("GFX", "eglCreateContext failed");
        Shutdown();
        return Result::GraphicsError;
    }

    if (!eglMakeCurrent(m_impl->eglDisplay, m_impl->eglSurface, m_impl->eglSurface, m_impl->eglContext)) {
        ZSDK_LOG_ERROR("GFX", "eglMakeCurrent failed");
        Shutdown();
        return Result::GraphicsError;
    }

    SetSwapInterval(static_cast<int32_t>(config.swapInterval));
    ZSDK_LOG_INFO("GFX", "EGL %d.%d ready (%dx%d)", major, minor, m_impl->width, m_impl->height);
    return Result::Ok;
}

void GLContext::Shutdown() {
    if (!m_impl) {
        return;
    }

    if (m_impl->eglDisplay != EGL_NO_DISPLAY) {
        if (m_impl->eglContext != EGL_NO_CONTEXT && m_impl->eglSurface != EGL_NO_SURFACE) {
            Renderer::ReleaseSharedFontTexture();
        }
        eglMakeCurrent(m_impl->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_impl->eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(m_impl->eglDisplay, m_impl->eglContext);
        }
        if (m_impl->eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(m_impl->eglDisplay, m_impl->eglSurface);
        }
        eglTerminate(m_impl->eglDisplay);
    }

    if (m_impl->dib) {
        IDIB_Release(m_impl->dib);
    }
    if (m_impl->deviceBitmap) {
        IBITMAP_Release(m_impl->deviceBitmap);
    }
    if (m_impl->display) {
        IDISPLAY_Release(m_impl->display);
    }
    if (m_impl->iegl) {
        IEGL10_Release(m_impl->iegl);
    }
    if (m_impl->glesBindingsLoaded) {
        GLES_Release();
    }
    if (m_impl->eglBindingsLoaded) {
        EGL_Release();
    }

    debug::MemoryTracker::Free(debug::MemorySubsystem::Graphics, m_impl);
    m_impl = nullptr;
}

Result GLContext::MakeCurrent() {
    if (!IsValid()) {
        return Result::NotInitialized;
    }
    return eglMakeCurrent(m_impl->eglDisplay, m_impl->eglSurface,
                          m_impl->eglSurface, m_impl->eglContext)
               ? Result::Ok
               : Result::GraphicsError;
}

Result GLContext::SwapBuffers() {
    if (!IsValid()) {
        return Result::NotInitialized;
    }
    debug::DebugOverlay::Render(m_impl->renderWidth, m_impl->renderHeight);
    return eglSwapBuffers(m_impl->eglDisplay, m_impl->eglSurface)
               ? Result::Ok
               : Result::GraphicsError;
}

Result GLContext::SetSwapInterval(int32_t interval) {
    if (!IsValid() || !m_impl->iegl) {
        return Result::NotInitialized;
    }
    IEGLOESSwapInterval* swapExt = nullptr;
    if (IEGL10_QueryInterface(m_impl->iegl, AEEIID_EGLOESSWAPINTERVAL,
                              reinterpret_cast<void**>(&swapExt)) != AEE_SUCCESS || !swapExt) {
        return Result::NotSupported;
    }
    AEEEGLBoolean ok = EGL_FALSE;
    IEGLOESSWAPINTERVAL_SwapIntervalOES(swapExt,
                                        reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                        static_cast<AEEEGLint>(interval),
                                        &ok);
    IEGLOESSWAPINTERVAL_Release(swapExt);
    return (ok != EGL_FALSE) ? Result::Ok : Result::GraphicsError;
}

static IEGLSurfaceManip* QuerySurfaceManip(IEGL10* iegl) {
    if (!iegl) {
        return nullptr;
    }
    IEGLSurfaceManip* manip = nullptr;
    if (IEGL10_QueryInterface(iegl, AEEIID_EGLSURFACEMANIP,
                              reinterpret_cast<void**>(&manip)) == AEE_SUCCESS && manip) {
        return manip;
    }
    if (IEGL10_QueryInterface(iegl, AEEIID_EGLSURFACEMANIP_V1,
                              reinterpret_cast<void**>(&manip)) == AEE_SUCCESS && manip) {
        return manip;
    }
    return nullptr;
}

Result GLContext::SetSurfaceScale(const Rect& srcRect, const Rect& dstRect, bool enable) {
    if (!IsValid() || !m_impl->iegl) {
        return Result::NotInitialized;
    }
    IEGLSurfaceManip* manip = QuerySurfaceManip(m_impl->iegl);
    if (!manip) {
        return Result::NotSupported;
    }

    AEEEGLSurfaceScaleRect src = {
        static_cast<AEEEGLint>(srcRect.x),
        static_cast<AEEEGLint>(srcRect.y),
        static_cast<AEEEGLint>(srcRect.width),
        static_cast<AEEEGLint>(srcRect.height)
    };
    AEEEGLSurfaceScaleRect dst = {
        static_cast<AEEEGLint>(dstRect.x),
        static_cast<AEEEGLint>(dstRect.y),
        static_cast<AEEEGLint>(dstRect.width),
        static_cast<AEEEGLint>(dstRect.height)
    };

    /* Zeebo Developer Guide 8.3: QUALCOMM_surface_scale only applies to non-current surfaces */
    eglMakeCurrent(m_impl->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    AEEEGLBoolean ok = EGL_FALSE;
    IEGLSURFACEMANIP_SetSurfaceScale(manip,
                                     reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                     reinterpret_cast<AEEEGLSurface>(m_impl->eglSurface),
                                     &src, &dst, &ok);
    if (ok != EGL_FALSE) {
        IEGLSURFACEMANIP_SurfaceScaleEnable(manip,
                                            reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                            reinterpret_cast<AEEEGLSurface>(m_impl->eglSurface),
                                            enable ? EGL_TRUE : EGL_FALSE, &ok);
    }

    eglMakeCurrent(m_impl->eglDisplay, m_impl->eglSurface, m_impl->eglSurface, m_impl->eglContext);
    IEGLSURFACEMANIP_Release(manip);
    return (ok != EGL_FALSE) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::EnableSurfaceScale(bool enable) {
    if (!IsValid() || !m_impl->iegl) {
        return Result::NotInitialized;
    }
    IEGLSurfaceManip* manip = QuerySurfaceManip(m_impl->iegl);
    if (!manip) {
        return Result::NotSupported;
    }
    eglMakeCurrent(m_impl->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    AEEEGLBoolean ok = EGL_FALSE;
    IEGLSURFACEMANIP_SurfaceScaleEnable(manip,
                                        reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                        reinterpret_cast<AEEEGLSurface>(m_impl->eglSurface),
                                        enable ? EGL_TRUE : EGL_FALSE, &ok);
    eglMakeCurrent(m_impl->eglDisplay, m_impl->eglSurface, m_impl->eglSurface, m_impl->eglContext);
    IEGLSURFACEMANIP_Release(manip);
    return (ok != EGL_FALSE) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::SetSurfaceRotate(bool rot90, bool hMirror, bool vMirror, bool enable) {
    if (!IsValid() || !m_impl->iegl) {
        return Result::NotInitialized;
    }
    IEGLSurfaceManip* manip = QuerySurfaceManip(m_impl->iegl);
    if (!manip) {
        return Result::NotSupported;
    }
    eglMakeCurrent(m_impl->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    AEEEGLBoolean ok = EGL_FALSE;
    IEGLSURFACEMANIP_SetSurfaceRotate(manip,
                                      reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                      reinterpret_cast<AEEEGLSurface>(m_impl->eglSurface),
                                      rot90 ? EGL_TRUE : EGL_FALSE,
                                      hMirror ? EGL_TRUE : EGL_FALSE,
                                      vMirror ? EGL_TRUE : EGL_FALSE,
                                      &ok);
    if (ok != EGL_FALSE) {
        IEGLSURFACEMANIP_SurfaceRotateEnable(manip,
                                             reinterpret_cast<AEEEGLDisplay>(m_impl->eglDisplay),
                                             reinterpret_cast<AEEEGLSurface>(m_impl->eglSurface),
                                             enable ? EGL_TRUE : EGL_FALSE, &ok);
    }
    eglMakeCurrent(m_impl->eglDisplay, m_impl->eglSurface, m_impl->eglSurface, m_impl->eglContext);
    IEGLSURFACEMANIP_Release(manip);
    return (ok != EGL_FALSE) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::GetPowerLevel(int32_t& outPowerLevel) const {
    outPowerLevel = 0;
    if (!IsValid() || !m_impl->iegl) {
        return Result::NotInitialized;
    }
    IEGLGetPowerLevel* pwr = nullptr;
    if (IEGL10_QueryInterface(m_impl->iegl, AEEIID_EGLGETPOWERLEVEL,
                              reinterpret_cast<void**>(&pwr)) != AEE_SUCCESS || !pwr) {
        return Result::NotSupported;
    }
    int level = 0;
    const int res = IEGLGETPOWERLEVEL_GetPowerLevel(pwr, &level);
    IEGLGETPOWERLEVEL_Release(pwr);
    outPowerLevel = static_cast<int32_t>(level);
    return (res == AEE_SUCCESS) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::SetRenderResolution(int32_t renderWidth, int32_t renderHeight) {
    if (!IsValid()) {
        return Result::NotInitialized;
    }
    if (renderWidth <= 0 || renderHeight <= 0) {
        return Result::InvalidArg;
    }
    if (renderWidth == m_impl->width && renderHeight == m_impl->height) {
        m_impl->renderWidth  = m_impl->width;
        m_impl->renderHeight = m_impl->height;
        return EnableSurfaceScale(false);
    }
    const Rect src = {0, 0, renderWidth, renderHeight};
    const Rect dst = {0, 0, m_impl->width, m_impl->height};
    const Result r = SetSurfaceScale(src, dst, true);
    if (Succeeded(r)) {
        m_impl->renderWidth  = renderWidth;
        m_impl->renderHeight = renderHeight;
    }
    return r;
}

int32_t GLContext::GetWidth() const {
    return m_impl ? m_impl->width : 0;
}

int32_t GLContext::GetHeight() const {
    return m_impl ? m_impl->height : 0;
}

int32_t GLContext::GetRenderWidth() const {
    return m_impl ? m_impl->renderWidth : 0;
}

int32_t GLContext::GetRenderHeight() const {
    return m_impl ? m_impl->renderHeight : 0;
}

bool GLContext::IsValid() const {
    return m_impl && m_impl->eglContext != EGL_NO_CONTEXT && m_impl->eglSurface != EGL_NO_SURFACE;
}

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
