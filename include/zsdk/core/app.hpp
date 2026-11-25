/**
 * @file app.hpp
 * @brief Application lifecycle & BREW AEEApplet / IShell abstraction.
 *
 * Isolates AEEApplet, IShell, IDisplay, and AEEEvent from user code.
 */

#ifndef ZSDK_CORE_APP_HPP
#define ZSDK_CORE_APP_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/core/clock.hpp"
#include "zsdk/input/input.hpp"

namespace zsdk {

/**
 * @brief Standard FPS cap presets for Zeebo / BREW games.
 */
enum class FpsCap : uint16_t {
    Fps30 = 30,
    Fps60 = 60
};

/**
 * @brief Startup configuration for a ZSDK application.
 */
struct AppConfig {
    const char* name;
    uint16_t    targetFps;          /**< Target frame rate (e.g., 30 or 60). */
    uint32_t    memoryBudgetBytes;  /**< Budget for profiler (default 32 MiB). */
    bool        enableDebugOverlay; /**< Initial visibility of DebugOverlay. */
};

/**
 * @brief Interface implemented by the game or engine using ZSDK.
 */
class IAppDelegate {
public:
    virtual ~IAppDelegate() {}

    /** @brief Called once after BREW/EGL/Input initialization succeeds. */
    virtual Result OnInit() = 0;

    /** @brief Called once per frame with clamped delta time in milliseconds. */
    virtual void OnUpdate(uint32_t deltaTimeMs) = 0;

    /** @brief Called once per frame to issue OpenGL ES rendering commands. */
    virtual void OnRender() = 0;

    /** @brief Called when the system suspends the application (e.g. Zeebo Home). */
    virtual void OnSuspend() {}

    /** @brief Called when the system resumes the application. */
    virtual void OnResume() {}

    /**
     * @brief A gamepad appeared in slot @p playerIndex (also fired once per pad already
     *        connected when the first frame runs). Zeebo requires games to handle gamepad
     *        detection: typically resume or hide a "connect a controller" screen.
     */
    virtual void OnGamepadConnected(uint8_t playerIndex) { ZSDK_UNUSED(playerIndex); }

    /** @brief A gamepad was removed from slot @p playerIndex; pause and prompt the player. */
    virtual void OnGamepadDisconnected(uint8_t playerIndex) { ZSDK_UNUSED(playerIndex); }

    /** @brief Called before shutdown to release game assets. */
    virtual void OnShutdown() = 0;
};

/**
 * @brief Singleton-like host owning the platform bridge, main loop, and InputManager.
 */
class Application : public NonCopyable {
public:
    Application();
    ~Application();

    /** @brief Returns the active Application instance (nullptr if not created). */
    static Application* GetInstance();

    /** @brief Configures and attaches the user delegate. */
    void Configure(const AppConfig& config, IAppDelegate* delegate);

    /** @brief Accesses the platform InputManager. */
    ZSDK_FORCE_INLINE input::InputManager& GetInput() { return m_input; }
    ZSDK_FORCE_INLINE const input::InputManager& GetInput() const { return m_input; }

    /** @brief Accesses the frame timer. */
    ZSDK_FORCE_INLINE const FrameTimer& GetTimer() const { return m_timer; }

    /** @brief Returns the opaque native platform context (IShell* on Zeebo). */
    void* GetNativePlatformContext() const;

    /** @brief Requests clean application exit. */
    void RequestQuit();

    /** @brief Returns true if quit was requested. */
    bool IsQuitRequested() const;

    /** @brief Current surface dimensions in pixels. */
    int32_t GetScreenWidth() const;
    int32_t GetScreenHeight() const;

    /** @brief Dynamically sets the target FPS cap (e.g. 30 or 60). */
    void     SetTargetFps(uint16_t fps);
    void     SetFpsCap(FpsCap cap) { SetTargetFps(static_cast<uint16_t>(cap)); }
    uint16_t GetTargetFps() const { return m_config.targetFps; }

    struct PlatformState;

    /**
     * @brief Platform backends call this right after InputManager::Update(): it diffs the
     *        connected-pad mask and fires IAppDelegate::OnGamepadConnected/Disconnected.
     */
    void DispatchGamepadChanges();

private:
    friend class ZeeboAppBridge;

    AppConfig            m_config;
    IAppDelegate*        m_delegate;
    input::InputManager  m_input;
    FrameTimer           m_timer;
    PlatformState*       m_platform;
    bool                 m_quitRequested;
    bool                 m_suspended;
    uint8_t              m_padMask;   /**< bit i = pad i was connected at the last dispatch */
};

/**
 * @brief User-provided factory hook invoked by the platform entry point.
 *        Use ZSDK_IMPLEMENT_APP(...) in your main.cpp to define it automatically.
 */
IAppDelegate* CreateUserApp(AppConfig& outConfig);

} // namespace zsdk

/**
 * @brief Registers the user's IAppDelegate without exposing BREW/Win32 entry points.
 */
#define ZSDK_IMPLEMENT_APP(DelegateClass, AppName, TargetFps, EnableDebugOverlay) \
    static DelegateClass g_zsdkUserDelegate;                                      \
    namespace zsdk {                                                              \
    IAppDelegate* CreateUserApp(AppConfig& outConfig) {                           \
        outConfig.name               = (AppName);                                 \
        outConfig.targetFps          = (TargetFps);                               \
        outConfig.memoryBudgetBytes  = ZSDK_DEFAULT_MEMORY_LIMIT_BYTES;           \
        outConfig.enableDebugOverlay = (EnableDebugOverlay);                      \
        return &g_zsdkUserDelegate;                                               \
    }                                                                             \
    }

#endif /* ZSDK_CORE_APP_HPP */
