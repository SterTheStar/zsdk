/**
 * @file windows_backend.cpp
 * @brief Windows/PC backend implementation sharing the exact same ZSDK public headers.
 */

#include "zsdk/zsdk.hpp"

#if ZSDK_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <stdio.h>

/* WinUser.h defines DrawText -> DrawTextA/W which collides with Display2D::DrawText. */
#ifdef DrawText
    #undef DrawText
#endif

namespace zsdk {

namespace {

typedef BOOL (WINAPI* PfnWglSwapIntervalEXT)(int interval);

struct KeyZPadMapping {
    int                  vk;
    input::GamepadButton button;
};

static const KeyZPadMapping kKeyToZPadTable[] = {
    { VK_UP,     input::GamepadButton::DPadUp    },
    { VK_DOWN,   input::GamepadButton::DPadDown  },
    { VK_LEFT,   input::GamepadButton::DPadLeft  },
    { VK_RIGHT,  input::GamepadButton::DPadRight },
    { 'Z',       input::GamepadButton::Action1   },
    { 'X',       input::GamepadButton::Action2   },
    { 'C',       input::GamepadButton::Action3   },
    { 'V',       input::GamepadButton::Action4   },
    { VK_RETURN, input::GamepadButton::Start     },
    { VK_ESCAPE, input::GamepadButton::Home      },
    { VK_BACK,   input::GamepadButton::Back      }
};

HWND g_mainHwnd = nullptr;

} // namespace

/* ============================================================================
 * Clock
 * ========================================================================== */

uint32_t Clock::GetUptimeMs() {
    static LARGE_INTEGER s_freq = {};
    static LARGE_INTEGER s_start = {};
    if (s_freq.QuadPart == 0) {
        ::QueryPerformanceFrequency(&s_freq);
        ::QueryPerformanceCounter(&s_start);
    }
    LARGE_INTEGER now = {};
    ::QueryPerformanceCounter(&now);
    const uint64_t elapsedTicks = static_cast<uint64_t>(now.QuadPart - s_start.QuadPart);
    return static_cast<uint32_t>((elapsedTicks * 1000ULL) / static_cast<uint64_t>(s_freq.QuadPart));
}

uint32_t Clock::GetWallTimeMs() {
    return static_cast<uint32_t>(::GetTickCount());
}

/* ============================================================================
 * Application::PlatformState & Win32 Lifecycle Bridge
 * ========================================================================== */

struct Application::PlatformState {
    HWND    hwnd;
    HDC     hdc;
    int32_t width;
    int32_t height;
    bool    active;
};

void* Application::GetNativePlatformContext() const {
    return m_platform ? static_cast<void*>(m_platform->hwnd) : nullptr;
}

int32_t Application::GetScreenWidth() const {
    return m_platform ? m_platform->width : 640;
}

int32_t Application::GetScreenHeight() const {
    return m_platform ? m_platform->height : 480;
}

void Application::SetTargetFps(uint16_t fps) {
    m_config.targetFps = fps;
}

class ZeeboAppBridge {
public:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Application* app = reinterpret_cast<Application*>(
            ::GetWindowLongPtrA(hwnd, GWLP_USERDATA));

