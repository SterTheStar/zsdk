/**
 * @file debug_overlay.cpp
 * @brief In-game OpenGL ES 1.1 Debug Panel & RAM/VRAM Profiler implementation.
 */

#include "zsdk/debug/debug_overlay.hpp"

#if ZSDK_ENABLE_DEBUG_OVERLAY

#include "zsdk/debug/embedded_font.hpp"
#include "zsdk/core/clock.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/input/input.hpp"
#include "../graphics/gl_platform.hpp"

#if ZSDK_PLATFORM_ZEEBO
    #include "AEEStdLib.h"
    #define ZSDK_SNPRINTF SNPRINTF
#else
    #include <stdio.h>
    #include <string.h>
    #define ZSDK_SNPRINTF snprintf
#endif

namespace zsdk {
namespace debug {

namespace {

struct OverlayState {
    bool     initialized;
    bool     visible;
    float    fps;
    float    frameTimeMs;
    uint32_t lastTickMs;
    uint32_t fpsAccumMs;
    uint32_t fpsFrames;
    uint32_t drawCalls;
    uint32_t lastFrameDrawCalls;
    uint32_t vertices;
    uint32_t lastFrameVertices;

    char     logLines[ZSDK_DEBUG_LOG_CAPACITY][ZSDK_DEBUG_LOG_LINE_LEN];
    uint32_t logHead;
    uint32_t logCount;
};

OverlayState g_overlay = {};

void DrawSolidRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const GLfloat verts[8] = {
        x,     y,
        x + w, y,
        x,     y + h,
        x + w, y + h
    };

