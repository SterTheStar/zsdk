/**
 * @file zeebo_input.cpp
 * @brief Qualcomm BREW / Zeebo IHID + AVK InputManager bridge.
 *
 * Wraps IHID, IHIDDevice, AEEHIDButtons.h, AEEHIDThumbsticks.h,
 * AEEHIDDevice_Joystick.h, and AEEVCodes.h with zero per-frame allocations.
 */

#include "zsdk/input/input.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEEvent.h"
#include "AEEVCodes.h"
#include "AEEHID.bid"
#include "AEEIHID.h"
#include "AEEIHIDDevice.h"
#include "AEEHIDButtons.h"
#include "AEEHIDThumbsticks.h"
#include "AEEHIDDevice_Joystick.h"

namespace zsdk {
namespace input {

namespace {

struct DeviceSlot {
    int                       devHandle;
    IHIDDevice*               device;
    AEEHIDThumbstickMapping*  thumbMapping;
    bool                      hasThumbsticks;
};

bool MapHidButtonToLogical(const AEEHIDButtonInfo& info, GamepadButton& outBtn) {
    /* Primary Zeebo button IDs from AEEHIDButtons.h */
    switch (info.nButtonID) {
        case ZEEBO_BUTTON_DPAD_UP:        outBtn = GamepadButton::DPadUp;        return true;
        case ZEEBO_BUTTON_DPAD_DOWN:      outBtn = GamepadButton::DPadDown;      return true;
        case ZEEBO_BUTTON_DPAD_LEFT:      outBtn = GamepadButton::DPadLeft;      return true;
        case ZEEBO_BUTTON_DPAD_RIGHT:     outBtn = GamepadButton::DPadRight;     return true;
        case ZEEBO_BUTTON_SOUTH:          outBtn = GamepadButton::Action1;       return true;
        case ZEEBO_BUTTON_WEST:           outBtn = GamepadButton::Action2;       return true;
        case ZEEBO_BUTTON_NORTH:          outBtn = GamepadButton::Action3;       return true;
        case ZEEBO_BUTTON_EAST:           outBtn = GamepadButton::Action4;       return true;
        case ZEEBO_BUTTON_SHOULDER_LEFT:  outBtn = GamepadButton::ShoulderLeft;  return true;
        case ZEEBO_BUTTON_SHOULDER_RIGHT: outBtn = GamepadButton::ShoulderRight; return true;
        case ZEEBO_BUTTON_HOME:           outBtn = GamepadButton::Home;          return true;
        case AEEHIDBUTTON_7:              outBtn = GamepadButton::TriggerLeft;   return true;
        case AEEHIDBUTTON_8:              outBtn = GamepadButton::TriggerRight;  return true;
        case AEEHIDBUTTON_10:             outBtn = GamepadButton::Start;         return true;
        case AEEHIDBUTTON_11:             outBtn = GamepadButton::LeftThumb;     return true;
        case AEEHIDBUTTON_12:             outBtn = GamepadButton::RightThumb;    return true;
        default: break;
    }

    /* Fallback by HID Joystick UID (AEEHIDDevice_Joystick.h) */
    switch (info.nButtonUID) {
        case AEEUID_HIDJoystick_Start:                outBtn = GamepadButton::Start;         return true;
        case AEEUID_HIDJoystick_Back:                 outBtn = GamepadButton::Back;          return true;
        case AEEUID_HIDJoystick_Left_Thumbstick:      outBtn = GamepadButton::LeftThumb;     return true;
        case AEEUID_HIDJoystick_Right_Thumbstick:     outBtn = GamepadButton::RightThumb;    return true;
        case AEEUID_HIDJoystick_Left_Shoulder_Lower:  outBtn = GamepadButton::TriggerLeft;   return true;
        case AEEUID_HIDJoystick_Right_Shoulder_Lower: outBtn = GamepadButton::TriggerRight;  return true;
        default: return false;
    }
}

bool MapAvkToLogical(uint16_t keyCode, GamepadButton& outBtn) {
    switch (keyCode) {
        case AVK_UP:     outBtn = GamepadButton::DPadUp;    return true;
        case AVK_DOWN:   outBtn = GamepadButton::DPadDown;  return true;
        case AVK_LEFT:   outBtn = GamepadButton::DPadLeft;  return true;
        case AVK_RIGHT:  outBtn = GamepadButton::DPadRight; return true;
        case AVK_SELECT: outBtn = GamepadButton::Start;     return true;
        case AVK_CLR:    outBtn = GamepadButton::Back;      return true;
        case AVK_SOFT1:  outBtn = GamepadButton::Action1;   return true;
        case AVK_SOFT2:  outBtn = GamepadButton::Action2;   return true;
#ifdef AVK_GP_1
        case AVK_GP_1:   outBtn = GamepadButton::Action1;       return true;
        case AVK_GP_2:   outBtn = GamepadButton::Action2;       return true;
        case AVK_GP_3:   outBtn = GamepadButton::Action3;       return true;
        case AVK_GP_4:   outBtn = GamepadButton::Action4;       return true;
        case AVK_GP_5:   outBtn = GamepadButton::ShoulderLeft;  return true;
        case AVK_GP_6:   outBtn = GamepadButton::ShoulderRight; return true;
        case AVK_GP_SL:  outBtn = GamepadButton::TriggerLeft;   return true;
        case AVK_GP_SR:  outBtn = GamepadButton::TriggerRight;  return true;
#endif
        default:         return false;
    }
}

} // namespace

struct InputManager::Impl {
    IShell*    shell;
    IHID*      hid;
    DeviceSlot slots[ZSDK_MAX_GAMEPADS];
};

/**
 * @brief Internal friend bridge for accessing GamepadState private members.
 */
class ZeeboInputBridge {
public:
    static void AttachDeviceSlot(InputManager::Impl* impl, GamepadState* pads, int devHandle) {
        if (!impl || !impl->hid) {
            return;
        }

        /* Avoid duplicate attachment */
        for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
            if (impl->slots[i].device && impl->slots[i].devHandle == devHandle) {
                return;
            }
        }

