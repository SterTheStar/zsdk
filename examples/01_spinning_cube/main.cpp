/**
 * @file main.cpp
 * @brief ZSDK Example 01: 1..1024 Spinning Lit 3D Cubes + Frustum/Face Culling + SpriteBatch2D Menu.
 *
 * Pure ZSDK C++ API (zero BREW / OpenGL ES headers).
 *
 * Controls (Z-Pad):
 *   - START              : Open / Close Settings Menu
 *   - L + D-Pad Down     : Toggle Debug / RAM / VRAM Overlay
 *   - Left Stick / D-Pad : Pan camera & spin cubes (or navigate menu when open)
 *   - Button 1 (South)   : Toggle lighting (or activate/adjust item in menu)
 *   - Button 2 (West)    : Quick-toggle 30 / 60 FPS cap
 *   - Home               : Exit application
 */

#include "zsdk/zsdk.hpp"

namespace {

static const uint16_t kCubeCounts[11] = {
    1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u
};

static const zsdk::graphics::VertexP3N3C4 kCubeVertices[24] = {
    /* Front (+Z) - Cyan */
    {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    /* Back (-Z) - Orange */
    { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    /* Left (-X) - Green */
    {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    /* Right (+X) - Magenta */
    { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    /* Top (+Y) - Yellow */
    {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    /* Bottom (-Y) - Red */
    {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255}
};

static const uint16_t kCubeIndices[36] = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23
};

struct CubeSettings {
    uint8_t lighting;
    uint8_t fpsMode;        /* 0 = 60, 1 = 30 */
    uint8_t vsync;
    uint8_t spinMode;       /* 0 = Paused, 1 = Slow, 2 = Normal, 3 = Fast */
    uint8_t cubeCountIndex; /* 0..10 -> 1..1024 */
    uint8_t frustumCull;    /* 0 = OFF, 1 = ON */
    uint8_t faceCull;       /* 0 = OFF, 1 = ON */
};

static const uint8_t  kMenuItemCount = 10u;
static const uint32_t kSaveVersion   = 2u;

int AppendStr(char* dst, int pos, int maxLen, const char* src) {
    while (*src != '\0' && pos < maxLen - 1) {
        dst[pos++] = *src++;
    }
    dst[pos] = '\0';
    return pos;
}

int AppendUInt(char* dst, int pos, int maxLen, uint32_t val) {
    char tmp[12];
    int n = 0;
    if (val == 0u) {
        tmp[n++] = '0';
    } else {
        while (val > 0u && n < 11) {
            tmp[n++] = static_cast<char>('0' + (val % 10u));
            val /= 10u;
        }
    }
    while (n > 0 && pos < maxLen - 1) {
        dst[pos++] = tmp[--n];
    }
    dst[pos] = '\0';
    return pos;
}

void GetGridDims(uint8_t countIdx, uint16_t& cols, uint16_t& rows, float& camDist) {
    static const uint16_t kCols[11]   = {1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 16};
    static const uint16_t kRows[11]   = {1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8};
    static const float    kDist[11]   = {4.5f, 6.5f, 8.0f, 10.5f, 13.0f, 16.0f, 19.0f, 23.0f, 28.0f, 33.0f, 38.0f};
    const uint8_t idx = (countIdx <= 10u) ? countIdx : 10u;
    cols    = kCols[idx];
    rows    = kRows[idx];
    camDist = kDist[idx];
}

} // namespace

class SpinningCubeApp : public zsdk::IAppDelegate {
public:
    SpinningCubeApp()
        : m_angleX(25.0f)
        , m_angleY(35.0f)
        , m_camPanX(0.0f)
        , m_camPanY(0.0f)
        , m_camZoomOffset(0.0f)
        , m_lastDrawnCubes(1u)
        , m_menuOpen(false)
        , m_menuCursor(0u)
        , m_menuRepeatDir(0u)
        , m_menuRepeatTimerMs(0)
#if ZSDK_PLATFORM_ZEEBO
        , m_settings{1u, 1u, 1u, 2u, 0u, 1u, 1u} {} /* 30 FPS default on Zeebo TV-Out (Guide 11.9) */
#else
        , m_settings{1u, 0u, 1u, 2u, 0u, 1u, 1u} {}
#endif

    zsdk::Result OnInit() override {
        auto* app = zsdk::Application::GetInstance();
        zsdk::graphics::GLContextConfig cfg = {5, 6, 5, 0, 16, 0, 1};
        zsdk::Result res = m_gl.Initialize(app->GetNativePlatformContext(), cfg);
        if (zsdk::Failed(res)) {
            return res;
        }

        m_vbo.Create(zsdk::graphics::BufferType::Vertex, zsdk::graphics::BufferUsage::Static,
                     sizeof(kCubeVertices), kCubeVertices);
        m_ibo.Create(zsdk::graphics::BufferType::Index, zsdk::graphics::BufferUsage::Static,
                     sizeof(kCubeIndices), kCubeIndices);

        m_batch.Initialize(64u);

        zsdk::graphics::Renderer::ConfigureLight(
            0,
            zsdk::Vec3{6.0f, 8.0f, 10.0f},
            zsdk::Color::Rgba(64, 64, 80, 255),
            zsdk::Color::Rgba(242, 242, 230, 255),
            zsdk::Color::Rgba(204, 204, 204, 255));

        m_saveMgr.Initialize(app->GetNativePlatformContext());
        CubeSettings loaded;
        uint32_t bytesRead = 0u;
        if (zsdk::Succeeded(m_saveMgr.LoadSlot(0, &loaded, sizeof(loaded), bytesRead, kSaveVersion)) &&
            bytesRead == sizeof(loaded)) {
            if (loaded.cubeCountIndex > 10u) loaded.cubeCountIndex = 0u;
            loaded.spinMode    &= 3u;
            loaded.frustumCull = loaded.frustumCull ? 1u : 0u;
            loaded.faceCull    = loaded.faceCull ? 1u : 0u;
            m_settings = loaded;
            ApplySettings();
            ZSDK_LOG_INFO("CUBE", "Loaded settings from udata.");
        } else {
            ZSDK_LOG_INFO("CUBE", "Spinning Cube ready.");
        }
        return zsdk::Result::Ok;
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        auto* app = zsdk::Application::GetInstance();
        const auto& p1 = app->GetInput().GetGamepad(0);
        const float dt = static_cast<float>(deltaTimeMs) * 0.001f;

        /* START or Z-Pad HOME (without D-Pad Down) toggles the interactive menu */
        const bool menuBtnPressed = p1.IsPressed(zsdk::input::GamepadButton::Start) ||
                                    p1.IsPressed(zsdk::input::GamepadButton::Home);
        if (menuBtnPressed && !p1.IsDown(zsdk::input::GamepadButton::DPadDown)) {
            m_menuOpen = !m_menuOpen;
            ZSDK_LOG_INFO("MENU", "%s", m_menuOpen ? "Opened" : "Closed");
        }

        if (m_menuOpen) {
            UpdateMenu(p1, deltaTimeMs);
        } else {
            const float spinTable[4] = {0.0f, 20.0f, 48.0f, 110.0f};
            const float baseSpin = spinTable[m_settings.spinMode & 3u];
            const zsdk::Vec2 stick = p1.GetLeftStick();
            m_angleY += baseSpin * dt;
            m_angleX += (baseSpin * 0.65f) * dt;

            /* Left stick / D-Pad pans camera to test Frustum Culling live */
            float panX = stick.x;
            float panY = stick.y;
            if (p1.IsDown(zsdk::input::GamepadButton::DPadLeft))  panX -= 1.0f;
            if (p1.IsDown(zsdk::input::GamepadButton::DPadRight)) panX += 1.0f;
            if (p1.IsDown(zsdk::input::GamepadButton::DPadUp))    panY += 1.0f;
            if (p1.IsDown(zsdk::input::GamepadButton::DPadDown))  panY -= 1.0f;

            m_camPanX += panX * 16.0f * dt;
            m_camPanY += panY * 16.0f * dt;
            if (m_camPanX >  40.0f) m_camPanX =  40.0f;
            if (m_camPanX < -40.0f) m_camPanX = -40.0f;
            if (m_camPanY >  30.0f) m_camPanY =  30.0f;
            if (m_camPanY < -30.0f) m_camPanY = -30.0f;

            const zsdk::Vec2 rStick = p1.GetRightStick();
            m_camZoomOffset -= rStick.y * 20.0f * dt;
            if (p1.IsDown(zsdk::input::GamepadButton::ShoulderLeft))  m_camZoomOffset -= 14.0f * dt;
            if (p1.IsDown(zsdk::input::GamepadButton::ShoulderRight)) m_camZoomOffset += 14.0f * dt;
            if (m_camZoomOffset < -30.0f) m_camZoomOffset = -30.0f;
            if (m_camZoomOffset >  50.0f) m_camZoomOffset =  50.0f;

            if (p1.IsPressed(zsdk::input::GamepadButton::Action1)) {
                m_settings.lighting = (m_settings.lighting == 0u) ? 1u : 0u;
                ZSDK_LOG_INFO("CUBE", "Lighting: %s", m_settings.lighting ? "ON" : "OFF");
            }
            if (p1.IsPressed(zsdk::input::GamepadButton::Action2)) {
                m_settings.fpsMode = (m_settings.fpsMode == 1u) ? 0u : 1u;
                ApplyFps();
            }
        }
    }

    void OnRender() override {
        using zsdk::graphics::Renderer;
        const int32_t w = m_gl.GetRenderWidth();
        const int32_t h = m_gl.GetRenderHeight();

        Renderer::SetViewport(0, 0, w, h);
        Renderer::Clear(zsdk::Color::Rgba(16, 20, 32, 255), true);

        const float aspect = (h > 0) ? (static_cast<float>(w) / static_cast<float>(h)) : 1.333f;
        const float fovY   = 55.0f;
        const float nearZ  = 0.5f;
        const float farZ   = 150.0f;

        uint16_t cols = 1u;
        uint16_t rows = 1u;
        float camDist = 4.5f;
        GetGridDims(m_settings.cubeCountIndex, cols, rows, camDist);
        camDist += m_camZoomOffset;
        if (camDist < 2.5f)  camDist = 2.5f;
        if (camDist > 90.0f) camDist = 90.0f;

        const zsdk::Vec3 eye{m_camPanX, m_camPanY, camDist};
        const zsdk::Vec3 target{m_camPanX, m_camPanY, 0.0f};
        const zsdk::Vec3 up{0.0f, 1.0f, 0.0f};

        Renderer::SetPerspective(fovY, aspect, nearZ, farZ);
        Renderer::ResetModelView();
        Renderer::LookAt(eye, target, up);

        zsdk::graphics::Frustum frustum;
        if (m_settings.frustumCull != 0u) {
            frustum.ExtractFromCamera(eye, target, up, fovY, aspect, nearZ, farZ);
        }

        Renderer::SetDepthTest(true);
        Renderer::SetCullFace(m_settings.faceCull != 0u);
        Renderer::SetLighting(m_settings.lighting != 0u);

        Renderer::BindInterleavedBuffer(m_vbo, zsdk::graphics::VertexP3N3C4::InterleavedLayout());
        m_ibo.Bind();

        const uint16_t totalCubes = kCubeCounts[m_settings.cubeCountIndex];
        const uint16_t perLayer   = static_cast<uint16_t>(cols * rows);
        const uint16_t layers     = (totalCubes + perLayer - 1u) / perLayer;
        const float    spacing    = 3.0f;
        const float    halfW      = static_cast<float>(cols - 1u) * 0.5f * spacing;
        const float    halfH      = static_cast<float>(rows - 1u) * 0.5f * spacing;
        const float    halfD      = static_cast<float>(layers - 1u) * 0.5f * spacing;
        const float    cubeRadius = 1.75f;

        uint16_t drawn = 0u;
        for (uint16_t i = 0u; i < totalCubes; ++i) {
            const uint16_t l = i / perLayer;
            const uint16_t r = i % perLayer;
            const uint16_t cx = r % cols;
            const uint16_t cy = r / cols;

            const float px = static_cast<float>(cx) * spacing - halfW;
            const float py = static_cast<float>(cy) * spacing - halfH;
            const float pz = -static_cast<float>(l) * spacing + halfD;

            if (m_settings.frustumCull != 0u) {
                if (!frustum.TestSphere(zsdk::Vec3{px, py, pz}, cubeRadius)) {
                    continue;
                }
            }

            Renderer::PushMatrix();
            Renderer::Translate(px, py, pz);
            Renderer::Rotate(m_angleX + static_cast<float>(i & 7u) * 12.0f, 1.0f, 0.0f, 0.0f);
            Renderer::Rotate(m_angleY + static_cast<float>((i >> 3u) & 7u) * 12.0f, 0.0f, 1.0f, 0.0f);
            Renderer::DrawElementsU16(zsdk::graphics::PrimitiveType::Triangles, 36, nullptr);
            Renderer::PopMatrix();
            ++drawn;
        }
        m_lastDrawnCubes = drawn;

        zsdk::graphics::MeshBuffer::Unbind(zsdk::graphics::BufferType::Index);
        Renderer::UnbindVertexAttributes();
        Renderer::SetLighting(false);

        /* Soft blob shadow for single-cube showcase mode */
        if (totalCubes == 1u) {
            Renderer::DrawBlobShadow(zsdk::Vec3{0.0f, -1.6f, 0.0f}, 1.45f,
                                     zsdk::Color::Rgba(0, 0, 0, 120));
        }

        Renderer::SetDepthTest(false);

        RenderHud(w, h, totalCubes, drawn);
        if (m_menuOpen) {
            RenderMenu(w, h);
        }

        m_gl.SwapBuffers();
    }

    void OnShutdown() override {
        m_saveMgr.Shutdown();
        m_batch.Shutdown();
        m_ibo.Destroy();
        m_vbo.Destroy();
        m_gl.Shutdown();
    }

private:
    void ApplyFps() {
        auto* app = zsdk::Application::GetInstance();
        app->SetFpsCap((m_settings.fpsMode & 1u) ? zsdk::FpsCap::Fps30 : zsdk::FpsCap::Fps60);
    }

    void ApplySettings() {
        ApplyFps();
        m_gl.SetSwapInterval(m_settings.vsync ? 1 : 0);
    }

    void AdjustMenuItem(int8_t dir) {
        auto* app = zsdk::Application::GetInstance();
        switch (m_menuCursor) {
            case 0: // Cube Count (1..1024)
                m_settings.cubeCountIndex = static_cast<uint8_t>((m_settings.cubeCountIndex + 11 + dir) % 11);
                m_camPanX = 0.0f;
                m_camPanY = 0.0f;
                m_camZoomOffset = 0.0f;
                break;
            case 1: // Frustum Culling
                m_settings.frustumCull = (m_settings.frustumCull == 0u) ? 1u : 0u;
                break;
            case 2: // Face Culling (Backface)
                m_settings.faceCull = (m_settings.faceCull == 0u) ? 1u : 0u;
                break;
            case 3: // Lighting
                m_settings.lighting = (m_settings.lighting == 0u) ? 1u : 0u;
                break;
            case 4: // FPS Cap
                m_settings.fpsMode = (m_settings.fpsMode == 0u) ? 1u : 0u;
                ApplyFps();
                break;
            case 5: // VSync
                m_settings.vsync = (m_settings.vsync == 0u) ? 1u : 0u;
                m_gl.SetSwapInterval(m_settings.vsync ? 1 : 0);
                break;
            case 6: // Spin Speed
                m_settings.spinMode = static_cast<uint8_t>((m_settings.spinMode + 4 + dir) % 4);
                break;
            case 7: // Debug Overlay
                zsdk::debug::DebugOverlay::SetVisible(!zsdk::debug::DebugOverlay::IsVisible());
                break;
            case 8: // Save to udata
                m_saveMgr.SaveSlot(0, &m_settings, sizeof(m_settings), kSaveVersion);
                ZSDK_LOG_INFO("CUBE", "Saved settings to udata.");
                break;
            case 9: // Exit
                app->RequestQuit();
                break;
            default:
                break;
        }
    }

    void UpdateMenu(const zsdk::input::GamepadState& p1, uint32_t deltaTimeMs) {
        const zsdk::Vec2 stick = p1.GetLeftStick();
        uint8_t dir = 0u;
        if (p1.IsDown(zsdk::input::GamepadButton::DPadUp)    || stick.y >  0.55f) dir = 1u;
        else if (p1.IsDown(zsdk::input::GamepadButton::DPadDown)  || stick.y < -0.55f) dir = 2u;
        else if (p1.IsDown(zsdk::input::GamepadButton::DPadLeft)  || stick.x < -0.55f) dir = 3u;
        else if (p1.IsDown(zsdk::input::GamepadButton::DPadRight) || stick.x >  0.55f) dir = 4u;

        uint8_t fireDir = 0u;
        if (dir == 0u) {
            m_menuRepeatDir = 0u;
            m_menuRepeatTimerMs = 0;
        } else if (dir != m_menuRepeatDir) {
            m_menuRepeatDir = dir;
            m_menuRepeatTimerMs = 260;
            fireDir = dir;
        } else {
            m_menuRepeatTimerMs -= static_cast<int32_t>(deltaTimeMs);
            if (m_menuRepeatTimerMs <= 0) {
                m_menuRepeatTimerMs = 95;
                fireDir = dir;
            }
        }

        if (fireDir == 1u) {
            m_menuCursor = (m_menuCursor == 0u) ? (kMenuItemCount - 1u) : (m_menuCursor - 1u);
        } else if (fireDir == 2u) {
            m_menuCursor = static_cast<uint8_t>((m_menuCursor + 1u) % kMenuItemCount);
        } else if (fireDir == 3u) {
            AdjustMenuItem(-1);
        } else if (fireDir == 4u) {
            AdjustMenuItem(1);
        }

        if (p1.IsPressed(zsdk::input::GamepadButton::Action1)) {
            AdjustMenuItem(1);
        }
        if (p1.IsPressed(zsdk::input::GamepadButton::Back)) {
            m_menuOpen = false;
        }
    }

    void RenderHud(int32_t w, int32_t h, uint16_t totalCubes, uint16_t drawnCubes) {
        using zsdk::graphics::Renderer;
        m_batch.BeginVirtual(640.0f, 480.0f, w, h);
        m_batch.DrawRect(10.0f, 10.0f, 290.0f, 36.0f, zsdk::Color::Rgba(10, 14, 22, 180));
        m_batch.Flush();

        char line1[48];
        int p = AppendStr(line1, 0, sizeof(line1), "CUBES: ");
        p = AppendUInt(line1, p, sizeof(line1), totalCubes);
        p = AppendStr(line1, p, sizeof(line1), "  DRAWN: ");
        AppendUInt(line1, p, sizeof(line1), drawnCubes);

        char line2[48];
        p = AppendStr(line2, 0, sizeof(line2), "FRUSTUM:");
        p = AppendStr(line2, p, sizeof(line2), m_settings.frustumCull ? "ON " : "OFF ");
        p = AppendStr(line2, p, sizeof(line2), "FACE:");
        AppendStr(line2, p, sizeof(line2), m_settings.faceCull ? "ON" : "OFF");

        Renderer::DrawText2D(18.0f, 16.0f, 1.0f, line1, zsdk::Color::Rgba(235, 245, 255, 255));
        Renderer::DrawText2D(18.0f, 30.0f, 1.0f, line2, zsdk::Color::Rgba(160, 215, 255, 255));
        m_batch.End();
    }

    void RenderMenu(int32_t w, int32_t h) {
        using zsdk::graphics::Renderer;
        m_batch.BeginVirtual(640.0f, 480.0f, w, h);

        const float menuW = 360.0f;
        const float menuH = 224.0f;
        const float mx = (640.0f - menuW) * 0.5f;
        const float my = (480.0f - menuH) * 0.5f;

        m_batch.DrawRect(mx, my, menuW, menuH, zsdk::Color::Rgba(12, 16, 26, 230));
        m_batch.DrawRect(mx, my, menuW, 24.0f, zsdk::Color::Rgba(35, 75, 130, 240));
        for (uint8_t i = 0u; i < kMenuItemCount; ++i) {
            if (i == m_menuCursor) {
                const float ry = my + 34.0f + static_cast<float>(i) * 18.0f;
                m_batch.DrawRect(mx + 8.0f, ry - 3.0f, menuW - 16.0f, 15.0f,
                                 zsdk::Color::Rgba(55, 110, 180, 200));
            }
        }
        m_batch.Flush();

        Renderer::DrawText2D(mx + 12.0f, my + 8.0f, 1.0f, "ZSDK SETTINGS (START)",
                             zsdk::Color::Rgba(255, 255, 255, 255));

        const char* fpsNames[2]  = {"60 FPS", "30 FPS"};
        const char* spinNames[4] = {"Paused", "Slow", "Normal", "Fast"};

        char cubeBuf[40];
        int pos = AppendStr(cubeBuf, 0, sizeof(cubeBuf), "Cube Count  : ");
        AppendUInt(cubeBuf, pos, sizeof(cubeBuf), kCubeCounts[m_settings.cubeCountIndex]);

        char fpsBuf[40];
        pos = AppendStr(fpsBuf, 0, sizeof(fpsBuf), "FPS Cap     : ");
        AppendStr(fpsBuf, pos, sizeof(fpsBuf), fpsNames[m_settings.fpsMode & 1u]);

        char spinBuf[40];
        pos = AppendStr(spinBuf, 0, sizeof(spinBuf), "Spin Speed  : ");
        AppendStr(spinBuf, pos, sizeof(spinBuf), spinNames[m_settings.spinMode & 3u]);

        const char* labels[kMenuItemCount] = {
            cubeBuf,
            m_settings.frustumCull ? "Frustum Cull: ON"  : "Frustum Cull: OFF",
            m_settings.faceCull    ? "Face Cull   : ON"  : "Face Cull   : OFF",
            m_settings.lighting    ? "Lighting    : ON"  : "Lighting    : OFF",
            fpsBuf,
            m_settings.vsync       ? "VSync       : ON"  : "VSync       : OFF",
            spinBuf,
            zsdk::debug::DebugOverlay::IsVisible() ? "Debug Panel : SHOW" : "Debug Panel : HIDE",
            "Save Config (udata)",
            "Exit App"
        };

        for (uint8_t i = 0u; i < kMenuItemCount; ++i) {
            const float ry = my + 34.0f + static_cast<float>(i) * 18.0f;
            const zsdk::Color col = (i == m_menuCursor)
                                        ? zsdk::Color::Rgba(255, 240, 120, 255)
                                        : zsdk::Color::Rgba(210, 220, 230, 255);
            Renderer::DrawText2D(mx + 16.0f, ry, 1.0f, labels[i], col);
        }

        m_batch.End();
    }

    zsdk::graphics::GLContext     m_gl;
    zsdk::graphics::MeshBuffer    m_vbo;
    zsdk::graphics::MeshBuffer    m_ibo;
    zsdk::graphics::SpriteBatch2D m_batch;
    zsdk::fs::SaveGameManager     m_saveMgr;

    float        m_angleX;
    float        m_angleY;
    float        m_camPanX;
    float        m_camPanY;
    float        m_camZoomOffset;
    uint16_t     m_lastDrawnCubes;
    bool         m_menuOpen;
    uint8_t      m_menuCursor;
    uint8_t      m_menuRepeatDir;
    int32_t      m_menuRepeatTimerMs;
    CubeSettings m_settings;
};

#if ZSDK_PLATFORM_ZEEBO
ZSDK_IMPLEMENT_APP(SpinningCubeApp, "ZSDK_SpinningCube", 30, true)
#else
ZSDK_IMPLEMENT_APP(SpinningCubeApp, "ZSDK_SpinningCube", 60, true)
#endif
