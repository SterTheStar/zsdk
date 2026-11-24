/**
 * @file input.hpp
 * @brief Zero-allocation per-frame Input & Controller API (Z-Pad, Boomerang, HID).
 *
 * Encapsulates BREW's IHID / IHIDDevice / AEEHIDButton / AEEHIDThumbstick
 * and AVK_* key events behind a deterministic, polling-friendly C++ interface.
 */

#ifndef ZSDK_INPUT_INPUT_HPP
#define ZSDK_INPUT_INPUT_HPP

#include "zsdk/input/input_types.hpp"

namespace zsdk {
namespace input {

/**
 * @brief Per-controller state updated once per frame with zero heap allocations.
 */
class GamepadState {
public:
    GamepadState();

    /** @brief True if the controller slot is currently connected. */
    ZSDK_FORCE_INLINE bool IsConnected() const { return m_connected; }

    /** @brief True while @p button is held down. */
    ZSDK_FORCE_INLINE bool IsDown(GamepadButton button) const {
        return (m_currentMask & ButtonMask(button)) != 0;
    }

    /** @brief True only on the frame @p button transitioned from up to down. */
    ZSDK_FORCE_INLINE bool IsPressed(GamepadButton button) const {
        const uint32_t mask = ButtonMask(button);
        return ((m_currentMask & mask) != 0) && ((m_previousMask & mask) == 0);
    }

    /** @brief True only on the frame @p button transitioned from down to up. */
    ZSDK_FORCE_INLINE bool IsReleased(GamepadButton button) const {
        const uint32_t mask = ButtonMask(button);
        return ((m_currentMask & mask) == 0) && ((m_previousMask & mask) != 0);
    }

    /**
     * @brief True if all buttons in @p comboMask are currently down,
     *        and at least one of them was just pressed this frame (ideal for shortcuts).
     */
    ZSDK_FORCE_INLINE bool IsComboJustTriggered(uint32_t comboMask) const {
        const bool allDown = (m_currentMask & comboMask) == comboMask;
        const bool wasAllDown = (m_previousMask & comboMask) == comboMask;
        return allDown && !wasAllDown;
    }

    /** @brief Returns how long @p button has been held down in milliseconds. */
    uint32_t GetHoldDurationMs(GamepadButton button) const;

    /** @brief Normalized left analog stick [-1.0..1.0] with deadzone applied. */
    ZSDK_FORCE_INLINE Vec2 GetLeftStick() const { return m_leftStick; }

    /** @brief Normalized right analog stick [-1.0..1.0] with deadzone applied. */
    ZSDK_FORCE_INLINE Vec2 GetRightStick() const { return m_rightStick; }

    /** @brief Motion / accelerometer state (Zeebo Boomerang). */
    ZSDK_FORCE_INLINE const MotionState& GetMotion() const { return m_motion; }

    /** @brief Configures radial deadzone for analog sticks. */
    void SetAnalogConfig(const AnalogConfig& config);

private:
    friend class InputManager;
    friend class ZeeboInputBridge;

    void BeginFrame(uint32_t deltaTimeMs);
    void SetButtonState(GamepadButton button, bool down);
    void SetRawSticks(int16_t lx, int16_t ly, int16_t rx, int16_t ry);

    bool         m_connected;
    uint32_t     m_currentMask;
    uint32_t     m_previousMask;
    uint32_t     m_holdDurationMs[static_cast<size_t>(GamepadButton::Count)];
    Vec2         m_leftStick;
    Vec2         m_rightStick;
    AnalogConfig m_analogConfig;
    MotionState  m_motion;
};

/**
 * @brief Central input manager owning up to ZSDK_MAX_GAMEPADS controllers.
 */
class InputManager : public NonCopyable {
public:
    InputManager();
    ~InputManager();

    /** @brief Initializes IHID / BREW key fallback (zero allocations per frame). */
    Result Initialize(void* platformContext);

    /** @brief Releases all IHIDDevice handles and thumbstick mappings. */
    void Shutdown();

    /**
     * @brief Polls HID button/axis queues and advances frame state & hold timers.
     * @param deltaTimeMs Frame delta in milliseconds.
     */
    void Update(uint32_t deltaTimeMs);

    /** @brief Feeds a BREW EVT_KEY / EVT_KEY_PRESS / EVT_KEY_RELEASE event into P1. */
    bool HandlePlatformEvent(uint32_t eventCode, uint16_t keyCode, uint32_t param);

    /** @brief Returns read-only state for controller @p playerIndex [0..ZSDK_MAX_GAMEPADS-1]. */
    const GamepadState& GetGamepad(uint8_t playerIndex = 0) const;

    /** @brief Returns mutable state (e.g. to configure deadzones) for @p playerIndex. */
    GamepadState& GetGamepadMutable(uint8_t playerIndex = 0);

    /** @brief Number of currently connected controllers. */
    uint8_t GetConnectedCount() const;

    /**
     * @brief Sets dual-motor rumble intensity [0..65535] for @p playerIndex.
     *        Wraps IHIDDevice_Rumble().
     */
    Result SetRumble(uint8_t playerIndex, uint16_t leftMotor, uint16_t rightMotor);

private:
    friend class ZeeboInputBridge;

    struct Impl;
    Impl*        m_impl;
    GamepadState m_gamepads[ZSDK_MAX_GAMEPADS];
};

} // namespace input
} // namespace zsdk

#endif /* ZSDK_INPUT_INPUT_HPP */
