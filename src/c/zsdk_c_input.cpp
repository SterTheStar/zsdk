/**
 * @file zsdk_c_input.cpp
 * @brief C ABI bridge for zsdk::input::InputManager.
 */

#include "zsdk/c/zsdk_c_input.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/input/input.hpp"
#include "zsdk/input/keyboard_mouse.hpp"
#include "zsdk/input/virtual_keyboard.hpp"

struct zsdk_input_t {
    zsdk::input::InputManager mgr;
};

struct zsdk_kbm_t {
    zsdk::input::KeyboardMouseManager kbm;
};

struct zsdk_vkbd_t {
    zsdk::input::VirtualKeyboard vkbd;
};

namespace {

bool IsValidButton(zsdk_button_t btn) {
    return static_cast<uint8_t>(btn) < static_cast<uint8_t>(zsdk::input::GamepadButton::Count);
}

zsdk::input::GamepadButton ToCppButton(zsdk_button_t btn) {
    return static_cast<zsdk::input::GamepadButton>(static_cast<uint8_t>(btn));
}

} // namespace

extern "C" {

zsdk_input_t* zsdk_input_create(void) {
    return zsdk::c_abi::Alloc<zsdk_input_t>(zsdk::debug::MemorySubsystem::Input);
}

void zsdk_input_destroy(zsdk_input_t* input) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Input, input);
}

zsdk_result_t zsdk_input_init(zsdk_input_t* input, void* platform_ctx) {
    if (!input) {
        return ZSDK_C_ERR_INVALID_ARG;
    }
    return zsdk::c_abi::ToCResult(input->mgr.Initialize(platform_ctx));
}

void zsdk_input_shutdown(zsdk_input_t* input) {
    if (input) {
        input->mgr.Shutdown();
    }
}

void zsdk_input_update(zsdk_input_t* input, uint32_t delta_ms) {
    if (input) {
        input->mgr.Update(delta_ms);
    }
}

uint8_t zsdk_input_connected_count(const zsdk_input_t* input) {
    return input ? input->mgr.GetConnectedCount() : 0u;
}

int zsdk_input_is_connected(const zsdk_input_t* input, uint8_t player) {
    if (!input) {
        return 0;
    }
    return input->mgr.GetGamepad(player).IsConnected() ? 1 : 0;
}

int zsdk_input_is_down(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn) {
    if (!input || !IsValidButton(btn)) {
        return 0;
    }
    return input->mgr.GetGamepad(player).IsDown(ToCppButton(btn)) ? 1 : 0;
}

int zsdk_input_is_pressed(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn) {
    if (!input || !IsValidButton(btn)) {
        return 0;
    }
    return input->mgr.GetGamepad(player).IsPressed(ToCppButton(btn)) ? 1 : 0;
}

int zsdk_input_is_released(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn) {
    if (!input || !IsValidButton(btn)) {
        return 0;
    }
    return input->mgr.GetGamepad(player).IsReleased(ToCppButton(btn)) ? 1 : 0;
}

void zsdk_input_left_stick(const zsdk_input_t* input, uint8_t player, float* out_x, float* out_y) {
    zsdk::Vec2 v = input ? input->mgr.GetGamepad(player).GetLeftStick() : zsdk::Vec2{0.0f, 0.0f};
    if (out_x) *out_x = v.x;
    if (out_y) *out_y = v.y;
}

void zsdk_input_right_stick(const zsdk_input_t* input, uint8_t player, float* out_x, float* out_y) {
    zsdk::Vec2 v = input ? input->mgr.GetGamepad(player).GetRightStick() : zsdk::Vec2{0.0f, 0.0f};
    if (out_x) *out_x = v.x;
    if (out_y) *out_y = v.y;
}

zsdk_result_t zsdk_input_set_rumble(zsdk_input_t* input, uint8_t player, uint16_t left_motor, uint16_t right_motor) {
    if (!input) {
        return ZSDK_C_ERR_INVALID_ARG;
    }
    return zsdk::c_abi::ToCResult(input->mgr.SetRumble(player, left_motor, right_motor));
}

zsdk_kbm_t* zsdk_kbm_create(void) {
    return zsdk::c_abi::Alloc<zsdk_kbm_t>(zsdk::debug::MemorySubsystem::Input);
}

void zsdk_kbm_destroy(zsdk_kbm_t* kbm) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Input, kbm);
}

zsdk_result_t zsdk_kbm_init(zsdk_kbm_t* kbm, void* platform_ctx) {
    if (!kbm) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(kbm->kbm.Initialize(platform_ctx));
}

