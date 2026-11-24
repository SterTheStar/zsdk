/**
 * @file virtual_keyboard.hpp
 * @brief Zero-allocation on-screen Virtual Keyboard for Z-Pad (Zeebo & Windows).
 *
 * Renders on the lower half of the 640x480 virtual display (matching Zeebo
 * Developer Guide 10.2) without requiring proprietary Zeebo.lib binaries.
 */

#ifndef ZSDK_INPUT_VIRTUAL_KEYBOARD_HPP
#define ZSDK_INPUT_VIRTUAL_KEYBOARD_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/input/input.hpp"

namespace zsdk {
namespace input {

static const uint8_t kVirtualKeyboardMaxChars = 63u;

class VirtualKeyboard {
public:
    VirtualKeyboard();

    void Open(const char* title, const char* initialText = "", uint8_t maxLen = 32u);
    void Close();

    /** @brief Polls D-Pad / Action1 (type) / Action2 (backspace) / Start (OK) / Back (Cancel). */
    void Update(const GamepadState& pad);

    /** @brief Renders the keyboard overlay in the lower half of the screen. */
    void Render(int32_t viewportWidth, int32_t viewportHeight) const;

    ZSDK_FORCE_INLINE bool        IsActive() const { return m_active; }
    ZSDK_FORCE_INLINE bool        IsSubmitted() const { return m_submitted; }
    ZSDK_FORCE_INLINE bool        IsCancelled() const { return m_cancelled; }
    ZSDK_FORCE_INLINE const char* GetText() const { return m_buffer; }

private:
    char    m_title[32];
    char    m_buffer[kVirtualKeyboardMaxChars + 1u];
    uint8_t m_len;
    uint8_t m_maxLen;
    uint8_t m_row;
    uint8_t m_col;
    bool    m_uppercase;
    bool    m_active;
    bool    m_submitted;
    bool    m_cancelled;
};

} // namespace input
} // namespace zsdk

#endif /* ZSDK_INPUT_VIRTUAL_KEYBOARD_HPP */
