/**
 * @file keyboard_mouse.hpp
 * @brief HID Keyboard & Mouse input abstraction (AEEHIDDevice_Keyboard.h / Mouse.h).
 */

#ifndef ZSDK_INPUT_KEYBOARD_MOUSE_HPP
#define ZSDK_INPUT_KEYBOARD_MOUSE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace input {

/**
 * @brief Polls USB/Bluetooth HID keyboard and mouse devices connected via IHID.
 */
class KeyboardMouseManager : public NonCopyable {
public:
    KeyboardMouseManager();
    ~KeyboardMouseManager();

    Result Initialize(void* platformContext);
    void   Shutdown();
    void   Update();

    bool    IsKeyDown(uint16_t hidKeyCode) const;
    Vec2    GetMouseDelta() const;
    uint8_t GetMouseButtonsMask() const;
    bool    HasKeyboard() const;
    bool    HasMouse() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace input
} // namespace zsdk

#endif /* ZSDK_INPUT_KEYBOARD_MOUSE_HPP */
