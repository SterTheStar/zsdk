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
    , m_suspended(false) {
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

bool Application::IsQuitRequested() const {
    return m_quitRequested;
}

} // namespace zsdk