        switch (msg) {
            case WM_CREATE: {
                const CREATESTRUCTA* cs = reinterpret_cast<const CREATESTRUCTA*>(lParam);
                if (cs && cs->lpCreateParams) {
                    ::SetWindowLongPtrA(
                        hwnd, GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                }
                return 0;
            }
            case WM_SIZE: {
                if (app && app->m_platform) {
                    const int32_t w = static_cast<int32_t>(LOWORD(lParam));
                    const int32_t h = static_cast<int32_t>(HIWORD(lParam));
                    if (w > 0 && h > 0) {
                        app->m_platform->width = w;
                        app->m_platform->height = h;
                    }
                }
                return 0;
            }
            case WM_ACTIVATE: {
                if (app && app->m_platform) {
                    const bool nowActive = (LOWORD(wParam) != WA_INACTIVE);
                    if (nowActive && app->m_suspended) {
                        app->m_suspended = false;
                        if (app->m_delegate) {
                            app->m_delegate->OnResume();
                        }
                    } else if (!nowActive && !app->m_suspended) {
                        app->m_suspended = true;
                        if (app->m_delegate) {
                            app->m_delegate->OnSuspend();
                        }
                    }
                    app->m_platform->active = nowActive;
                }
                return 0;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: {
                if (app) {
                    app->m_input.HandlePlatformEvent(
                        static_cast<uint32_t>(msg),
                        static_cast<uint16_t>(wParam),
                        static_cast<uint32_t>(lParam));
                }
                return 0;
            }
            case WM_CLOSE: {
                if (app) {
                    app->RequestQuit();
                }
                ::DestroyWindow(hwnd);
                return 0;
            }
            case WM_DESTROY: {
                if (app) {
                    app->RequestQuit();
                }
                ::PostQuitMessage(0);
                return 0;
            }
            default:
                break;
        }
        return ::DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    static int Run() {
        AppConfig config = {"ZSDK App", 60, ZSDK_DEFAULT_MEMORY_LIMIT_BYTES, false};
        IAppDelegate* delegate = CreateUserApp(config);
        if (!delegate) {
            return 1;
        }

        const HINSTANCE hInst = ::GetModuleHandleA(nullptr);
        static const char kWndClassName[] = "ZSDK_WindowClass";

        WNDCLASSA wc = {};
        wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInst;
        wc.hCursor       = ::LoadCursorA(nullptr, IDC_ARROW);
        wc.lpszClassName = kWndClassName;
        if (!::RegisterClassA(&wc)) {
            return 1;
        }

        const int32_t clientW = 640;
        const int32_t clientH = 480;
        RECT rect = {0, 0, clientW, clientH};
        const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        ::AdjustWindowRect(&rect, style, FALSE);

        Application app;
        Application::PlatformState ps = {};
        ps.width  = clientW;
        ps.height = clientH;
        ps.active = true;

        const char* title = config.name ? config.name : "ZSDK App";
        HWND hwnd = ::CreateWindowExA(
            0, kWndClassName, title, style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, hInst, &app);

        if (!hwnd) {
            ::UnregisterClassA(kWndClassName, hInst);
            return 1;
        }

        ps.hwnd = hwnd;
        ps.hdc  = ::GetDC(hwnd);
        g_mainHwnd = hwnd;

        app.m_platform = &ps;
        app.Configure(config, delegate);

        debug::DebugOverlay::Initialize();
        debug::DebugOverlay::SetVisible(config.enableDebugOverlay);

        if (Failed(app.m_input.Initialize(hwnd))) {
            ::ReleaseDC(hwnd, ps.hdc);
            ::DestroyWindow(hwnd);
            ::UnregisterClassA(kWndClassName, hInst);
            g_mainHwnd = nullptr;
            app.m_platform = nullptr;
            return 1;
        }

        if (Failed(delegate->OnInit())) {
            app.m_input.Shutdown();
            ::ReleaseDC(hwnd, ps.hdc);
            ::DestroyWindow(hwnd);
            ::UnregisterClassA(kWndClassName, hInst);
            g_mainHwnd = nullptr;
            app.m_platform = nullptr;
            return 1;
        }

        LARGE_INTEGER qpcFreq = {};
        ::QueryPerformanceFrequency(&qpcFreq);
        ::timeBeginPeriod(1);
        app.m_timer.Reset();

        LARGE_INTEGER nextFrameTick = {};
        ::QueryPerformanceCounter(&nextFrameTick);

        MSG msg = {};
        while (!app.IsQuitRequested()) {
            while (::PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    app.RequestQuit();
                    break;
                }
                ::TranslateMessage(&msg);
                ::DispatchMessageA(&msg);
            }

            if (app.IsQuitRequested()) {
                break;
            }

            if (!app.m_suspended) {
                const uint32_t dtMs = app.m_timer.Tick();
                app.m_input.Update(dtMs);
                app.DispatchGamepadChanges();
                debug::DebugOverlay::Update(app.m_input, static_cast<float>(dtMs));

                delegate->OnUpdate(dtMs);
                if (!app.IsQuitRequested()) {
                    delegate->OnRender();
                }
            }

            const uint16_t curTargetFps = app.GetTargetFps();
            if (curTargetFps > 0u && qpcFreq.QuadPart > 0) {
                const LONGLONG targetTicks = qpcFreq.QuadPart / static_cast<LONGLONG>(curTargetFps);
                nextFrameTick.QuadPart += targetTicks;

                LARGE_INTEGER now = {};
                ::QueryPerformanceCounter(&now);
                /* If we fell behind by more than 2 frames, resync to avoid catch-up burst */
                if (now.QuadPart > nextFrameTick.QuadPart + targetTicks * 2) {
                    nextFrameTick.QuadPart = now.QuadPart;
                }

                for (;;) {
                    ::QueryPerformanceCounter(&now);
                    const LONGLONG remainTicks = nextFrameTick.QuadPart - now.QuadPart;
                    if (remainTicks <= 0) {
                        break;
                    }
                    const double remainMs = (static_cast<double>(remainTicks) * 1000.0) /
                                            static_cast<double>(qpcFreq.QuadPart);
                    if (remainMs > 2.0) {
                        ::Sleep(static_cast<DWORD>(remainMs - 1.5));
                    }
                }
            }
        }

        ::timeEndPeriod(1);

        delegate->OnShutdown();
        debug::DebugOverlay::Shutdown();
        app.m_input.Shutdown();

        if (ps.hdc) {
            ::ReleaseDC(hwnd, ps.hdc);
            ps.hdc = nullptr;
        }
        if (::IsWindow(hwnd)) {
            ::DestroyWindow(hwnd);
        }
        ::UnregisterClassA(kWndClassName, hInst);
        g_mainHwnd = nullptr;
        app.m_platform = nullptr;
        return 0;
    }
};

/* ============================================================================
 * SystemInfo & Thread
 * ========================================================================== */

Result SystemInfo::GetMemoryInfo(SystemMemoryInfo& outInfo) {
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!::GlobalMemoryStatusEx(&ms)) {
        return Result::Error;
    }
    outInfo.freeRamBytes          = static_cast<uint32_t>(ms.ullAvailPhys);
    outInfo.totalRamBytes         = static_cast<uint32_t>(ms.ullTotalPhys);
    outInfo.largestFreeBlockBytes = outInfo.freeRamBytes;
    outInfo.freeFsBytes           = 0u;
    outInfo.totalFsBytes          = 0u;
    return Result::Ok;
}

Result SystemInfo::GetBatteryStatus(void*, BatteryStatus& outStatus) {
    outStatus = BatteryStatus{false, false, 100u};
    return Result::Ok;
}

Result SystemInfo::SetBacklight(void*, bool, uint8_t) { return Result::Ok; }
uint32_t SystemInfo::GetLanguageCode(void*) { return 0u; }

struct Thread::Impl { bool running; };
Thread::Thread() : m_impl(nullptr) {}
Thread::~Thread() { Join(); }
Result Thread::Start(void*, ThreadEntryFn entry, void* userData, uint32_t) {
    if (!entry) return Result::InvalidArg;
    entry(userData);
    return Result::Ok;
}
void Thread::Join() {}
void Thread::SleepMs(uint32_t ms) { ::Sleep(ms); }
bool Thread::IsRunning() const { return false; }

/* ============================================================================
 * InputManager & KeyboardMouseManager
 * ========================================================================== */

namespace input {

class ZeeboInputBridge {
public:
    static void SetButton(GamepadState& pad, GamepadButton btn, bool down) {
        pad.SetButtonState(btn, down);
    }
    static void SetConnected(GamepadState& pad, bool connected) {
        pad.m_connected = connected;
    }
    static void SetSticks(GamepadState& pad, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
        pad.SetRawSticks(lx, ly, rx, ry);
    }
};

struct InputManager::Impl {
    HWND hwnd;
};

namespace {
bool MapVirtualKeyToZPad(uint16_t vk, GamepadButton& outButton) {
    for (size_t i = 0; i < (sizeof(kKeyToZPadTable) / sizeof(kKeyToZPadTable[0])); ++i) {
        if (static_cast<uint16_t>(kKeyToZPadTable[i].vk) == vk) {
            outButton = kKeyToZPadTable[i].button;
            return true;
        }
    }
    return false;
}
/* Lightweight dynamic XInput definitions (no xinput.lib link required) */
struct WinXInputGamepad {
    uint16_t wButtons;
    uint8_t  bLeftTrigger;
    uint8_t  bRightTrigger;
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
};

struct WinXInputState {
    uint32_t         dwPacketNumber;
    WinXInputGamepad Gamepad;
};

struct WinXInputVibration {
    uint16_t wLeftMotorSpeed;
    uint16_t wRightMotorSpeed;
};

typedef uint32_t (WINAPI *PfnXInputGetState)(uint32_t dwUserIndex, WinXInputState* pState);
typedef uint32_t (WINAPI *PfnXInputSetState)(uint32_t dwUserIndex, WinXInputVibration* pVibration);

static PfnXInputGetState g_xinputGetState = nullptr;
static PfnXInputSetState g_xinputSetState = nullptr;
static bool              g_xinputLoaded   = false;

void EnsureXInputLoaded() {
    if (g_xinputLoaded) {
        return;
    }
    g_xinputLoaded = true;
    const char* dlls[3] = {"xinput1_4.dll", "xinput9_1_0.dll", "xinput1_3.dll"};
    for (int i = 0; i < 3; ++i) {
        HMODULE mod = ::LoadLibraryA(dlls[i]);
        if (mod) {
            g_xinputGetState = reinterpret_cast<PfnXInputGetState>(
                ::GetProcAddress(mod, "XInputGetState"));
            g_xinputSetState = reinterpret_cast<PfnXInputSetState>(
                ::GetProcAddress(mod, "XInputSetState"));
            if (g_xinputGetState) {
                return;
            }
        }
    }
}
} // namespace

InputManager::InputManager() : m_impl(nullptr) {}
InputManager::~InputManager() { Shutdown(); }

Result InputManager::Initialize(void* platformContext) {
    static Impl s_inputImpl = {};
    s_inputImpl.hwnd = static_cast<HWND>(platformContext);
    if (!s_inputImpl.hwnd) {
        s_inputImpl.hwnd = g_mainHwnd;
    }
    m_impl = &s_inputImpl;
    EnsureXInputLoaded();
    m_gamepads[0].m_connected = true;
    return Result::Ok;
}

void InputManager::Shutdown() {
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        m_gamepads[i].m_connected = false;
    }
    m_impl = nullptr;
}

void InputManager::Update(uint32_t deltaTimeMs) {
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        m_gamepads[i].BeginFrame(deltaTimeMs);
    }

