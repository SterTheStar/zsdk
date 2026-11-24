/**
 * @file debug_overlay.hpp
 * @brief In-game OpenGL ES Debug Panel & RAM/VRAM Profiler Overlay.
 *
 * Activated/toggled simultaneously by pressing START + D-Pad Down on Player 1.
 * Renders directly via OpenGL ES 1.x using an embedded 8x8 bitmap font in ROM/RAM
 * with zero external asset dependencies.
 *
 * When ZSDK_ENABLE_DEBUG_OVERLAY == 0, all methods compile down to empty inline no-ops.
 */

#ifndef ZSDK_DEBUG_DEBUG_OVERLAY_HPP
#define ZSDK_DEBUG_DEBUG_OVERLAY_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/debug/memory_tracker.hpp"

namespace zsdk {
namespace input {
class InputManager;
} // namespace input

namespace debug {

/**
 * @brief Real-time frame and hardware metrics displayed by the overlay.
 */
struct OverlayMetrics {
    float    fps;
    float    frameTimeMs;
    uint32_t drawCalls;
    uint32_t activeTextures;
    MemorySnapshot memory;
};

/**
 * @brief In-game debug panel rendered over the active OpenGL ES framebuffer.
 */
class DebugOverlay {
public:
#if ZSDK_ENABLE_DEBUG_OVERLAY
    /** @brief Initializes internal ring-buffer and embedded font state. */
    static Result Initialize();

    /** @brief Releases overlay resources. */
    static void Shutdown();

    /**
     * @brief Polls the toggle combo (START + D-Pad Down) and updates frame metrics.
     * @param input Active InputManager instance.
     * @param deltaTimeMs Elapsed frame time in milliseconds.
     */
    static void Update(const input::InputManager& input, float deltaTimeMs);

    /**
     * @brief Renders the debug panel on top of the current GLES viewport if visible.
     * @param viewportWidth Current surface width in pixels (e.g., 640).
     * @param viewportHeight Current surface height in pixels (e.g., 480).
     */
    static void Render(int32_t viewportWidth, int32_t viewportHeight);

    /** @brief Appends a formatted line to the on-screen log console ring buffer. */
    static void PushLog(const char* message);

    /** @brief Programmatically shows or hides the overlay. */
    static void SetVisible(bool visible);

    /** @brief Returns true if the overlay is currently visible. */
    static bool IsVisible();

    /** @brief Records a draw call (and optional vertex count) for the current frame's telemetry. */
    static void RecordDrawCall(uint32_t vertexCount = 0u);
#else
    static ZSDK_FORCE_INLINE Result Initialize() { return Result::Ok; }
    static ZSDK_FORCE_INLINE void Shutdown() {}
    static ZSDK_FORCE_INLINE void Update(const input::InputManager&, float) {}
    static ZSDK_FORCE_INLINE void Render(int32_t, int32_t) {}
    static ZSDK_FORCE_INLINE void PushLog(const char*) {}
    static ZSDK_FORCE_INLINE void SetVisible(bool) {}
    static ZSDK_FORCE_INLINE bool IsVisible() { return false; }
    static ZSDK_FORCE_INLINE void RecordDrawCall(uint32_t = 0u) {}
#endif
};

} // namespace debug
} // namespace zsdk

#endif /* ZSDK_DEBUG_DEBUG_OVERLAY_HPP */
