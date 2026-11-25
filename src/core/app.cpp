/**
 * @file app.cpp
 * @brief Platform-independent Application singleton & configuration.
 */

#include "zsdk/core/app.hpp"
#include "zsdk/debug/memory_tracker.hpp"

namespace zsdk {

namespace {
Application* g_appInstance = nullptr;
} // namespace

Application::Application()
    : m_config{"ZSDK App", 60, ZSDK_DEFAULT_MEMORY_LIMIT_BYTES, false}
    , m_delegate(nullptr)
    , m_platform(nullptr)
    , m_quitRequested(false)
    , m_suspended(false)
    , m_padMask(0u) {
    g_appInstance = this;
}

Application::~Application() {
    if (g_appInstance == this) {
        g_appInstance = nullptr;
    }
}

Application* Application::GetInstance() {
    return g_appInstance;
}

void Application::Configure(const AppConfig& config, IAppDelegate* delegate) {
    m_config = config;
    m_delegate = delegate;
    debug::MemoryTracker::SetBudgetBytes(config.memoryBudgetBytes);
}

void Application::RequestQuit() {
    m_quitRequested = true;
}

void Application::DispatchGamepadChanges() {
    if (m_delegate == nullptr) {
        return;
    }
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS && i < 8u; ++i) {
        const bool now = m_input.GetGamepad(i).IsConnected();
        const bool was = (m_padMask & (1u << i)) != 0u;
        if (now == was) {
            continue;
        }
        m_padMask = static_cast<uint8_t>(now ? (m_padMask | (1u << i)) : (m_padMask & ~(1u << i)));
        if (now) {
            m_delegate->OnGamepadConnected(i);
        } else {
            m_delegate->OnGamepadDisconnected(i);
        }
    }
}

bool Application::IsQuitRequested() const {
    return m_quitRequested;
}

} // namespace zsdk