    bool p1Buttons[static_cast<size_t>(GamepadButton::Count)] = {};

    const HWND targetHwnd = m_impl ? m_impl->hwnd : g_mainHwnd;
    if (targetHwnd && ::GetForegroundWindow() == targetHwnd) {
        for (size_t i = 0; i < (sizeof(kKeyToZPadTable) / sizeof(kKeyToZPadTable[0])); ++i) {
            if ((::GetAsyncKeyState(kKeyToZPadTable[i].vk) & 0x8000) != 0) {
                p1Buttons[static_cast<size_t>(kKeyToZPadTable[i].button)] = true;
            }
        }
    }

    if (g_xinputGetState) {
        for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
            WinXInputState xs = {};
            const bool connected = (g_xinputGetState(i, &xs) == ERROR_SUCCESS);
            ZeeboInputBridge::SetConnected(m_gamepads[i], (i == 0u) ? true : connected);

            if (!connected) {
                if (i > 0u) {
                    ZeeboInputBridge::SetSticks(m_gamepads[i], 0, 0, 0, 0);
                }
                continue;
            }

            const uint16_t b = xs.Gamepad.wButtons;
            const bool map[static_cast<size_t>(GamepadButton::Count)] = {
                (b & 0x0001u) != 0u, // DPadUp
                (b & 0x0002u) != 0u, // DPadDown
                (b & 0x0004u) != 0u, // DPadLeft
                (b & 0x0008u) != 0u, // DPadRight
                (b & 0x1000u) != 0u, // Action1 (A)
                (b & 0x4000u) != 0u, // Action2 (X)
                (b & 0x8000u) != 0u, // Action3 (Y)
                (b & 0x2000u) != 0u, // Action4 (B)
                (b & 0x0100u) != 0u, // ShoulderLeft (LB)
                (b & 0x0200u) != 0u, // ShoulderRight (RB)
                xs.Gamepad.bLeftTrigger > 30u,  // TriggerLeft (LT)
                xs.Gamepad.bRightTrigger > 30u, // TriggerRight (RT)
                (b & 0x0010u) != 0u, // Start
                (b & 0x0020u) != 0u, // Back
                false,               // Home
                (b & 0x0040u) != 0u, // LeftThumb
                (b & 0x0080u) != 0u  // RightThumb
            };

            if (i == 0u) {
                for (size_t k = 0; k < static_cast<size_t>(GamepadButton::Count); ++k) {
                    p1Buttons[k] = p1Buttons[k] || map[k];
                }
            } else {
                for (size_t k = 0; k < static_cast<size_t>(GamepadButton::Count); ++k) {
                    ZeeboInputBridge::SetButton(m_gamepads[i], static_cast<GamepadButton>(k), map[k]);
                }
            }

            auto negateAxis = [](SHORT v) -> int16_t {
                return (v <= -32767) ? static_cast<int16_t>(32767) : static_cast<int16_t>(-v);
            };
            ZeeboInputBridge::SetSticks(m_gamepads[i],
                                        xs.Gamepad.sThumbLX, negateAxis(xs.Gamepad.sThumbLY),
                                        xs.Gamepad.sThumbRX, negateAxis(xs.Gamepad.sThumbRY));
        }
    }

    for (size_t k = 0; k < static_cast<size_t>(GamepadButton::Count); ++k) {
        ZeeboInputBridge::SetButton(m_gamepads[0], static_cast<GamepadButton>(k), p1Buttons[k]);
    }
}

