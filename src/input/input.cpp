/**
 * @file input.cpp
 * @brief Platform-agnostic GamepadState deadzones, hold timers, and stick normalization.
 */

#include "zsdk/input/input.hpp"

#include <math.h>

namespace zsdk {
namespace input {

namespace {

Vec2 NormalizeStickWithDeadzone(int16_t rawX, int16_t rawY, const AnalogConfig& cfg) {
    float x = static_cast<float>(rawX) / 32767.0f;
    float y = static_cast<float>(rawY) / 32767.0f;
    if (x < -1.0f) x = -1.0f;
    if (y < -1.0f) y = -1.0f;

    const float magSq = x * x + y * y;
    const float inner = cfg.innerDeadzone;
    if (magSq <= inner * inner) {
        return Vec2{0.0f, 0.0f};
    }

    const float mag = sqrtf(magSq);
    const float outer = (cfg.outerDeadzone > inner) ? cfg.outerDeadzone : 1.0f;
    float norm = (mag - inner) / (outer - inner);
    if (norm > 1.0f) {
        norm = 1.0f;
    }

    const float scale = norm / mag;
    return Vec2{x * scale, y * scale};
}

} // namespace

GamepadState::GamepadState()
    : m_connected(false)
    , m_currentMask(0u)
    , m_previousMask(0u)
    , m_leftStick{0.0f, 0.0f}
    , m_rightStick{0.0f, 0.0f}
    , m_analogConfig{0.15f, 0.95f}
    , m_motion{false, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}} {
    for (size_t i = 0; i < static_cast<size_t>(GamepadButton::Count); ++i) {
        m_holdDurationMs[i] = 0u;
    }
}

uint32_t GamepadState::GetHoldDurationMs(GamepadButton button) const {
    const size_t idx = static_cast<size_t>(button);
    if (idx >= static_cast<size_t>(GamepadButton::Count)) {
        return 0u;
    }
    return m_holdDurationMs[idx];
}

void GamepadState::SetAnalogConfig(const AnalogConfig& config) {
    m_analogConfig = config;
}

void GamepadState::BeginFrame(uint32_t deltaTimeMs) {
    m_previousMask = m_currentMask;
    for (size_t i = 0; i < static_cast<size_t>(GamepadButton::Count); ++i) {
        const uint32_t mask = 1u << static_cast<uint32_t>(i);
        if ((m_currentMask & mask) != 0u) {
            m_holdDurationMs[i] += deltaTimeMs;
        } else {
            m_holdDurationMs[i] = 0u;
        }
    }
}

void GamepadState::SetButtonState(GamepadButton button, bool down) {
    const uint32_t mask = ButtonMask(button);
    if (down) {
        m_currentMask |= mask;
    } else {
        m_currentMask &= ~mask;
    }
}

void GamepadState::SetRawSticks(int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    m_leftStick  = NormalizeStickWithDeadzone(lx, ly, m_analogConfig);
    m_rightStick = NormalizeStickWithDeadzone(rx, ry, m_analogConfig);
}

const GamepadState& InputManager::GetGamepad(uint8_t playerIndex) const {
    const uint8_t idx = (playerIndex < ZSDK_MAX_GAMEPADS) ? playerIndex : 0u;
    return m_gamepads[idx];
}

GamepadState& InputManager::GetGamepadMutable(uint8_t playerIndex) {
    const uint8_t idx = (playerIndex < ZSDK_MAX_GAMEPADS) ? playerIndex : 0u;
    return m_gamepads[idx];
}

uint8_t InputManager::GetConnectedCount() const {
    uint8_t count = 0u;
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        if (m_gamepads[i].IsConnected()) {
            ++count;
        }
    }
    return count;
}

} // namespace input
} // namespace zsdk