void zsdk_kbm_shutdown(zsdk_kbm_t* kbm) {
    if (kbm) kbm->kbm.Shutdown();
}

void zsdk_kbm_update(zsdk_kbm_t* kbm) {
    if (kbm) kbm->kbm.Update();
}

int zsdk_kbm_is_key_down(const zsdk_kbm_t* kbm, uint16_t hid_key) {
    return (kbm && kbm->kbm.IsKeyDown(hid_key)) ? 1 : 0;
}

void zsdk_kbm_mouse_delta(const zsdk_kbm_t* kbm, float* out_dx, float* out_dy) {
    zsdk::Vec2 d = kbm ? kbm->kbm.GetMouseDelta() : zsdk::Vec2{0.0f, 0.0f};
    if (out_dx) *out_dx = d.x;
    if (out_dy) *out_dy = d.y;
}

uint8_t zsdk_kbm_mouse_buttons(const zsdk_kbm_t* kbm) {
    return kbm ? kbm->kbm.GetMouseButtonsMask() : 0u;
}

zsdk_vkbd_t* zsdk_vkbd_create(void) {
    return zsdk::c_abi::Alloc<zsdk_vkbd_t>(zsdk::debug::MemorySubsystem::Input);
}

void zsdk_vkbd_destroy(zsdk_vkbd_t* vkbd) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Input, vkbd);
}

void zsdk_vkbd_open(zsdk_vkbd_t* vkbd, const char* title, const char* initial_text, uint8_t max_len) {
    if (vkbd) vkbd->vkbd.Open(title, initial_text ? initial_text : "", max_len);
}

void zsdk_vkbd_close(zsdk_vkbd_t* vkbd) {
    if (vkbd) vkbd->vkbd.Close();
}

void zsdk_vkbd_update(zsdk_vkbd_t* vkbd, const zsdk_input_t* input, uint8_t player) {
    if (vkbd && input) vkbd->vkbd.Update(input->mgr.GetGamepad(player));
}

void zsdk_vkbd_render(const zsdk_vkbd_t* vkbd, int32_t viewport_w, int32_t viewport_h) {
    if (vkbd) vkbd->vkbd.Render(viewport_w, viewport_h);
}

int zsdk_vkbd_is_active(const zsdk_vkbd_t* vkbd) {
    return (vkbd && vkbd->vkbd.IsActive()) ? 1 : 0;
}

int zsdk_vkbd_is_submitted(const zsdk_vkbd_t* vkbd) {
    return (vkbd && vkbd->vkbd.IsSubmitted()) ? 1 : 0;
}

int zsdk_vkbd_is_cancelled(const zsdk_vkbd_t* vkbd) {
    return (vkbd && vkbd->vkbd.IsCancelled()) ? 1 : 0;
}

const char* zsdk_vkbd_get_text(const zsdk_vkbd_t* vkbd) {
    return vkbd ? vkbd->vkbd.GetText() : "";
}

int zsdk_kbm_has_keyboard(const zsdk_kbm_t* kbm) {
    return (kbm && kbm->kbm.HasKeyboard()) ? 1 : 0;
}

int zsdk_kbm_has_mouse(const zsdk_kbm_t* kbm) {
    return (kbm && kbm->kbm.HasMouse()) ? 1 : 0;
}

uint32_t zsdk_input_hold_duration_ms(const zsdk_input_t* input, uint8_t player, zsdk_button_t btn) {
    if (!input || btn >= ZSDK_BTN_COUNT) return 0u;
    return input->mgr.GetGamepad(player).GetHoldDurationMs(static_cast<zsdk::input::GamepadButton>(btn));
}

int zsdk_input_is_combo_triggered(const zsdk_input_t* input, uint8_t player, uint32_t combo_mask) {
    return (input && input->mgr.GetGamepad(player).IsComboJustTriggered(combo_mask)) ? 1 : 0;
}

int zsdk_input_motion(const zsdk_input_t* input, uint8_t player,
                      float* out_ax, float* out_ay, float* out_az,
                      float* out_gx, float* out_gy, float* out_gz) {
    if (!input) return 0;
    const zsdk::input::MotionState& m = input->mgr.GetGamepad(player).GetMotion();
    if (out_ax) *out_ax = m.acceleration.x;
    if (out_ay) *out_ay = m.acceleration.y;
    if (out_az) *out_az = m.acceleration.z;
    if (out_gx) *out_gx = m.angularVelocity.x;
    if (out_gy) *out_gy = m.angularVelocity.y;
    if (out_gz) *out_gz = m.angularVelocity.z;
    return m.supported ? 1 : 0;
}

} /* extern "C" */