bool InputManager::HandlePlatformEvent(uint32_t eventCode, uint16_t keyCode, uint32_t) {
    const bool isDown = (eventCode == WM_KEYDOWN || eventCode == WM_SYSKEYDOWN);
    const bool isUp   = (eventCode == WM_KEYUP || eventCode == WM_SYSKEYUP);
    if (!isDown && !isUp) {
        return false;
    }

    GamepadButton btn = GamepadButton::DPadUp;
    if (!MapVirtualKeyToZPad(keyCode, btn)) {
        return false;
    }

    ZeeboInputBridge::SetButton(m_gamepads[0], btn, isDown);
    return true;
}

Result InputManager::SetRumble(uint8_t playerIndex, uint16_t leftMotor, uint16_t rightMotor) {
    if (playerIndex >= ZSDK_MAX_GAMEPADS) {
        return Result::InvalidArg;
    }
    if (!g_xinputSetState) {
        return Result::NotSupported;
    }
    WinXInputVibration vib = {leftMotor, rightMotor};
    return (g_xinputSetState(playerIndex, &vib) == ERROR_SUCCESS) ? Result::Ok : Result::DeviceDisconnected;
}

struct KeyboardMouseManager::Impl { int dummy; };
KeyboardMouseManager::KeyboardMouseManager() : m_impl(nullptr) {}
KeyboardMouseManager::~KeyboardMouseManager() {}
Result KeyboardMouseManager::Initialize(void*) { return Result::Ok; }
void KeyboardMouseManager::Shutdown() {}
void KeyboardMouseManager::Update() {}
bool KeyboardMouseManager::IsKeyDown(uint16_t vk) const { return (::GetAsyncKeyState(vk) & 0x8000) != 0; }
Vec2 KeyboardMouseManager::GetMouseDelta() const { return Vec2{0.0f, 0.0f}; }
uint8_t KeyboardMouseManager::GetMouseButtonsMask() const { return 0u; }
bool KeyboardMouseManager::HasKeyboard() const { return true; }
bool KeyboardMouseManager::HasMouse() const { return true; }

} // namespace input

/* ============================================================================
 * WGL OpenGL Context & Display2D
 * ========================================================================== */

namespace graphics {

struct GLContext::Impl {
    HWND    hwnd;
    HDC     hdc;
    HGLRC   hglrc;
    int32_t width;
    int32_t height;
    int32_t renderWidth;
    int32_t renderHeight;
    GLuint  stretchTex;
    int32_t stretchTexW;
    int32_t stretchTexH;
    bool    valid;
};

GLContext::GLContext() : m_impl(nullptr) {}
GLContext::~GLContext() { Shutdown(); }

Result GLContext::Initialize(void* platformContext, const GLContextConfig& config) {
    static Impl s_glImpl = {};
    Shutdown();

    HWND hwnd = static_cast<HWND>(platformContext);
    if (!hwnd) {
        hwnd = g_mainHwnd ? g_mainHwnd : ::GetActiveWindow();
    }
    if (!hwnd) {
        return Result::NotInitialized;
    }

    HDC hdc = ::GetDC(hwnd);
    if (!hdc) {
        return Result::GraphicsError;
    }

    const uint8_t colorBits = static_cast<uint8_t>(
        (config.redBits + config.greenBits + config.blueBits) > 0
            ? (config.redBits + config.greenBits + config.blueBits)
            : 24);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = colorBits;
    pfd.cAlphaBits   = config.alphaBits;
    pfd.cDepthBits   = (config.depthBits > 0u) ? config.depthBits : 16u;
    pfd.cStencilBits = config.stencilBits;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    const int pixelFormat = ::ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0 || !::SetPixelFormat(hdc, pixelFormat, &pfd)) {
        ::ReleaseDC(hwnd, hdc);
        return Result::GraphicsError;
    }

    HGLRC hglrc = ::wglCreateContext(hdc);
    if (!hglrc) {
        ::ReleaseDC(hwnd, hdc);
        return Result::GraphicsError;
    }

    if (!::wglMakeCurrent(hdc, hglrc)) {
        ::wglDeleteContext(hglrc);
        ::ReleaseDC(hwnd, hdc);
        return Result::GraphicsError;
    }

    RECT rc = {};
    ::GetClientRect(hwnd, &rc);

    s_glImpl.hwnd         = hwnd;
    s_glImpl.hdc          = hdc;
    s_glImpl.hglrc        = hglrc;
    s_glImpl.width        = (rc.right > rc.left) ? static_cast<int32_t>(rc.right - rc.left) : 640;
    s_glImpl.height       = (rc.bottom > rc.top) ? static_cast<int32_t>(rc.bottom - rc.top) : 480;
    s_glImpl.renderWidth  = s_glImpl.width;
    s_glImpl.renderHeight = s_glImpl.height;
    s_glImpl.stretchTex   = 0u;
    s_glImpl.stretchTexW  = 0;
    s_glImpl.stretchTexH  = 0;
    s_glImpl.valid        = true;

    m_impl = &s_glImpl;
    SetSwapInterval(static_cast<int32_t>(config.swapInterval));
    return Result::Ok;
}

