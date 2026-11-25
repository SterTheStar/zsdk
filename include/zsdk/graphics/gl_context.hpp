/**
 * @file gl_context.hpp
 * @brief EGL 1.1 / OpenGL ES 1.1 context abstraction over BREW (EGL_1x.h / GLES_1x.h).
 */

#ifndef ZSDK_GRAPHICS_GL_CONTEXT_HPP
#define ZSDK_GRAPHICS_GL_CONTEXT_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

/**
 * @brief Surface & EGL configuration parameters.
 */
struct GLContextConfig {
    uint8_t redBits;        /**< Default 5 or 8 */
    uint8_t greenBits;      /**< Default 6 or 8 */
    uint8_t blueBits;       /**< Default 5 or 8 */
    uint8_t alphaBits;      /**< Default 0 or 8 */
    uint8_t depthBits;      /**< Default 16 */
    uint8_t stencilBits;    /**< Default 0 */
    uint8_t swapInterval;   /**< 1 = VSync on, 0 = off */
};

/**
 * @brief Manages native EGLDisplay, EGLSurface, and EGLContext without exposing EGL types.
 */
class GLContext : public NonCopyable {
public:
    GLContext();
    ~GLContext();

    /**
     * @brief Initializes EGL/GLES bindings (EGL_Init / GLES_Init) and creates the window surface.
     * @param platformContext Opaque IShell* (from Application::GetNativePlatformContext()).
     * @param config Desired framebuffer bit depths.
     */
    Result Initialize(void* platformContext, const GLContextConfig& config);

    /** @brief Destroys EGL context/surface and releases BREW GLES/EGL bindings. */
    void Shutdown();

    /** @brief Binds this context to the current thread. */
    Result MakeCurrent();

    /** @brief Presents the backbuffer (eglSwapBuffers). */
    Result SwapBuffers();

    /** @brief Configures swap interval / VSync (eglSwapInterval). */
    Result SetSwapInterval(int32_t interval);

    /**
     * @brief Qualcomm Q3Dimension hardware surface scaling ("esticar tela" / upscaling).
     *        Wraps IEGLSurfaceManip_V1::SetSurfaceScale & SurfaceScaleEnable.
     */
    Result SetSurfaceScale(const Rect& srcRect, const Rect& dstRect, bool enable = true);
    Result EnableSurfaceScale(bool enable);

    /**
     * @brief Qualcomm Q3Dimension hardware surface rotation & mirroring.
     *        Wraps IEGLSurfaceManip_V1::SetSurfaceRotate & SurfaceRotateEnable.
     */
    Result SetSurfaceRotate(bool rot90, bool hMirror, bool vMirror, bool enable = true);

    /** @brief Queries Q3Dimension GPU power level (IEGLGetPowerLevel). */
    Result GetPowerLevel(int32_t& outPowerLevel) const;

    /**
     * @brief Convenience helper to change the internal render resolution (e.g. 320x240)
     *        and hardware-stretch it to the native display size (e.g. 640x480) via
     *        Qualcomm IEGLSurfaceManip_V1. Pass native width/height to disable scaling.
     */
    Result SetRenderResolution(int32_t renderWidth, int32_t renderHeight);

    /**
     * @brief True if @p name (e.g. "GL_OES_draw_texture") is in GL_EXTENSIONS. Needs a current
     *        context. Titles on this platform probe extensions before enabling a path
     *        (draw_texture blits, ATITC textures, FBOs) and must fall back when one is absent.
     */
    bool HasExtension(const char* name) const;

    /** @brief Native display surface width and height in pixels. */
    int32_t GetWidth() const;
    int32_t GetHeight() const;

    /** @brief Active internal render width/height (equals GetWidth/GetHeight unless scaled). */
    int32_t GetRenderWidth() const;
    int32_t GetRenderHeight() const;
    bool    IsValid() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_GL_CONTEXT_HPP */
