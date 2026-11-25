/**
 * @file zsdk_rust_host.cpp
 * @brief Generic ZSDK Rust application host (used automatically by zsdk_add_rust_app).
 */

#include "zsdk/zsdk.hpp"
#include "zsdk/c/zsdk_c_app.h"

#ifndef ZSDK_RUST_APP_NAME
    #define ZSDK_RUST_APP_NAME "ZSDK_Rust_App"
#endif

class ZsdkRustHostApp : public zsdk::IAppDelegate {
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
        if (app && app->GetInput().GetGamepad(0).IsPressed(zsdk::input::GamepadButton::Home)) {
            app->RequestQuit();
            return;
        }
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
ZSDK_IMPLEMENT_APP(ZsdkRustHostApp, ZSDK_RUST_APP_NAME, 30, true)
#else
ZSDK_IMPLEMENT_APP(ZsdkRustHostApp, ZSDK_RUST_APP_NAME, 60, true)
#endif
