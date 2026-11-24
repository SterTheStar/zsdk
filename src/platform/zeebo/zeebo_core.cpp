/**
 * @file zeebo_core.cpp
 * @brief Qualcomm BREW / Zeebo bridge for Clock and Application lifecycle.
 *
 * Isolates AEEApplet, IShell, IDisplay, AEEDeviceInfo, and ISHELL_SetTimer
 * from the public C++ headers.
 */

#include "zsdk/core/app.hpp"
#include "zsdk/core/clock.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/logger.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEE.h"
#include "AEEAppGen.h"
#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEDisp.h"
#include "AEEModGen.h"

namespace zsdk {

uint32_t Clock::GetUptimeMs() {
    return GETUPTIMEMS();
}

uint32_t Clock::GetWallTimeMs() {
    return GETTIMEMS();
}

struct Application::PlatformState {
    AEEApplet   applet;         /* Must remain first for BREW AEEApplet* casts */
    Application* owner;
    int32_t     screenWidth;
    int32_t     screenHeight;
    uint32_t    frameIntervalMs;
    bool        timerActive;
};

/**
 * @brief Internal BREW event & timer bridge.
 */
class ZeeboAppBridge {
public:
    static void FrameTickCallback(void* userData) {
        Application::PlatformState* ps = static_cast<Application::PlatformState*>(userData);
        if (!ps || !ps->owner) {
            return;
        }
        Application* app = ps->owner;
        if (app->m_quitRequested) {
            ISHELL_CloseApplet(ps->applet.m_pIShell, FALSE);
            return;
        }

        const uint32_t frameStartMs = GETUPTIMEMS();

        if (!app->m_suspended && app->m_delegate) {
            const uint32_t dtMs = app->m_timer.Tick();
            app->m_input.Update(dtMs);
            debug::DebugOverlay::Update(app->m_input, static_cast<float>(dtMs));

            app->m_delegate->OnUpdate(dtMs);
            app->m_delegate->OnRender();
        }

        if (ps->timerActive && !app->m_quitRequested) {
            const uint32_t nowMs = GETUPTIMEMS();
            const uint32_t workMs = (nowMs >= frameStartMs) ? (nowMs - frameStartMs) : 0u;
            uint32_t nextDelayMs = (ps->frameIntervalMs > workMs)
                                       ? (ps->frameIntervalMs - workMs)
                                       : 1u;
            if (nextDelayMs == 0u) {
                nextDelayMs = 1u;
            }
            ISHELL_SetTimer(ps->applet.m_pIShell,
                            static_cast<int32>(nextDelayMs),
                            FrameTickCallback, ps);
        }
    }

    static boolean HandleEvent(Application::PlatformState* ps,
                               AEEEvent eCode, uint16 wParam, uint32 dwParam) {
        if (!ps || !ps->owner) {
            return FALSE;
        }
        Application* app = ps->owner;

        switch (eCode) {
            case EVT_APP_START: {
                AEEDeviceInfo devInfo;
                MEMSET(&devInfo, 0, sizeof(devInfo));
                devInfo.wStructSize = sizeof(devInfo);
                ISHELL_GetDeviceInfo(ps->applet.m_pIShell, &devInfo);
                ps->screenWidth  = static_cast<int32_t>(devInfo.cxScreen);
                ps->screenHeight = static_cast<int32_t>(devInfo.cyScreen);

                app->m_timer.Reset();
                app->m_input.Initialize(ps->applet.m_pIShell);
                debug::DebugOverlay::Initialize();
                debug::DebugOverlay::SetVisible(app->m_config.enableDebugOverlay);

                if (app->m_delegate) {
                    const Result res = app->m_delegate->OnInit();
                    if (Failed(res)) {
                        ZSDK_LOG_ERROR("CORE", "OnInit failed (%d)", static_cast<int>(res));
                        return FALSE;
                    }
                }

                const uint16_t fps = (app->m_config.targetFps > 0) ? app->m_config.targetFps : 60;
                ps->frameIntervalMs = 1000u / fps;
                if (ps->frameIntervalMs == 0u) {
                    ps->frameIntervalMs = 1u;
                }
                ps->timerActive = true;
                ISHELL_SetTimer(ps->applet.m_pIShell,
                                static_cast<int32>(ps->frameIntervalMs),
                                FrameTickCallback, ps);
                return TRUE;
            }

            case EVT_APP_STOP:
                ps->timerActive = false;
                ISHELL_CancelTimer(ps->applet.m_pIShell, FrameTickCallback, ps);
                if (app->m_delegate) {
                    app->m_delegate->OnShutdown();
                }
                debug::DebugOverlay::Shutdown();
                app->m_input.Shutdown();
                return TRUE;

            case EVT_APP_SUSPEND:
                app->m_suspended = true;
                ps->timerActive = false;
                ISHELL_CancelTimer(ps->applet.m_pIShell, FrameTickCallback, ps);
                if (app->m_delegate) {
                    app->m_delegate->OnSuspend();
                }
                return TRUE;

            case EVT_APP_RESUME:
                app->m_suspended = false;
                app->m_timer.Reset();
                if (app->m_delegate) {
                    app->m_delegate->OnResume();
                }
                ps->timerActive = true;
                ISHELL_SetTimer(ps->applet.m_pIShell,
                                static_cast<int32>(ps->frameIntervalMs),
                                FrameTickCallback, ps);
                return TRUE;

            case EVT_KEY:
            case EVT_KEY_PRESS:
            case EVT_KEY_RELEASE:
                if (app->m_input.HandlePlatformEvent(static_cast<uint32_t>(eCode), wParam, dwParam)) {
                    return TRUE;
                }
                return FALSE;

            default:
                return FALSE;
        }
    }