void GLContext::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->hglrc) {
        graphics::Renderer::ReleaseSharedFontTexture();
        if (m_impl->stretchTex != 0u) {
            ::glDeleteTextures(1, &m_impl->stretchTex);
            m_impl->stretchTex = 0u;
        }
        ::wglMakeCurrent(nullptr, nullptr);
        ::wglDeleteContext(m_impl->hglrc);
        m_impl->hglrc = nullptr;
    }
    if (m_impl->hdc && m_impl->hwnd) {
        ::ReleaseDC(m_impl->hwnd, m_impl->hdc);
        m_impl->hdc = nullptr;
    }
    m_impl->hwnd  = nullptr;
    m_impl->valid = false;
    m_impl = nullptr;
}

Result GLContext::MakeCurrent() {
    if (!m_impl || !m_impl->valid) {
        return Result::NotInitialized;
    }
    return ::wglMakeCurrent(m_impl->hdc, m_impl->hglrc) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::SwapBuffers() {
    if (!m_impl || !m_impl->valid) {
        return Result::NotInitialized;
    }
    RECT rc = {};
    if (::GetClientRect(m_impl->hwnd, &rc)) {
        const int32_t newW = (rc.right > rc.left) ? static_cast<int32_t>(rc.right - rc.left) : m_impl->width;
        const int32_t newH = (rc.bottom > rc.top) ? static_cast<int32_t>(rc.bottom - rc.top) : m_impl->height;
        if (m_impl->renderWidth == m_impl->width && m_impl->renderHeight == m_impl->height) {
            m_impl->renderWidth  = newW;
            m_impl->renderHeight = newH;
        }
        m_impl->width  = newW;
        m_impl->height = newH;
    }
    if (m_impl->hdc) {
        debug::DebugOverlay::Render(m_impl->renderWidth, m_impl->renderHeight);

        if (m_impl->renderWidth > 0 && m_impl->renderHeight > 0 &&
            (m_impl->renderWidth != m_impl->width || m_impl->renderHeight != m_impl->height)) {
            int32_t texW = 1;
            while (texW < m_impl->renderWidth) texW <<= 1;
            int32_t texH = 1;
            while (texH < m_impl->renderHeight) texH <<= 1;

            if (m_impl->stretchTex == 0u) {
                ::glGenTextures(1, &m_impl->stretchTex);
            }
            ::glBindTexture(GL_TEXTURE_2D, m_impl->stretchTex);
            if (m_impl->stretchTexW != texW || m_impl->stretchTexH != texH) {
                ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                m_impl->stretchTexW = texW;
                m_impl->stretchTexH = texH;
            }
            ::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                                  m_impl->renderWidth, m_impl->renderHeight);

            ::glViewport(0, 0, m_impl->width, m_impl->height);
            ::glMatrixMode(GL_PROJECTION);
            ::glPushMatrix();
            ::glLoadIdentity();
            ::glOrtho(0.0, static_cast<double>(m_impl->width),
                      0.0, static_cast<double>(m_impl->height), -1.0, 1.0);
            ::glMatrixMode(GL_MODELVIEW);
            ::glPushMatrix();
            ::glLoadIdentity();

            ::glDisable(GL_DEPTH_TEST);
            ::glDisable(GL_LIGHTING);
            ::glDisable(GL_BLEND);
            ::glEnable(GL_TEXTURE_2D);
            ::glColor4ub(255, 255, 255, 255);

            const float uMax = static_cast<float>(m_impl->renderWidth) / static_cast<float>(texW);
            const float vMax = static_cast<float>(m_impl->renderHeight) / static_cast<float>(texH);
            const float fw = static_cast<float>(m_impl->width);
            const float fh = static_cast<float>(m_impl->height);
            const GLfloat quadVerts[8] = {0.0f, 0.0f, fw, 0.0f, 0.0f, fh, fw, fh};
            const GLfloat quadUvs[8]   = {0.0f, 0.0f, uMax, 0.0f, 0.0f, vMax, uMax, vMax};

            ::glEnableClientState(GL_VERTEX_ARRAY);
            ::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            ::glVertexPointer(2, GL_FLOAT, 0, quadVerts);
            ::glTexCoordPointer(2, GL_FLOAT, 0, quadUvs);
            ::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            ::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            ::glDisableClientState(GL_VERTEX_ARRAY);
            ::glDisable(GL_TEXTURE_2D);

            ::glMatrixMode(GL_PROJECTION);
            ::glPopMatrix();
            ::glMatrixMode(GL_MODELVIEW);
            ::glPopMatrix();
        }
    }
    return ::SwapBuffers(m_impl->hdc) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::SetSwapInterval(int32_t interval) {
    if (!m_impl || !m_impl->valid) {
        return Result::NotInitialized;
    }
    PfnWglSwapIntervalEXT swapInterval = reinterpret_cast<PfnWglSwapIntervalEXT>(
        ::wglGetProcAddress("wglSwapIntervalEXT"));
    if (!swapInterval) {
        return Result::NotSupported;
    }
    return swapInterval(interval) ? Result::Ok : Result::GraphicsError;
}

Result GLContext::SetSurfaceScale(const Rect&, const Rect&, bool) { return Result::NotSupported; }
Result GLContext::EnableSurfaceScale(bool) { return Result::NotSupported; }
Result GLContext::SetSurfaceRotate(bool, bool, bool, bool) { return Result::NotSupported; }
Result GLContext::GetPowerLevel(int32_t& outPowerLevel) const { outPowerLevel = 0; return Result::Ok; }
Result GLContext::SetRenderResolution(int32_t renderWidth, int32_t renderHeight) {
    if (!m_impl || !m_impl->valid) {
        return Result::NotInitialized;
    }
    if (renderWidth <= 0 || renderHeight <= 0) {
        return Result::InvalidArg;
    }
    m_impl->renderWidth  = renderWidth;
    m_impl->renderHeight = renderHeight;
    return Result::Ok;
}

int32_t GLContext::GetRenderWidth() const { return m_impl ? m_impl->renderWidth : 0; }
int32_t GLContext::GetRenderHeight() const { return m_impl ? m_impl->renderHeight : 0; }
int32_t GLContext::GetWidth() const { return m_impl ? m_impl->width : 640; }
int32_t GLContext::GetHeight() const { return m_impl ? m_impl->height : 480; }
bool GLContext::IsValid() const { return m_impl && m_impl->valid; }

struct Display2D::Impl { int dummy; };
Display2D::Display2D() : m_impl(nullptr) {}
Display2D::~Display2D() {}
Result Display2D::Initialize(void*) { return Result::Ok; }
void Display2D::Shutdown() {}
void Display2D::Clear(const Color&) {}
void Display2D::FillRect(const Rect&, const Color&) {}
void Display2D::DrawText(int32_t, int32_t, const char*, const Color&) {}
void Display2D::Present() {}
int32_t Display2D::GetWidth() const { return 640; }
int32_t Display2D::GetHeight() const { return 480; }

} // namespace graphics

