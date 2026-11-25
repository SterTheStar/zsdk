/**
 * @file frame.hpp
 * @brief RAII frame helper: clear + optional 2D pass + SwapBuffers in one scope.
 *
 *   void OnRender() override {
 *       zsdk::graphics::Frame frame(m_gl, zsdk::Color::Rgba(16, 20, 32, 255));
 *       // ... 3D drawing ...
 *       frame.Begin2D(m_batch, 640.0f, 480.0f);   // HUD / menu pass (depth test off)
 *       // ... batch / DrawText2D ...
 *   }   // End2D() + SwapBuffers() happen here, even on early return
 */

#ifndef ZSDK_GRAPHICS_FRAME_HPP
#define ZSDK_GRAPHICS_FRAME_HPP

#include "zsdk/graphics/gl_context.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/sprite_batch.hpp"

namespace zsdk {
namespace graphics {

class Frame : public NonCopyable {
public:
    Frame(GLContext& gl, const Color& clearColor, bool clearDepth = true)
        : m_gl(gl), m_batch(nullptr) {
        Renderer::SetViewport(0, 0, gl.GetRenderWidth(), gl.GetRenderHeight());
        Renderer::Clear(clearColor, clearDepth);
    }

    ~Frame() {
        End2D();
        m_gl.SwapBuffers();
    }

    int32_t Width()  const { return m_gl.GetRenderWidth(); }
    int32_t Height() const { return m_gl.GetRenderHeight(); }

    /** @brief Starts a 2D pass in a virtual resolution (depth test disabled). */
    void Begin2D(SpriteBatch2D& batch, float virtualW, float virtualH) {
        End2D();
        Renderer::SetDepthTest(false);
        batch.BeginVirtual(virtualW, virtualH, m_gl.GetRenderWidth(), m_gl.GetRenderHeight());
        m_batch = &batch;
    }

    void End2D() {
        if (m_batch != nullptr) {
            m_batch->End();
            m_batch = nullptr;
        }
    }

private:
    GLContext&     m_gl;
    SpriteBatch2D* m_batch;
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_FRAME_HPP */