    glDisable(GL_TEXTURE_2D);
    const GLfloat inv = 1.0f / 255.0f;
    glColor4f(static_cast<GLfloat>(r) * inv, static_cast<GLfloat>(g) * inv,
              static_cast<GLfloat>(b) * inv, static_cast<GLfloat>(a) * inv);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void DrawTextLine(float startX, float startY, float scale, const char* text,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const GLuint fontTex = static_cast<GLuint>(graphics::Renderer::GetSharedFontTextureId());
    if (!text || fontTex == 0u) {
        return;
    }

    /* Static batch buffer (avoids blowing BREW's small applet stack). */
    static const uint32_t kMaxChars = ZSDK_DEBUG_LOG_LINE_LEN;
    static GLfloat verts[kMaxChars * 12];
    static GLfloat uvs[kMaxChars * 12];

    uint32_t charCount = 0;
    float cursorX = startX;
    const float adv = 8.0f * scale;
    const float gh  = 8.0f * scale;

    const float du = 8.0f / 128.0f;
    const float dv = 8.0f / 64.0f;

    for (const char* p = text; *p != '\0' && charCount < kMaxChars; ++p) {
        uint8_t uc = static_cast<uint8_t>(*p);
        if (uc < 0x20u || uc > 0x7Fu) {
            uc = 0x20u;
        }
        const uint32_t idx = static_cast<uint32_t>(uc - 0x20u);
        const uint32_t col = idx & 15u;
        const uint32_t row = idx >> 4u;

        const float u0 = static_cast<float>(col) * du;
        const float v0 = static_cast<float>(row) * dv;
        const float u1 = u0 + du;
        const float v1 = v0 + dv;

        const float x0 = cursorX;
        const float y0 = startY;
        const float x1 = x0 + adv;
        const float y1 = y0 + gh;

        const uint32_t vi = charCount * 12u;
        // Triangle 1
        verts[vi + 0] = x0; verts[vi + 1] = y0;
        verts[vi + 2] = x1; verts[vi + 3] = y0;
        verts[vi + 4] = x0; verts[vi + 5] = y1;
        // Triangle 2
        verts[vi + 6] = x1; verts[vi + 7] = y0;
        verts[vi + 8] = x1; verts[vi + 9] = y1;
        verts[vi + 10] = x0; verts[vi + 11] = y1;

        uvs[vi + 0] = u0; uvs[vi + 1] = v0;
        uvs[vi + 2] = u1; uvs[vi + 3] = v0;
        uvs[vi + 4] = u0; uvs[vi + 5] = v1;

        uvs[vi + 6] = u1; uvs[vi + 7] = v0;
        uvs[vi + 8] = u1; uvs[vi + 9] = v1;
        uvs[vi + 10] = u0; uvs[vi + 11] = v1;

        cursorX += adv;
        ++charCount;
    }

    if (charCount == 0) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    const GLfloat inv = 1.0f / 255.0f;
    glColor4f(static_cast<GLfloat>(r) * inv, static_cast<GLfloat>(g) * inv,
              static_cast<GLfloat>(b) * inv, static_cast<GLfloat>(a) * inv);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(charCount * 6u));
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

} // namespace

Result DebugOverlay::Initialize() {
    g_overlay.initialized = true;
    g_overlay.lastTickMs  = Clock::GetUptimeMs();
    g_overlay.fpsAccumMs  = 0u;
    g_overlay.fpsFrames   = 0u;
    g_overlay.fps         = 0.0f;
    g_overlay.frameTimeMs = 0.0f;
    return Result::Ok;
}

void DebugOverlay::Shutdown() {
    if (!g_overlay.initialized) {
        return;
    }
    graphics::Renderer::ReleaseSharedFontTexture();
    g_overlay.initialized = false;
}

void DebugOverlay::Update(const input::InputManager& input, float deltaTimeMs) {
    /* Shortcut: L + D-Pad Down toggles the panel (works reliably on real Zeebo Z-Pad and Windows). */
    const uint32_t toggleCombo = input::ButtonMask(input::GamepadButton::ShoulderLeft) |
                                 input::ButtonMask(input::GamepadButton::DPadDown);
    const auto& pad0 = input.GetGamepad(0);
    if (pad0.IsComboJustTriggered(toggleCombo)) {
        g_overlay.visible = !g_overlay.visible;
    }

    const uint32_t nowMs = Clock::GetUptimeMs();
    const uint32_t rawDt = (nowMs >= g_overlay.lastTickMs) ? (nowMs - g_overlay.lastTickMs) : 0u;
    g_overlay.lastTickMs = nowMs;

    g_overlay.fpsAccumMs += rawDt;
    ++g_overlay.fpsFrames;
    if (g_overlay.fpsAccumMs >= 500u) {
        g_overlay.fps = (static_cast<float>(g_overlay.fpsFrames) * 1000.0f) /
                        static_cast<float>(g_overlay.fpsAccumMs);
        g_overlay.frameTimeMs = static_cast<float>(g_overlay.fpsAccumMs) /
                                static_cast<float>(g_overlay.fpsFrames);
        g_overlay.fpsAccumMs = 0u;
        g_overlay.fpsFrames  = 0u;
    } else if (g_overlay.fps <= 0.0f && deltaTimeMs > 0.01f) {
        g_overlay.frameTimeMs = deltaTimeMs;
        g_overlay.fps = 1000.0f / deltaTimeMs;
    }
    g_overlay.lastFrameDrawCalls = g_overlay.drawCalls;
    g_overlay.drawCalls = 0u;
    g_overlay.lastFrameVertices = g_overlay.vertices;
    g_overlay.vertices = 0u;
}

void DebugOverlay::Render(int32_t viewportWidth, int32_t viewportHeight) {
    if (!g_overlay.initialized || !g_overlay.visible || viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    (void)graphics::Renderer::GetSharedFontTextureId();

    /* Save minimal GLES 1.1 state and isolate overlay from 3D game state */
    glViewport(0, 0, static_cast<GLsizei>(viewportWidth), static_cast<GLsizei>(viewportHeight));
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0.0f, static_cast<GLfloat>(viewportWidth),
             static_cast<GLfloat>(viewportHeight), 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, static_cast<GLfloat>(GL_MODULATE));
    glEnableClientState(GL_VERTEX_ARRAY);

    MemorySnapshot mem;
    MemoryTracker::GetSnapshot(mem);

    const uint32_t totalUsed = mem.totalRamBytes + mem.totalVramBytes;
    const uint32_t budgetMb  = mem.budgetBytes / (1024u * 1024u);
    const uint32_t usedPct10 = (mem.budgetBytes > 0u)
                                   ? static_cast<uint32_t>((static_cast<uint64_t>(totalUsed) * 1000u) / mem.budgetBytes)
                                   : 0u;
    const uint32_t fps10     = static_cast<uint32_t>(g_overlay.fps * 10.0f);
    const uint32_t ft10      = static_cast<uint32_t>(g_overlay.frameTimeMs * 10.0f);

    const float panelX = 8.0f;
    const float panelY = 8.0f;
    const float maxW = static_cast<float>(viewportWidth) - 16.0f;
    const float panelW = (maxW > 460.0f) ? 460.0f : maxW;
    const float panelH = (viewportHeight < 300) ? 130.0f : 152.0f;

    /* Translucent dark backdrop */
    DrawSolidRect(panelX, panelY, panelW, panelH, 10, 12, 18, 210);

    char line[ZSDK_DEBUG_LOG_LINE_LEN];
    ZSDK_SNPRINTF(line, sizeof(line), "FPS:%u.%u (%u.%ums) Drw:%u Vtx:%u",
                  fps10 / 10u, fps10 % 10u, ft10 / 10u, ft10 % 10u,
                  g_overlay.lastFrameDrawCalls,
                  g_overlay.lastFrameVertices);
    DrawTextLine(panelX + 8.0f, panelY + 6.0f, 1.0f, line, 120, 230, 255, 255);

    ZSDK_SNPRINTF(line, sizeof(line), "MEM:%uK/%uM (%u.%u%%) Peak:%uK",
                  totalUsed / 1024u, budgetMb, usedPct10 / 10u, usedPct10 % 10u,
                  mem.peakRamBytes / 1024u);
    DrawTextLine(panelX + 8.0f, panelY + 18.0f, 1.0f, line, 255, 255, 255, 255);

    /* Budget progress bar */
    const float barX = panelX + 8.0f;
    const float barY = panelY + 29.0f;
    const float barW = panelW - 16.0f;
    const float barH = 6.0f;
    DrawSolidRect(barX, barY, barW, barH, 40, 45, 55, 255);
    const float fillRatio = (usedPct10 > 1000u) ? 1.0f : (static_cast<float>(usedPct10) / 1000.0f);
    const uint8_t barR = (usedPct10 > 850u) ? 235u : ((usedPct10 > 650u) ? 240u : 70u);
    const uint8_t barG = (usedPct10 > 850u) ? 60u  : ((usedPct10 > 650u) ? 190u : 210u);
    DrawSolidRect(barX, barY, barW * fillRatio, barH, barR, barG, 80, 255);

    ZSDK_SNPRINTF(line, sizeof(line), "VRAM:%uK CORE:%uK GFX:%uK AUD:%uK FS:%uK",
                  mem.totalVramBytes / 1024u,
                  mem.subsystems[0].currentBytes / 1024u,
                  mem.subsystems[1].currentBytes / 1024u,
                  mem.subsystems[2].currentBytes / 1024u,
                  mem.subsystems[3].currentBytes / 1024u);
    DrawTextLine(panelX + 8.0f, panelY + 39.0f, 1.0f, line, 190, 220, 190, 255);

    /* Recent logs (up to 6 lines) */
    const uint32_t maxVisibleLogs = (viewportHeight < 300) ? 5u : 7u;
    const uint32_t toShow = (g_overlay.logCount < maxVisibleLogs) ? g_overlay.logCount : maxVisibleLogs;
    for (uint32_t i = 0; i < toShow; ++i) {
        const uint32_t idx = (g_overlay.logHead + ZSDK_DEBUG_LOG_CAPACITY - toShow + i) % ZSDK_DEBUG_LOG_CAPACITY;
        DrawTextLine(panelX + 8.0f, panelY + 54.0f + static_cast<float>(i) * 11.0f,
                     1.0f, g_overlay.logLines[idx], 220, 220, 220, 255);
    }

    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    graphics::Renderer::ResetStateCache();
}

void DebugOverlay::PushLog(const char* message) {
    if (!message) {
        return;
    }
    uint32_t i = 0;
    for (; i < ZSDK_DEBUG_LOG_LINE_LEN - 1u && message[i] != '\0'; ++i) {
        g_overlay.logLines[g_overlay.logHead][i] = message[i];
    }
    g_overlay.logLines[g_overlay.logHead][i] = '\0';

    g_overlay.logHead = (g_overlay.logHead + 1u) % ZSDK_DEBUG_LOG_CAPACITY;
    if (g_overlay.logCount < ZSDK_DEBUG_LOG_CAPACITY) {
        ++g_overlay.logCount;
    }
}

void DebugOverlay::SetVisible(bool visible) {
    g_overlay.visible = visible;
}

bool DebugOverlay::IsVisible() {
    return g_overlay.visible;
}

void DebugOverlay::RecordDrawCall(uint32_t vertexCount) {
    ++g_overlay.drawCalls;
    g_overlay.vertices += vertexCount;
}

} // namespace debug
} // namespace zsdk

#endif /* ZSDK_ENABLE_DEBUG_OVERLAY */