/* ============================================================================
 * Filesystem, Audio, Network, Security
 * ========================================================================== */

namespace fs {

File::File() : m_nativeFile(nullptr) {}
File::~File() { Close(); }
bool File::IsOpen() const { return m_nativeFile != nullptr; }
int32_t File::Read(void* b, uint32_t n) { return m_nativeFile ? static_cast<int32_t>(::fread(b, 1, n, static_cast<FILE*>(m_nativeFile))) : -1; }
int32_t File::Write(const void* b, uint32_t n) { return m_nativeFile ? static_cast<int32_t>(::fwrite(b, 1, n, static_cast<FILE*>(m_nativeFile))) : -1; }
Result File::Seek(SeekOrigin o, int32_t off) {
    if (!m_nativeFile) return Result::NotInitialized;
    int orig = (o == SeekOrigin::Current) ? SEEK_CUR : ((o == SeekOrigin::End) ? SEEK_END : SEEK_SET);
    return (::fseek(static_cast<FILE*>(m_nativeFile), off, orig) == 0) ? Result::Ok : Result::IoError;
}
uint32_t File::GetSize() const {
    if (!m_nativeFile) return 0u;
    FILE* f = static_cast<FILE*>(m_nativeFile);
    long cur = ::ftell(f); ::fseek(f, 0, SEEK_END);
    long sz = ::ftell(f);  ::fseek(f, cur, SEEK_SET);
    return (sz > 0) ? static_cast<uint32_t>(sz) : 0u;
}
void File::Flush() { if (m_nativeFile) { ::fflush(static_cast<FILE*>(m_nativeFile)); } }
void File::Close() { if (m_nativeFile) { ::fclose(static_cast<FILE*>(m_nativeFile)); m_nativeFile = nullptr; } }

struct FileSystem::Impl { int dummy; };
FileSystem::FileSystem() : m_impl(nullptr) {}
FileSystem::~FileSystem() {}
Result FileSystem::Initialize(void*) { return Result::Ok; }
void FileSystem::Shutdown() {}
Result FileSystem::Open(const char* path, FileMode mode, File& outFile) {
    outFile.Close();
    if (!path) return Result::InvalidArg;
    const char* m = (mode == FileMode::Create) ? "wb+" : ((mode == FileMode::Append) ? "ab+" : ((mode == FileMode::ReadWrite) ? "rb+" : "rb"));
    FILE* f = nullptr;
    if (::fopen_s(&f, path, m) != 0 || !f) return Result::NotFound;
    outFile.m_nativeFile = f;
    return Result::Ok;
}
bool FileSystem::Exists(const char* path) const {
    if (!path) return false;
    FILE* f = nullptr;
    if (::fopen_s(&f, path, "rb") == 0 && f) {
        ::fclose(f);
        return true;
    }
    return false;
}
Result FileSystem::Remove(const char* path) { return (path && ::remove(path) == 0) ? Result::Ok : Result::IoError; }
Result FileSystem::Rename(const char* src, const char* dst) {
    return (src && dst && ::MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING)) ? Result::Ok : Result::IoError;
}
uint32_t FileSystem::GetFreeBytes(uint32_t* outTotal) const {
    ULARGE_INTEGER freeBytes, total, totalFree;
    if (!::GetDiskFreeSpaceExA(nullptr, &freeBytes, &total, &totalFree)) {
        if (outTotal) *outTotal = 0u;
        return 0u;
    }
    const ULONGLONG cap = 0xFFFFFFFFull;
    if (outTotal) *outTotal = static_cast<uint32_t>(total.QuadPart > cap ? cap : total.QuadPart);
    return static_cast<uint32_t>(freeBytes.QuadPart > cap ? cap : freeBytes.QuadPart);
}
Result FileSystem::MkDir(const char* path) { return (path && ::CreateDirectoryA(path, nullptr)) ? Result::Ok : Result::IoError; }
Result FileSystem::ReadAllBytes(const char* path, void** outBuf, uint32_t& outSize) {
    outSize = 0u; if (!outBuf) return Result::InvalidArg; *outBuf = nullptr;
    File f; Result r = Open(path, FileMode::Read, f); if (Failed(r)) return r;
    uint32_t sz = f.GetSize(); void* b = debug::MemoryTracker::Alloc(debug::MemorySubsystem::Filesystem, sz);
    if (!b) return Result::OutOfMemory;
    if (f.Read(b, sz) < 0) { debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, b); return Result::IoError; }
    *outBuf = b; outSize = sz; return Result::Ok;
}
void FileSystem::FreeBuffer(void* buf) { debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, buf); }
Result FileSystem::EnumerateDirectory(const char*, bool, EnumerateDirCallbackFn, void*) { return Result::NotSupported; }

struct UnzipStream::Impl { int dummy; };
UnzipStream::UnzipStream() : m_impl(nullptr) {}
UnzipStream::~UnzipStream() {}
Result UnzipStream::Initialize(void*) { return Result::Ok; }
void UnzipStream::Shutdown() {}
Result UnzipStream::SetInputStream(void*) { return Result::Ok; }
int32_t UnzipStream::Read(void*, uint32_t) { return -1; }