    static void FreeAppData(IApplet* pi) {
        Application::PlatformState* ps = reinterpret_cast<Application::PlatformState*>(pi);
        if (ps && ps->owner) {
            ps->owner->m_platform = nullptr;
            ps->owner = nullptr;
        }
    }

    static void AttachPlatform(Application* app, Application::PlatformState* ps) {
        app->m_platform = ps;
    }
};

void* Application::GetNativePlatformContext() const {
    return m_platform ? static_cast<void*>(m_platform->applet.m_pIShell) : nullptr;
}

int32_t Application::GetScreenWidth() const {
    return m_platform ? m_platform->screenWidth : 640;
}

int32_t Application::GetScreenHeight() const {
    return m_platform ? m_platform->screenHeight : 480;
}

void Application::SetTargetFps(uint16_t fps) {
    const uint16_t clamped = (fps == 0u) ? 60u : fps;
    m_config.targetFps = clamped;
    if (m_platform) {
        uint32_t interval = 1000u / static_cast<uint32_t>(clamped);
        m_platform->frameIntervalMs = (interval > 0u) ? interval : 1u;
    }
}

} // namespace zsdk

/**
 * @brief Standard BREW module entry point, completely hidden from ZSDK user code.
 */
extern "C" void _fini(void) {}

static zsdk::Application g_zsdkApp;

extern "C" int AEEClsCreateInstance(AEECLSID clsId, IShell* pIShell, IModule* pIModule, void** ppObj) {
    zsdk::AppConfig cfg = {"ZSDK App", 60, ZSDK_DEFAULT_MEMORY_LIMIT_BYTES, true};
    zsdk::IAppDelegate* delegate = zsdk::CreateUserApp(cfg);
    if (!delegate || !ppObj) {
        return EFAILED;
    }

    g_zsdkApp.Configure(cfg, delegate);

    if (!AEEApplet_New(sizeof(zsdk::Application::PlatformState), clsId, pIShell, pIModule,
                       reinterpret_cast<IApplet**>(ppObj),
                       reinterpret_cast<AEEHANDLER>(zsdk::ZeeboAppBridge::HandleEvent),
                       zsdk::ZeeboAppBridge::FreeAppData)) {
        return ENOMEMORY;
    }

    zsdk::Application::PlatformState* ps =
        reinterpret_cast<zsdk::Application::PlatformState*>(*ppObj);
    ps->owner = &g_zsdkApp;
    ps->screenWidth = 640;
    ps->screenHeight = 480;
    ps->frameIntervalMs = 16;
    ps->timerActive = false;
    zsdk::ZeeboAppBridge::AttachPlatform(&g_zsdkApp, ps);

    return AEE_SUCCESS;
}

#endif /* ZSDK_PLATFORM_ZEEBO */
