/**
 * @file zsdk_lua_host.cpp
 * @brief Generic ZSDK Lua application host (used automatically by zsdk_add_lua_app).
 */

#include "zsdk/zsdk.hpp"
#include "zsdk_lua.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#ifndef ZSDK_LUA_APP_NAME
    #define ZSDK_LUA_APP_NAME "ZSDK_Lua_App"
#endif

class ZsdkLuaHostApp : public zsdk::IAppDelegate {
public:
    ZsdkLuaHostApp()
        : m_L(nullptr)
        , m_inputRef(LUA_NOREF) {
    }

    zsdk::Result OnInit() override {
        zsdk::Application* app = zsdk::Application::GetInstance();
        if (!app) {
            return zsdk::Result::Error;
        }

        zsdk::graphics::GLContextConfig cfg = { 5u, 6u, 5u, 0u, 16u, 0u, 1u };
        if (zsdk::Failed(m_gl.Initialize(app->GetNativePlatformContext(), cfg))) {
            return zsdk::Result::GraphicsError;
        }

        zsdk::fs::FileSystem fs;
        if (zsdk::Failed(fs.Initialize(app->GetNativePlatformContext()))) {
            return zsdk::Result::IoError;
        }

#if ZSDK_PLATFORM_ZEEBO
        const char* scriptPath = "fs:/~/main.lua";
#else
        const char* scriptPath = "main.lua";
#endif
        void* scriptBuf = nullptr;
        uint32_t scriptSize = 0u;
        if (zsdk::Failed(fs.ReadAllBytes(scriptPath, &scriptBuf, scriptSize)) || !scriptBuf) {
            ZSDK_LOG_ERROR("LUA", "Failed to load %s", scriptPath);
            return zsdk::Result::NotFound;
        }

        m_L = luaL_newstate();
        if (!m_L) {
            zsdk::fs::FileSystem::FreeBuffer(scriptBuf);
            return zsdk::Result::OutOfMemory;
        }
        luaL_openlibs(m_L);
        luaL_requiref(m_L, "zsdk", luaopen_zsdk, 1);
        lua_pop(m_L, 1);

        if (luaL_loadbuffer(m_L, static_cast<const char*>(scriptBuf), scriptSize, scriptPath) != LUA_OK ||
            lua_pcall(m_L, 0, 0, 0) != LUA_OK) {
            ZSDK_LOG_ERROR("LUA", "Script error: %s", lua_tostring(m_L, -1));
            zsdk::fs::FileSystem::FreeBuffer(scriptBuf);
            return zsdk::Result::Error;
        }
        zsdk::fs::FileSystem::FreeBuffer(scriptBuf);

        zsdk_lua_push_input_view(m_L, &app->GetInput());
        m_inputRef = luaL_ref(m_L, LUA_REGISTRYINDEX);

        lua_getglobal(m_L, "zsdk_on_init");
        if (lua_isfunction(m_L, -1)) {
            lua_pushinteger(m_L, m_gl.GetWidth());
            lua_pushinteger(m_L, m_gl.GetHeight());
            if (lua_pcall(m_L, 2, 0, 0) != LUA_OK) {
                ZSDK_LOG_ERROR("LUA", "zsdk_on_init: %s", lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
            }
        } else {
            lua_pop(m_L, 1);
        }

        return zsdk::Result::Ok;
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        zsdk::Application* app = zsdk::Application::GetInstance();
        if (app && app->GetInput().GetGamepad(0).IsPressed(zsdk::input::GamepadButton::Home)) {
            app->RequestQuit();
            return;
        }
        if (!m_L) {
            return;
        }
        lua_getglobal(m_L, "zsdk_on_update");
        if (lua_isfunction(m_L, -1)) {
            lua_pushinteger(m_L, static_cast<lua_Integer>(deltaTimeMs));
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_inputRef);
            if (lua_pcall(m_L, 2, 0, 0) != LUA_OK) {
                ZSDK_LOG_ERROR("LUA", "zsdk_on_update: %s", lua_tostring(m_L, -1));
                lua_pop(m_L, 1);
            }
        } else {
            lua_pop(m_L, 1);
        }
    }

    void OnRender() override {
        const int32_t w = m_gl.GetWidth();
        const int32_t h = m_gl.GetHeight();

        if (m_L) {
            lua_getglobal(m_L, "zsdk_on_render");
            if (lua_isfunction(m_L, -1)) {
                lua_pushinteger(m_L, w);
                lua_pushinteger(m_L, h);
                if (lua_pcall(m_L, 2, 0, 0) != LUA_OK) {
                    ZSDK_LOG_ERROR("LUA", "zsdk_on_render: %s", lua_tostring(m_L, -1));
                    lua_pop(m_L, 1);
                }
            } else {
                lua_pop(m_L, 1);
            }
        }

        zsdk::debug::DebugOverlay::Render(w, h);
        m_gl.SwapBuffers();
    }

    void OnShutdown() override {
        if (m_L) {
            if (m_inputRef != LUA_NOREF) {
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_inputRef);
                m_inputRef = LUA_NOREF;
            }
            lua_close(m_L);
            m_L = nullptr;
        }
        m_gl.Shutdown();
    }

private:
    zsdk::graphics::GLContext m_gl;
    lua_State*                m_L;
    int                       m_inputRef;
};

#if ZSDK_PLATFORM_ZEEBO
ZSDK_IMPLEMENT_APP(ZsdkLuaHostApp, ZSDK_LUA_APP_NAME, 30, true)
#else
ZSDK_IMPLEMENT_APP(ZsdkLuaHostApp, ZSDK_LUA_APP_NAME, 60, true)
#endif