struct LocalStorage::Impl { int dummy; };
LocalStorage::LocalStorage() : m_impl(nullptr) {}
LocalStorage::~LocalStorage() {}
Result LocalStorage::Initialize(void*) { return Result::Ok; }
void LocalStorage::Shutdown() {}
Result LocalStorage::Write(const char*, const void*, uint32_t) { return Result::Ok; }
Result LocalStorage::Read(const char*, void*, uint32_t, uint32_t& out) { out = 0u; return Result::NotFound; }
Result LocalStorage::Remove(const char*) { return Result::Ok; }

struct RamCache::Impl { int dummy; };
RamCache::RamCache() : m_impl(nullptr) {}
RamCache::~RamCache() {}
Result RamCache::Initialize(void*, uint32_t) { return Result::Ok; }
void RamCache::Shutdown() {}
Result RamCache::Put(const char*, const void*, uint32_t) { return Result::Ok; }
Result RamCache::Get(const char*, void*, uint32_t, uint32_t& out) { out = 0u; return Result::NotFound; }
void RamCache::Clear() {}

} // namespace fs

namespace audio {

struct MediaStream::Impl {
    char alias[32];
    bool loaded;
    bool playing;
    bool looping;
    uint8_t volume;
};

static uint32_t g_nextMciId = 1u;

MediaStream::MediaStream() : m_impl(nullptr) {}
MediaStream::~MediaStream() { Unload(); }

Result MediaStream::LoadFromFile(void*, const char* filePath) {
    if (!filePath) {
        return Result::InvalidArg;
    }
    Unload();

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Audio, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    ::memset(m_impl, 0, sizeof(Impl));
    m_impl->volume = 100u;
    ::snprintf(m_impl->alias, sizeof(m_impl->alias), "zsdk_mci_%u", g_nextMciId++);

    char cmd[320];
    ::snprintf(cmd, sizeof(cmd), "open \"%s\" alias %s", filePath, m_impl->alias);
    if (::mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Audio, m_impl);
        m_impl = nullptr;
        return Result::AudioError;
    }
    m_impl->loaded = true;
    return Result::Ok;
}

Result MediaStream::LoadFromMemory(void*, const void*, uint32_t, MediaFormat) {
    return Result::NotSupported;
}

void MediaStream::Unload() {
    if (!m_impl) {
        return;
    }
    if (m_impl->loaded) {
        char cmd[64];
        ::snprintf(cmd, sizeof(cmd), "close %s", m_impl->alias);
        ::mciSendStringA(cmd, nullptr, 0, nullptr);
        m_impl->loaded = false;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Audio, m_impl);
    m_impl = nullptr;
}

Result MediaStream::Play() {
    if (!m_impl || !m_impl->loaded) {
        return Result::NotInitialized;
    }
    char cmd[96];
    ::snprintf(cmd, sizeof(cmd), "play %s from 0%s", m_impl->alias,
               m_impl->looping ? " repeat" : "");
    if (::mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
        return Result::AudioError;
    }
    m_impl->playing = true;
    return Result::Ok;
}

Result MediaStream::Stop() {
    if (!m_impl || !m_impl->loaded) {
        return Result::NotInitialized;
    }
    char cmd[64];
    ::snprintf(cmd, sizeof(cmd), "stop %s", m_impl->alias);
    ::mciSendStringA(cmd, nullptr, 0, nullptr);
    m_impl->playing = false;
    return Result::Ok;
}

Result MediaStream::Pause() {
    if (!m_impl || !m_impl->loaded) {
        return Result::NotInitialized;
    }
    char cmd[64];
    ::snprintf(cmd, sizeof(cmd), "pause %s", m_impl->alias);
    return (::mciSendStringA(cmd, nullptr, 0, nullptr) == 0) ? Result::Ok : Result::AudioError;
}

Result MediaStream::Resume() {
    if (!m_impl || !m_impl->loaded) {
        return Result::NotInitialized;
    }
    char cmd[64];
    ::snprintf(cmd, sizeof(cmd), "resume %s", m_impl->alias);
    return (::mciSendStringA(cmd, nullptr, 0, nullptr) == 0) ? Result::Ok : Result::AudioError;
}

Result MediaStream::SetVolume(uint8_t volumePercent) {
    if (!m_impl || !m_impl->loaded) {
        return Result::NotInitialized;
    }
    const uint8_t clamped = (volumePercent > 100u) ? 100u : volumePercent;
    m_impl->volume = clamped;
    char cmd[96];
    ::snprintf(cmd, sizeof(cmd), "setaudio %s volume to %u",
               m_impl->alias, static_cast<unsigned>(clamped) * 10u);
    ::mciSendStringA(cmd, nullptr, 0, nullptr);
    return Result::Ok;
}

void MediaStream::SetLooping(bool loop) {
    if (m_impl) {
        m_impl->looping = loop;
    }
}

bool MediaStream::IsPlaying() const {
    if (!m_impl || !m_impl->loaded) {
        return false;
    }
    char cmd[64];
    char res[32] = {};
    ::snprintf(cmd, sizeof(cmd), "status %s mode", m_impl->alias);
    if (::mciSendStringA(cmd, res, sizeof(res), nullptr) == 0) {
        return (::strcmp(res, "playing") == 0);
    }
    return m_impl->playing;
}

bool MediaStream::IsLoaded() const {
    return m_impl && m_impl->loaded;
}

struct SoundEffect::Impl {
    uint8_t volume;
};

SoundEffect::SoundEffect() : m_impl(nullptr) {}
SoundEffect::~SoundEffect() { Shutdown(); }

Result SoundEffect::Initialize(void*) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Audio, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->volume = 100u;
    return Result::Ok;
}

void SoundEffect::Shutdown() {
    if (!m_impl) {
        return;
    }
    Stop();
    debug::MemoryTracker::Free(debug::MemorySubsystem::Audio, m_impl);
    m_impl = nullptr;
}

