/**
 * @file zsdk_c_host.cpp
 * @brief Generic ZSDK C application host (used automatically by zsdk_add_c_app).
 */

#include "zsdk/zsdk.hpp"
#include "zsdk/c/zsdk_c_app.h"

#ifndef ZSDK_C_APP_NAME
    #define ZSDK_C_APP_NAME "ZSDK_C_App"
#endif

class ZsdkCHostApp : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        zsdk::Application* app = zsdk::Application::GetInstance();
        if (!app || zsdk_c_app_init(app->GetNativePlatformContext()) != 0) {
            return zsdk::Result::GraphicsError;
        }
        return zsdk::Result::Ok;
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        zsdk::Application* app = zsdk::Application::GetInstance();
        const zsdk_input_t* inp = app
            ? reinterpret_cast<const zsdk_input_t*>(&app->GetInput())
            : nullptr;
        zsdk_c_app_update(deltaTimeMs, inp);
    }

    void OnRender() override {
        int32_t w = 640;
        int32_t h = 480;
        zsdk_c_app_render(&w, &h);
        zsdk::debug::DebugOverlay::Render(w, h);
        zsdk_c_app_swap();
    }

    void OnShutdown() override {
        zsdk_c_app_shutdown();
    }
};

#if ZSDK_PLATFORM_ZEEBO
ZSDK_IMPLEMENT_APP(ZsdkCHostApp, ZSDK_C_APP_NAME, 30, true)
#else
ZSDK_IMPLEMENT_APP(ZsdkCHostApp, ZSDK_C_APP_NAME, 60, true)
#endif
