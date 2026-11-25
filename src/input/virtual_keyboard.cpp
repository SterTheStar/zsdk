/**
 * @file virtual_keyboard.cpp
 * @brief Zero-allocation on-screen Virtual Keyboard for Z-Pad (Zeebo & Windows).
 */

#include "zsdk/input/virtual_keyboard.hpp"
#include "zsdk/graphics/renderer.hpp"

namespace zsdk {
namespace input {

namespace {

static const uint8_t kRows = 4u;
static const uint8_t kCols = 10u;

static const char kLowerGrid[kRows][kCols] = {
    {'1','2','3','4','5','6','7','8','9','0'},
    {'q','w','e','r','t','y','u','i','o','p'},
    {'a','s','d','f','g','h','j','k','l','-'},
    {'^','z','x','c','v','b','n','m','<','>'} /* ^ = CAPS, < = DEL, > = OK */
};

static const char kUpperGrid[kRows][kCols] = {
    {'1','2','3','4','5','6','7','8','9','0'},
    {'Q','W','E','R','T','Y','U','I','O','P'},
    {'A','S','D','F','G','H','J','K','L','_'},
    {'^','Z','X','C','V','B','N','M','<','>'}
};

void CopyTrunc(char* dst, size_t maxDst, const char* src) {
    if (!dst || maxDst == 0u) {
        return;
    }
    size_t i = 0u;
    if (src) {
        while (src[i] != '\0' && i < maxDst - 1u) {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

} // namespace

VirtualKeyboard::VirtualKeyboard()
    : m_len(0u)
    , m_maxLen(32u)
    , m_row(1u)
    , m_col(0u)
    , m_uppercase(false)
    , m_active(false)
    , m_submitted(false)
    , m_cancelled(false) {
    m_title[0] = '\0';
    m_buffer[0] = '\0';
}

void VirtualKeyboard::Open(const char* title, const char* initialText, uint8_t maxLen) {
    CopyTrunc(m_title, sizeof(m_title), title ? title : "INPUT");
    m_maxLen = (maxLen > kVirtualKeyboardMaxChars) ? kVirtualKeyboardMaxChars : (maxLen == 0u ? 1u : maxLen);
    CopyTrunc(m_buffer, static_cast<size_t>(m_maxLen) + 1u, initialText ? initialText : "");

    m_len = 0u;
    while (m_buffer[m_len] != '\0') {
        ++m_len;
    }

    m_row         = 1u;
    m_col         = 0u;
    m_uppercase   = false;
    m_active      = true;
    m_submitted   = false;
    m_cancelled   = false;
}

void VirtualKeyboard::Close() {
    m_active = false;
}

void VirtualKeyboard::Update(const GamepadState& pad) {
    if (!m_active) {
        return;
    }

    if (pad.IsPressed(GamepadButton::DPadUp)) {
        m_row = (m_row == 0u) ? (kRows - 1u) : static_cast<uint8_t>(m_row - 1u);
    }
    if (pad.IsPressed(GamepadButton::DPadDown)) {
        m_row = static_cast<uint8_t>((m_row + 1u) % kRows);
    }
    if (pad.IsPressed(GamepadButton::DPadLeft)) {
        m_col = (m_col == 0u) ? (kCols - 1u) : static_cast<uint8_t>(m_col - 1u);
    }
    if (pad.IsPressed(GamepadButton::DPadRight)) {
        m_col = static_cast<uint8_t>((m_col + 1u) % kCols);
    }

    if (pad.IsPressed(GamepadButton::Action2)) {
        if (m_len > 0u) {
            --m_len;
            m_buffer[m_len] = '\0';
        }
    }

    if (pad.IsPressed(GamepadButton::Action3)) {
        if (m_len < m_maxLen) {
            m_buffer[m_len++] = ' ';
            m_buffer[m_len] = '\0';
        }
    }

    if (pad.IsPressed(GamepadButton::Start)) {
        m_submitted = true;
        m_active = false;
        return;
    }

    if (pad.IsPressed(GamepadButton::Back)) {
        m_cancelled = true;
        m_active = false;
        return;
    }

    if (pad.IsPressed(GamepadButton::Action1)) {
        const char ch = m_uppercase ? kUpperGrid[m_row][m_col] : kLowerGrid[m_row][m_col];
        if (ch == '^') {
            m_uppercase = !m_uppercase;
        } else if (ch == '<') {
            if (m_len > 0u) {
                --m_len;
                m_buffer[m_len] = '\0';
            }
        } else if (ch == '>') {
            m_submitted = true;
            m_active = false;
        } else if (m_len < m_maxLen) {
            m_buffer[m_len++] = ch;
            m_buffer[m_len] = '\0';
        }
    }
}

void VirtualKeyboard::Render(int32_t viewportWidth, int32_t viewportHeight) const {
    if (!m_active) {
        return;
    }

    using graphics::Renderer;
    Renderer::Begin2DVirtual(640.0f, 480.0f, viewportWidth, viewportHeight);

    /* Lower half of 640x480 screen (respecting 8px CRT safe area) */
    const float panelX = 24.0f;
    const float panelY = 236.0f;
    const float panelW = 592.0f;
    const float panelH = 228.0f;

    Renderer::DrawRect2D(panelX, panelY, panelW, panelH, Color::Rgba(12, 16, 26, 235));
    Renderer::DrawRect2D(panelX, panelY, panelW, 24.0f, Color::Rgba(35, 75, 130, 245));
    Renderer::DrawText2D(panelX + 10.0f, panelY + 8.0f, 1.0f, m_title, Color::Rgba(255, 255, 255, 255));

    /* Input text box */
    Renderer::DrawRect2D(panelX + 12.0f, panelY + 32.0f, panelW - 24.0f, 24.0f, Color::Rgba(22, 28, 42, 255));
    Renderer::DrawText2D(panelX + 18.0f, panelY + 40.0f, 1.0f, m_buffer, Color::Rgba(255, 240, 140, 255));

    const float gridX = panelX + 16.0f;
    const float gridY = panelY + 66.0f;
    const float cellW = 54.0f;
    const float cellH = 34.0f;

    for (uint8_t r = 0u; r < kRows; ++r) {
        for (uint8_t c = 0u; c < kCols; ++c) {
            const float kx = gridX + static_cast<float>(c) * 56.0f;
            const float ky = gridY + static_cast<float>(r) * 38.0f;
            const bool sel = (r == m_row && c == m_col);

            Renderer::DrawRect2D(kx, ky, cellW, cellH,
                                 sel ? Color::Rgba(60, 125, 210, 240) : Color::Rgba(28, 36, 52, 220));

            const char ch = m_uppercase ? kUpperGrid[r][c] : kLowerGrid[r][c];
            char label[4] = {ch, '\0', '\0', '\0'};
            if (ch == '^') { label[0] = 'C'; label[1] = 'A'; label[2] = 'P'; }
            else if (ch == '<') { label[0] = 'D'; label[1] = 'E'; label[2] = 'L'; }
            else if (ch == '>') { label[0] = 'O'; label[1] = 'K'; label[2] = '\0'; }

            Renderer::DrawText2D(kx + 12.0f, ky + 13.0f, 1.0f, label,
                                 sel ? Color::Rgba(255, 245, 130, 255) : Color::Rgba(225, 232, 240, 255));
        }
    }

    Renderer::End2D();
}

} // namespace input
} // namespace zsdk