        for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
            if (!impl->slots[i].device) {
                IHIDDevice* dev = nullptr;
                if (IHID_CreateDevice(impl->hid, devHandle, &dev) == AEE_SUCCESS && dev) {
                    /* Zeebo Developer Guide 6.3.6: suppress duplicate default AVK events */
                    IHIDDevice_SetExclusiveLevel(dev, 1);
                    impl->slots[i].devHandle = devHandle;
                    impl->slots[i].device = dev;
                    impl->slots[i].thumbMapping = nullptr;
                    impl->slots[i].hasThumbsticks =
                        (AEEHIDThumbstick_InitializeMapping(dev, &impl->slots[i].thumbMapping) == AEE_SUCCESS);
                    pads[i] = GamepadState();
                    pads[i].m_connected = true;
                    ZSDK_LOG_INFO("INP", "HID Gamepad P%u connected (handle=%d)", i + 1u, devHandle);
                }
                return;
            }
        }
    }

    static void DetachDeviceSlot(InputManager::Impl* impl, GamepadState* pads, int devHandle) {
        if (!impl) {
            return;
        }
        for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
            if (impl->slots[i].device && impl->slots[i].devHandle == devHandle) {
                if (impl->slots[i].hasThumbsticks && impl->slots[i].thumbMapping) {
                    AEEHIDThumbstick_DestroyMapping(impl->slots[i].device, impl->slots[i].thumbMapping);
                }
                IHIDDevice_Release(impl->slots[i].device);
                impl->slots[i].device = nullptr;
                impl->slots[i].thumbMapping = nullptr;
                impl->slots[i].hasThumbsticks = false;
                impl->slots[i].devHandle = 0;
                pads[i].m_connected = false;
                ZSDK_LOG_INFO("INP", "HID Gamepad P%u disconnected", i + 1u);
                return;
            }
        }
    }
};

