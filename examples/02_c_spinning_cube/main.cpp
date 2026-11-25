/**
 * @file main.cpp
 * @brief Thin C++ Application entry bridge forwarding to pure C99 cube_c.c.
 */

#include "zsdk/zsdk.hpp"
#include "cube_c.h"

class CSpinningCubeApp : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        zsdk::Application* app = zsdk::Application::GetInstance();
        if (!app || cube_c_init(app->GetNativePlatformContext()) != 0) {
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
        cube_c_update(static_cast<float>(deltaTimeMs) * 0.001f, inp);
    }

    void OnRender() override {
        int32_t w = 640;
        int32_t h = 480;
        cube_c_render(&w, &h);
        zsdk::debug::DebugOverlay::Render(w, h);
        cube_c_swap();
    }

    void OnShutdown() override {
        cube_c_shutdown();
    }
};

#if ZSDK_PLATFORM_ZEEBO
ZSDK_IMPLEMENT_APP(CSpinningCubeApp, "ZSDK_C_Cube", 30, true)
#else
ZSDK_IMPLEMENT_APP(CSpinningCubeApp, "ZSDK_C_Cube", 60, true)
#endif