Result SoundEffect::PlayFile(const char* filePath) {
    if (!m_impl || !filePath) {
        return Result::InvalidArg;
    }
    return ::PlaySoundA(filePath, nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)
               ? Result::Ok
               : Result::AudioError;
}

Result SoundEffect::PlayMemory(const void* data, uint32_t sizeBytes) {
    if (!m_impl || !data || sizeBytes == 0u) {
        return Result::InvalidArg;
    }
    return ::PlaySoundA(static_cast<LPCSTR>(data), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT)
               ? Result::Ok
               : Result::AudioError;
}

Result SoundEffect::Stop() {
    ::PlaySoundA(nullptr, nullptr, 0);
    return Result::Ok;
}

Result SoundEffect::SetVolume(uint8_t volumePercent) {
    if (!m_impl) {
        return Result::NotInitialized;
    }
    const uint8_t clamped = (volumePercent > 100u) ? 100u : volumePercent;
    m_impl->volume = clamped;
    const uint32_t ch = (static_cast<uint32_t>(clamped) * 0xFFFFu) / 100u;
    const DWORD packed = static_cast<DWORD>((ch & 0xFFFFu) | ((ch & 0xFFFFu) << 16u));
    ::waveOutSetVolume(nullptr, packed);
    return Result::Ok;
}

} // namespace audio

namespace net {
struct Socket::Impl { int dummy; };
Socket::Socket() : m_impl(nullptr) {}
Socket::~Socket() {}
Result Socket::Open(void*, SocketProtocol) { return Result::Ok; }
void Socket::Close() {}
Result Socket::ConnectIPv4(uint32_t, uint16_t) { return Result::Ok; }
int32_t Socket::Send(const void*, uint32_t, Result*) { return -1; }
int32_t Socket::Recv(void*, uint32_t, Result*) { return -1; }
bool Socket::IsOpen() const { return false; }

struct HttpClient::Impl { int dummy; };
HttpClient::HttpClient() : m_impl(nullptr) {}
HttpClient::~HttpClient() {}
Result HttpClient::Initialize(void*) { return Result::Ok; }
void HttpClient::Shutdown() {}
Result HttpClient::Get(const char*, HttpResponseCallback, void*) { return Result::NotSupported; }
Result HttpClient::Post(const char*, const char*, const void*, uint32_t, HttpResponseCallback, void*) { return Result::NotSupported; }
void HttpClient::SetMaxResponseBytes(uint32_t) {}
void HttpClient::CancelPending() {}

struct NetworkManager::Impl { int dummy; };
NetworkManager::NetworkManager() : m_impl(nullptr) {}
NetworkManager::~NetworkManager() {}
Result NetworkManager::Initialize(void*) { return Result::Ok; }
void NetworkManager::Shutdown() {}
NetworkState NetworkManager::GetState() const { return NetworkState::Connected; }
bool NetworkManager::IsWifiAvailable() const { return true; }
uint32_t NetworkManager::GetLocalIpAddress() const { return 0x7F000001u; }
Result NetworkManager::ResolveHost(const char*, DnsResolveCallback, void*) { return Result::NotSupported; }
void NetworkManager::CancelDns() {}

struct BluetoothManager::Impl { int dummy; };
BluetoothManager::BluetoothManager() : m_impl(nullptr) {}
BluetoothManager::~BluetoothManager() {}
Result BluetoothManager::Initialize(void*) { return Result::Ok; }
void BluetoothManager::Shutdown() {}
bool BluetoothManager::IsSupported() const { return false; }
bool BluetoothManager::IsRadioEnabled() const { return false; }
} // namespace net

namespace security {
Result Random::GetBytes(void*, void* outBuffer, uint32_t sizeBytes) {
    uint8_t* p = static_cast<uint8_t*>(outBuffer);
    uint32_t s = ::GetTickCount();
    for (uint32_t i = 0u; i < sizeBytes; ++i) {
        s = s * 1664525u + 1013904223u;
        p[i] = static_cast<uint8_t>(s >> 24u);
    }
    return Result::Ok;
}
uint32_t Random::GetUInt32(void* ctx) {
    uint32_t v = 0u; GetBytes(ctx, &v, sizeof(v)); return v;
}
Result LicenseManager::CheckLicense(void*, LicenseStatus& outStatus) {
    outStatus = LicenseStatus{true, false, 999u};
    return Result::Ok;
}
Result Hash::Compute(void*, HashAlgorithm algo, const void*, uint32_t, void* outDigest, uint32_t digestBufferBytes, uint32_t* outDigestBytes) {
    const uint32_t need = (algo == HashAlgorithm::Md5) ? 16u : 20u;
    if (!outDigest || digestBufferBytes < need) return Result::InvalidArg;
    memset(outDigest, 0, need);
    if (outDigestBytes) *outDigestBytes = need;
    return Result::NotSupported;
}
Result Hmac::Compute(void*, HashAlgorithm algo, const void*, uint32_t, const void*, uint32_t, void* outMac, uint32_t macBufferBytes, uint32_t* outMacBytes) {
    const uint32_t need = (algo == HashAlgorithm::Md5) ? 16u : 20u;
    if (!outMac || macBufferBytes < need) return Result::InvalidArg;
    memset(outMac, 0, need);
    if (outMacBytes) *outMacBytes = need;
    return Result::NotSupported;
}
Result Rsa::ModExp(void*, const void*, uint32_t, const void*, uint32_t, const void*, uint32_t, void*, uint32_t, uint32_t*) {
    return Result::NotSupported;
}
} // namespace security

} // namespace zsdk

/* ============================================================================
 * WinMain / main Entry Point Bridge for ZSDK_IMPLEMENT_APP
 * ========================================================================== */

int main(int, char**) {
    return zsdk::ZeeboAppBridge::Run();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return zsdk::ZeeboAppBridge::Run();
}

#endif /* ZSDK_PLATFORM_WINDOWS */