InputManager::InputManager()
    : m_impl(nullptr) {
}

InputManager::~InputManager() {
    Shutdown();
}

Result InputManager::Initialize(void* platformContext) {
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
    m_impl->shell = shell;

    /* Create IHID instance (optional on simulator if extension is absent, falls back to AVK) */
    if (ISHELL_CreateInstance(shell, AEECLSID_HID, reinterpret_cast<void**>(&m_impl->hid)) != AEE_SUCCESS) {
        ZSDK_LOG_WARN("INP", "IHID unavailable; using AVK key fallback");
        m_impl->hid = nullptr;
        m_gamepads[0].m_connected = true;
        return Result::Ok;
    }

    int handles[ZSDK_MAX_GAMEPADS] = {};
    int reqLen = 0;
    if (IHID_GetConnectedDevices(m_impl->hid, AEEUID_HID_Joystick_Device,
                                 handles, ZSDK_MAX_GAMEPADS, &reqLen) == AEE_SUCCESS) {
        const int count = (reqLen < static_cast<int>(ZSDK_MAX_GAMEPADS))
                              ? reqLen
                              : static_cast<int>(ZSDK_MAX_GAMEPADS);
        for (int i = 0; i < count; ++i) {
            ZeeboInputBridge::AttachDeviceSlot(m_impl, m_gamepads, handles[i]);
        }
    }

    return Result::Ok;
}

void InputManager::Shutdown() {
    if (!m_impl) {
        return;
    }

    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        if (m_impl->slots[i].device) {
            if (m_impl->slots[i].hasThumbsticks && m_impl->slots[i].thumbMapping) {
                AEEHIDThumbstick_DestroyMapping(m_impl->slots[i].device, m_impl->slots[i].thumbMapping);
            }
            IHIDDevice_Release(m_impl->slots[i].device);
            m_impl->slots[i].device = nullptr;
        }
        m_gamepads[i].m_connected = false;
    }

    if (m_impl->hid) {
        IHID_Release(m_impl->hid);
        m_impl->hid = nullptr;
    }

    debug::MemoryTracker::Free(debug::MemorySubsystem::Input, m_impl);
    m_impl = nullptr;
}

