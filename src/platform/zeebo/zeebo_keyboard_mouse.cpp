/**
 * @file zeebo_keyboard_mouse.cpp
 * @brief Qualcomm BREW / Zeebo HID Keyboard & Mouse bridge.
 */

#include "zsdk/input/keyboard_mouse.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEHID.bid"
#include "AEEIHID.h"
#include "AEEIHIDDevice.h"
#include "AEEHIDDevice_Keyboard.h"
#include "AEEHIDDevice_Mouse.h"

namespace zsdk {
namespace input {

struct KeyboardMouseManager::Impl {
    IHID*       hid;
    IHIDDevice* keyboard;
    IHIDDevice* mouse;
    uint8_t     keyDown[32]; /* 256-bit key state bitset */
    Vec2        mouseDelta;
    uint8_t     mouseButtons;
};

KeyboardMouseManager::KeyboardMouseManager()
    : m_impl(nullptr) {
}

KeyboardMouseManager::~KeyboardMouseManager() {
    Shutdown();
}

Result KeyboardMouseManager::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Input, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    MEMSET(m_impl, 0, sizeof(Impl));

    if (ISHELL_CreateInstance(shell, AEECLSID_HID,
                              reinterpret_cast<void**>(&m_impl->hid)) != AEE_SUCCESS || !m_impl->hid) {
        return Result::Ok; /* Optional */
    }

    int handle = 0;
    int req = 0;
    if (IHID_GetConnectedDevices(m_impl->hid, AEEUID_HID_Keyboard_Device, &handle, 1, &req) == AEE_SUCCESS && req > 0) {
        IHID_CreateDevice(m_impl->hid, handle, &m_impl->keyboard);
    }
    req = 0;
    if (IHID_GetConnectedDevices(m_impl->hid, AEEUID_HID_Mouse_Device, &handle, 1, &req) == AEE_SUCCESS && req > 0) {
        IHID_CreateDevice(m_impl->hid, handle, &m_impl->mouse);
    }

    return Result::Ok;
}

void KeyboardMouseManager::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->keyboard) {
        IHIDDevice_Release(m_impl->keyboard);
    }
    if (m_impl->mouse) {
        IHIDDevice_Release(m_impl->mouse);
    }
    if (m_impl->hid) {
        IHID_Release(m_impl->hid);
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Input, m_impl);
    m_impl = nullptr;
}

void KeyboardMouseManager::Update() {
    if (!m_impl) {
        return;
    }
    m_impl->mouseDelta = Vec2{0.0f, 0.0f};

    AEEHIDButtonInfo btn;
    uint32 ts = 0;
    boolean dropped = FALSE;

    if (m_impl->keyboard) {
        while (IHIDDevice_GetNextButtonEvent(m_impl->keyboard, &btn, &ts, &dropped) == AEE_SUCCESS) {
            const uint8_t code = static_cast<uint8_t>(btn.nButtonID & 0xFF);
            if (btn.nState != 0) {
                m_impl->keyDown[code >> 3] |= static_cast<uint8_t>(1u << (code & 7u));
            } else {
                m_impl->keyDown[code >> 3] &= static_cast<uint8_t>(~(1u << (code & 7u)));
            }
        }
    }

    if (m_impl->mouse) {
        while (IHIDDevice_GetNextButtonEvent(m_impl->mouse, &btn, &ts, &dropped) == AEE_SUCCESS) {
            const uint8_t mask = static_cast<uint8_t>(1u << (btn.nButtonID & 7));
            if (btn.nState != 0) {
                m_impl->mouseButtons |= mask;
            } else {
                m_impl->mouseButtons &= static_cast<uint8_t>(~mask);
            }
        }
        AEEHIDPositionInfo pos;
        if (IHIDDevice_GetPositionState(m_impl->mouse, &pos) == AEE_SUCCESS) {
            m_impl->mouseDelta = Vec2{static_cast<float>(pos.nX), static_cast<float>(pos.nY)};
        }
    }
}

bool KeyboardMouseManager::IsKeyDown(uint16_t hidKeyCode) const {
    if (!m_impl || hidKeyCode >= 256u) {
        return false;
    }
    return (m_impl->keyDown[hidKeyCode >> 3] & (1u << (hidKeyCode & 7u))) != 0u;
}

Vec2 KeyboardMouseManager::GetMouseDelta() const {
    return m_impl ? m_impl->mouseDelta : Vec2{0.0f, 0.0f};
}

uint8_t KeyboardMouseManager::GetMouseButtonsMask() const {
    return m_impl ? m_impl->mouseButtons : 0u;
}

bool KeyboardMouseManager::HasKeyboard() const {
    return m_impl && (m_impl->keyboard != nullptr);
}

bool KeyboardMouseManager::HasMouse() const {
    return m_impl && (m_impl->mouse != nullptr);
}

} // namespace input
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
