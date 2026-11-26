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

static const uint32_t kFrameHistSize = 44u;

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

    float    frameHistory[kFrameHistSize];
    uint32_t frameHistHead;
    uint32_t frameHistCount;

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

void DrawFrametimeGraph(float gx, float gy, float gw, float gh) {
    DrawSolidRect(gx, gy, gw, gh, 0, 0, 0, 100);

    const float maxMs = 50.0f;
    const float baseX = gx + 1.0f;
    const float baseY = gy + gh - 1.0f;
    const float innerW = gw - 2.0f;
    const float innerH = gh - 2.0f;

    /* 16.7ms (60fps) and 33.3ms (30fps) faint reference lines */
    const float ref60Y = baseY - (16.67f / maxMs) * innerH;
    const float ref30Y = baseY - (33.33f / maxMs) * innerH;
    DrawSolidRect(baseX, ref60Y, innerW, 1.0f, 255, 255, 255, 24);
    DrawSolidRect(baseX, ref30Y, innerW, 1.0f, 255, 255, 255, 36);

    const float step = innerW / static_cast<float>(kFrameHistSize);
    const uint32_t count = g_overlay.frameHistCount;
    const uint32_t startSlot = (kFrameHistSize > count) ? (kFrameHistSize - count) : 0u;

    for (uint32_t i = 0u; i < count; ++i) {
        const uint32_t idx = (g_overlay.frameHistHead + kFrameHistSize - count + i) % kFrameHistSize;
        float ms = g_overlay.frameHistory[idx];
        if (ms < 0.0f) ms = 0.0f;
        if (ms > maxMs) ms = maxMs;
        float bh = (ms / maxMs) * innerH;
        if (bh < 1.0f) bh = 1.0f;

        uint8_t br = 90u,  bg = 185u, bb = 125u;
        uint8_t cr = 165u, cg = 235u, cb = 185u;
        if (ms > 44.0f) {
            br = 210u; bg = 80u;  bb = 75u;
            cr = 250u; cg = 140u; cb = 135u;
        } else if (ms > 35.0f) {
            br = 215u; bg = 165u; bb = 65u;
            cr = 250u; cg = 210u; cb = 120u;
        }

        const float bx = baseX + static_cast<float>(startSlot + i) * step;
        const float bw = (step >= 1.0f) ? step : 1.0f;
        DrawSolidRect(bx, baseY - bh, bw, bh, br, bg, bb, 220);
        DrawSolidRect(bx, baseY - bh, bw, 1.0f, cr, cg, cb, 255);
    }
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
    const float sampleMs = (rawDt > 0u) ? static_cast<float>(rawDt) : deltaTimeMs;
    if (sampleMs > 0.0f) {
        g_overlay.frameHistory[g_overlay.frameHistHead] = sampleMs;
        g_overlay.frameHistHead = (g_overlay.frameHistHead + 1u) % kFrameHistSize;
        if (g_overlay.frameHistCount < kFrameHistSize) {
            ++g_overlay.frameHistCount;
        }
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
    const float panelW = (maxW > 320.0f) ? 320.0f : maxW;
    const uint32_t maxVisualLines = (viewportHeight < 300) ? 4u : 6u;
    const float lineStep = 10.0f;
    const float logsStartY = panelY + 72.0f;
    const float panelH = 72.0f + static_cast<float>(maxVisualLines) * lineStep + 5.0f;

    /* Clean flat dark backdrop */
    DrawSolidRect(panelX, panelY, panelW, panelH, 8, 10, 14, 196);

    const float pad = 7.0f;
    const float innerW = panelW - pad * 2.0f;
    char line[ZSDK_DEBUG_LOG_LINE_LEN];

    uint8_t fpsR = 235u, fpsG = 240u, fpsB = 245u;
    if (g_overlay.fps < 28.0f) {
        fpsR = 235u; fpsG = 95u; fpsB = 90u;
    } else if (g_overlay.fps < 55.0f && g_overlay.fps >= 30.0f) {
        fpsR = 120u; fpsG = 225u; fpsB = 155u;
    }

    /* Top row (panelY + 6 .. panelY + 30, 24px tall) */
    ZSDK_SNPRINTF(line, sizeof(line), "%u.%u", fps10 / 10u, fps10 % 10u);
    DrawTextLine(panelX + pad, panelY + 10.0f, 2.0f, line, fpsR, fpsG, fpsB, 255);

    const float fpsNumW = static_cast<float>((fps10 >= 1000u) ? 5u : ((fps10 >= 100u) ? 4u : 3u)) * 16.0f;
    const float metaX = panelX + pad + fpsNumW + 4.0f;
    ZSDK_SNPRINTF(line, sizeof(line), "FPS %u.%ums", ft10 / 10u, ft10 % 10u);
    DrawTextLine(metaX, panelY + 9.0f, 1.0f, line, 175, 185, 198, 255);

    ZSDK_SNPRINTF(line, sizeof(line), "D:%u V:%u",
                  g_overlay.lastFrameDrawCalls, g_overlay.lastFrameVertices);
    DrawTextLine(metaX, panelY + 19.0f, 1.0f, line, 140, 152, 168, 255);

    const float graphW = (innerW > 220.0f) ? 134.0f : (innerW * 0.42f);
    if (graphW >= 48.0f && metaX + 76.0f < panelX + panelW - pad - graphW) {
        DrawFrametimeGraph(panelX + panelW - pad - graphW, panelY + 6.0f, graphW, 24.0f);
    }

    /* Memory section */
    ZSDK_SNPRINTF(line, sizeof(line), "MEM %uK/%uM (%u.%u%%) PK:%uK",
                  totalUsed / 1024u, budgetMb, usedPct10 / 10u, usedPct10 % 10u,
                  mem.peakRamBytes / 1024u);
    DrawTextLine(panelX + pad, panelY + 35.0f, 1.0f, line, 232, 236, 242, 255);

    const float barX = panelX + pad;
    const float barY = panelY + 46.0f;
    const float barW = innerW;
    const float barH = 4.0f;
    DrawSolidRect(barX, barY, barW, barH, 255, 255, 255, 26);

    /* Color-coded stacked segments: VRAM, Core, Gfx, Audio, FS, Other */
    struct MemSeg {
        uint32_t bytes;
        uint8_t  r, g, b;
        char     tag;
    };
    const uint32_t trackedSubs = mem.subsystems[0].currentBytes + mem.subsystems[1].currentBytes +
                                 mem.subsystems[2].currentBytes + mem.subsystems[3].currentBytes;
    const uint32_t otherBytes  = (mem.totalRamBytes > trackedSubs) ? (mem.totalRamBytes - trackedSubs) : 0u;
    const MemSeg segs[6] = {
        { mem.totalVramBytes,             170u, 140u, 235u, 'V' }, /* VRAM: violet */
        { mem.subsystems[0].currentBytes, 100u, 180u, 240u, 'C' }, /* Core: blue */
        { mem.subsystems[1].currentBytes,  95u, 200u, 140u, 'G' }, /* Gfx: green */
        { mem.subsystems[2].currentBytes, 235u, 185u,  85u, 'A' }, /* Audio: amber */
        { mem.subsystems[3].currentBytes, 230u, 120u, 110u, 'F' }, /* FS: coral */
        { otherBytes,                     150u, 160u, 175u, 'O' }  /* Other: slate */
    };

    if (mem.budgetBytes > 0u) {
        float curX = barX;
        const float barEnd = barX + barW;
        for (int s = 0; s < 6; ++s) {
            if (segs[s].bytes == 0u || curX >= barEnd) {
                continue;
            }
            float segW = barW * (static_cast<float>(segs[s].bytes) / static_cast<float>(mem.budgetBytes));
            if (segW < 2.0f) {
                segW = 2.0f;
            }
            if (curX + segW > barEnd) {
                segW = barEnd - curX;
            }
            DrawSolidRect(curX, barY, segW, barH, segs[s].r, segs[s].g, segs[s].b, 255);
            curX += segW;
        }
    }

    float legX = panelX + pad;
    const float legY = panelY + 54.0f;
    for (int s = 0; s < 5; ++s) {
        if (s == 0) {
            ZSDK_SNPRINTF(line, sizeof(line), "VRAM:%uK", segs[s].bytes / 1024u);
        } else {
            ZSDK_SNPRINTF(line, sizeof(line), "%c:%uK", segs[s].tag, segs[s].bytes / 1024u);
        }
        DrawTextLine(legX, legY, 1.0f, line, segs[s].r, segs[s].g, segs[s].b, 255);
        uint32_t len = 0u;
        while (line[len] != '\0') {
            ++len;
        }
        legX += static_cast<float>(len) * 8.0f + ((s == 0) ? 10.0f : 6.0f);
    }

    /* Subtle hairline above logs */
    DrawSolidRect(panelX + pad, panelY + 67.0f, innerW, 1.0f, 255, 255, 255, 22);

    /* Wrap recent logs into at most 2 visual lines per entry (truncate remainder with "..."). */
    uint32_t maxChars = (innerW >= 24.0f) ? static_cast<uint32_t>(innerW / 8.0f) : 3u;
    if (maxChars >= ZSDK_DEBUG_LOG_LINE_LEN) {
        maxChars = ZSDK_DEBUG_LOG_LINE_LEN - 1u;
    }

    static char wrappedLines[8][ZSDK_DEBUG_LOG_LINE_LEN];
    static uint8_t wrappedKind[8];
    uint32_t visualCount = 0u;
    for (uint32_t age = 0u; age < g_overlay.logCount && visualCount < maxVisualLines; ++age) {
        const uint32_t idx = (g_overlay.logHead + ZSDK_DEBUG_LOG_CAPACITY - 1u - age) % ZSDK_DEBUG_LOG_CAPACITY;
        const char* src = g_overlay.logLines[idx];
        const uint8_t kind = (src[0] == '[' && src[1] == 'E') ? 2u
                           : ((src[0] == '[' && src[1] == 'W') ? 1u : 0u);
        uint32_t len = 0u;
        while (src[len] != '\0' && len < ZSDK_DEBUG_LOG_LINE_LEN - 1u) {
            ++len;
        }

        if (len <= maxChars) {
            for (uint32_t c = 0u; c < len; ++c) {
                wrappedLines[visualCount][c] = src[c];
            }
            wrappedLines[visualCount][len] = '\0';
            wrappedKind[visualCount] = kind;
            ++visualCount;
        } else {
            uint32_t split = maxChars;
            for (uint32_t c = maxChars; c > (maxChars > 10u ? maxChars - 10u : 0u); --c) {
                if (src[c] == ' ') {
                    split = c;
                    break;
                }
            }
            uint32_t nextStart = split;
            while (nextStart < len && src[nextStart] == ' ') {
                ++nextStart;
            }
            const uint32_t rem = (len > nextStart) ? (len - nextStart) : 0u;
            if (rem > 0u && visualCount + 2u <= maxVisualLines) {
                if (rem <= maxChars) {
                    for (uint32_t c = 0u; c < rem; ++c) {
                        wrappedLines[visualCount][c] = src[nextStart + c];
                    }
                    wrappedLines[visualCount][rem] = '\0';
                } else {
                    const uint32_t keep = (maxChars > 3u) ? (maxChars - 3u) : 0u;
                    for (uint32_t c = 0u; c < keep; ++c) {
                        wrappedLines[visualCount][c] = src[nextStart + c];
                    }
                    wrappedLines[visualCount][keep + 0u] = '.';
                    wrappedLines[visualCount][keep + 1u] = '.';
                    wrappedLines[visualCount][keep + 2u] = '.';
                    wrappedLines[visualCount][keep + 3u] = '\0';
                }
                wrappedKind[visualCount] = kind;
                ++visualCount;
                for (uint32_t c = 0u; c < split; ++c) {
                    wrappedLines[visualCount][c] = src[c];
                }
                wrappedLines[visualCount][split] = '\0';
                wrappedKind[visualCount] = kind;
                ++visualCount;
            } else {
                const uint32_t keep = (maxChars > 3u) ? (maxChars - 3u) : 0u;
                for (uint32_t c = 0u; c < keep; ++c) {
                    wrappedLines[visualCount][c] = src[c];
                }
                wrappedLines[visualCount][keep + 0u] = '.';
                wrappedLines[visualCount][keep + 1u] = '.';
                wrappedLines[visualCount][keep + 2u] = '.';
                wrappedLines[visualCount][keep + 3u] = '\0';
                wrappedKind[visualCount] = kind;
                ++visualCount;
            }
        }
    }

    for (uint32_t i = 0u; i < visualCount; ++i) {
        const uint32_t slot = visualCount - 1u - i;
        const uint8_t kind = wrappedKind[slot];
        const uint8_t lr = (kind == 2u) ? 255u : ((kind == 1u) ? 250u : 210u);
        const uint8_t lg = (kind == 2u) ? 120u : ((kind == 1u) ? 210u : 220u);
        const uint8_t lb = (kind == 2u) ? 115u : ((kind == 1u) ? 110u : 232u);
        DrawTextLine(panelX + pad, logsStartY + static_cast<float>(i) * lineStep,
                     1.0f, wrappedLines[slot], lr, lg, lb, 255);
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