void InputManager::Update(uint32_t deltaTimeMs) {
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        m_gamepads[i].BeginFrame(deltaTimeMs);
    }

    if (!m_impl || !m_impl->hid) {
        return;
    }

    /* 1. Drain hotplug connect/disconnect events */
    int devHandle = 0;
    int status = 0;
    boolean dropped = FALSE;
    boolean connectDropped = FALSE;
    while (IHID_GetNextConnectEvent(m_impl->hid, &devHandle, &status, &dropped) == AEE_SUCCESS) {
        if (dropped) {
            connectDropped = TRUE;
            break;
        }
        AEEHIDDeviceInfo info;
        MEMSET(&info, 0, sizeof(info));
        if (status != 0 && IHID_GetDeviceInfo(m_impl->hid, devHandle, &info) == AEE_SUCCESS) {
            if (info.nDeviceType == AEEUID_HID_Joystick_Device) {
                ZeeboInputBridge::AttachDeviceSlot(m_impl, m_gamepads, devHandle);
            }
        } else {
            ZeeboInputBridge::DetachDeviceSlot(m_impl, m_gamepads, devHandle);
        }
    }

    if (connectDropped) {
        /* Zeebo Developer Guide 6.2.2: flush queue and re-enumerate connected devices */
        while (IHID_GetNextConnectEvent(m_impl->hid, nullptr, nullptr, nullptr) == AEE_SUCCESS) {}
        int handles[ZSDK_MAX_GAMEPADS] = {};
        int reqLen = 0;
        if (IHID_GetConnectedDevices(m_impl->hid, AEEUID_HID_Joystick_Device,
                                     handles, ZSDK_MAX_GAMEPADS, &reqLen) == AEE_SUCCESS) {
            const int count = (reqLen < static_cast<int>(ZSDK_MAX_GAMEPADS))
                                  ? reqLen
                                  : static_cast<int>(ZSDK_MAX_GAMEPADS);
            for (int i = 0; i < count; ++i) {
                ZeeboInputBridge::AttachDeviceSlot(m_impl, m_gamepads, handles[i]);
            }
        }
    }

    /* 2. Poll each connected controller slot */
    for (uint8_t i = 0u; i < ZSDK_MAX_GAMEPADS; ++i) {
        DeviceSlot& slot = m_impl->slots[i];
        GamepadState& pad = m_gamepads[i];
        pad.m_connected = (slot.device != nullptr);
        if (!slot.device) {
            continue;
        }

        /* Drain button event queue */
        AEEHIDButtonInfo btnInfo;
        uint32 timestamp = 0;
        while (AEEHIDButton_GetNextButtonEvent(slot.device, &btnInfo, &timestamp, &dropped) == AEE_SUCCESS) {
            GamepadButton logicalBtn;
            if (MapHidButtonToLogical(btnInfo, logicalBtn)) {
                pad.SetButtonState(logicalBtn, btnInfo.nState != 0);
            }
        }

        /* Read analog thumbsticks */
        if (slot.hasThumbsticks && slot.thumbMapping) {
            AEEHIDThumbstickValues sticks;
            if (AEEHIDThumbstick_GetPositionState(slot.device, slot.thumbMapping, &sticks) == AEE_SUCCESS) {
                pad.SetRawSticks(sticks.sLeftX, sticks.sLeftY, sticks.sRightX, sticks.sRightY);
            }
        }

        /* Read full HID position / accelerometer / gyro state */
        AEEHIDPositionInfo posInfo;
        if (IHIDDevice_GetPositionState(slot.device, &posInfo) == AEE_SUCCESS) {
            pad.m_motion.supported = true;
            pad.m_motion.acceleration    = Vec3{static_cast<float>(posInfo.nAX),
                                                static_cast<float>(posInfo.nAY),
                                                static_cast<float>(posInfo.nAZ)};
            pad.m_motion.angularVelocity = Vec3{static_cast<float>(posInfo.nVRx),
                                                static_cast<float>(posInfo.nVRy),
                                                static_cast<float>(posInfo.nVRz)};
            pad.m_motion.rotation        = Vec3{static_cast<float>(posInfo.nRx),
                                                static_cast<float>(posInfo.nRy),
                                                static_cast<float>(posInfo.nRz)};
        }
    }
}

bool InputManager::HandlePlatformEvent(uint32_t eventCode, uint16_t keyCode, uint32_t param) {
    ZSDK_UNUSED(param);
    GamepadButton btn;
    if (!MapAvkToLogical(keyCode, btn)) {
        return false;
    }

    if (eventCode == EVT_KEY_PRESS || eventCode == EVT_KEY) {
        m_gamepads[0].m_connected = true;
        m_gamepads[0].SetButtonState(btn, true);
        return true;
    } else if (eventCode == EVT_KEY_RELEASE) {
        m_gamepads[0].SetButtonState(btn, false);
        return true;
    }
    return false;
}

Result InputManager::SetRumble(uint8_t playerIndex, uint16_t leftMotor, uint16_t rightMotor) {
    if (!m_impl || playerIndex >= ZSDK_MAX_GAMEPADS) {
        return Result::InvalidArg;
    }
    IHIDDevice* dev = m_impl->slots[playerIndex].device;
    if (!dev) {
        return Result::DeviceDisconnected;
    }
    const AEEResult res = IHIDDevice_Rumble(dev, static_cast<int>(leftMotor), static_cast<int>(rightMotor));
    return (res == AEE_SUCCESS) ? Result::Ok : Result::NotSupported;
}

} // namespace input
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
