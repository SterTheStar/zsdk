/**
 * @file input_types.hpp
 * @brief Hardware-agnostic controller, analog stick, and motion sensor types.
 *
 * Maps directly to Zeebo's Z-Pad & Boomerang (AEEHIDButtons.h, AEEHIDThumbsticks.h,
 * AEEHIDDevice_Joystick.h, AEEIHIDDevice.h) and BREW key events (AEEVCodes.h).
 */

#ifndef ZSDK_INPUT_INPUT_TYPES_HPP
#define ZSDK_INPUT_INPUT_TYPES_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace input {

/**
 * @brief Logical gamepad buttons mapped to Zeebo Z-Pad & standard HID Joystick UIDs.
 */
enum class GamepadButton : uint8_t {
    DPadUp = 0,         /**< ZEEBO_BUTTON_DPAD_UP */
    DPadDown,           /**< ZEEBO_BUTTON_DPAD_DOWN */
    DPadLeft,           /**< ZEEBO_BUTTON_DPAD_LEFT */
    DPadRight,          /**< ZEEBO_BUTTON_DPAD_RIGHT */

    Action1,            /**< Zeebo Button 1 / South (ZEEBO_BUTTON_SOUTH) */
    Action2,            /**< Zeebo Button 2 / West  (ZEEBO_BUTTON_WEST) */
    Action3,            /**< Zeebo Button 3 / North (ZEEBO_BUTTON_NORTH) */
    Action4,            /**< Zeebo Button 4 / East  (ZEEBO_BUTTON_EAST) */

    ShoulderLeft,       /**< Zeebo ZL (ZEEBO_BUTTON_SHOULDER_LEFT) */
    ShoulderRight,      /**< Zeebo ZR (ZEEBO_BUTTON_SHOULDER_RIGHT) */
    TriggerLeft,        /**< HID Left Shoulder Lower */
    TriggerRight,       /**< HID Right Shoulder Lower */

    Start,              /**< AEEUID_HIDJoystick_Start / AVK_SELECT */
    Back,               /**< AEEUID_HIDJoystick_Back / AVK_CLR */
    Home,               /**< ZEEBO_BUTTON_HOME */

    LeftThumb,          /**< AEEUID_HIDJoystick_Left_Thumbstick */
    RightThumb,         /**< AEEUID_HIDJoystick_Right_Thumbstick */

    Count
};

/** @brief Bitmask helper for button combos (e.g. START + DPadDown). */
ZSDK_FORCE_INLINE uint32_t ButtonMask(GamepadButton button) {
    return 1u << static_cast<uint8_t>(button);
}

/**
 * @brief Analog stick configuration (deadzone & saturation).
 */
struct AnalogConfig {
    float innerDeadzone;    /**< Normalized [0.0..1.0], default 0.15f. */
    float outerDeadzone;    /**< Normalized [0.0..1.0], default 0.95f. */
};

/**
 * @brief Motion sensor state from IHIDDevice (AEEHIDPositionInfo).
 *
 * Available on motion-capable controllers (e.g., Zeebo Boomerang).
 */
struct MotionState {
    bool supported;
    Vec3 acceleration;      /**< Linear acceleration (from nAX, nAY, nAZ). */
    Vec3 angularVelocity;   /**< Angular velocity / gyro (from nVRx, nVRy, nVRz). */
    Vec3 rotation;          /**< Orientation/rotation (from nRx, nRy, nRz). */
};

} // namespace input
} // namespace zsdk

#endif /* ZSDK_INPUT_INPUT_TYPES_HPP */
