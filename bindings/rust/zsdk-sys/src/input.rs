use core::ffi::{c_char, c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)] pub struct zsdk_input_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_kbm_t   { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_vkbd_t  { _private: [u8; 0] }

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkButton {
    DPadUp        = 0,
    DPadDown      = 1,
    DPadLeft      = 2,
    DPadRight     = 3,
    Action1       = 4,
    Action2       = 5,
    Action3       = 6,
    Action4       = 7,
    ShoulderLeft  = 8,
    ShoulderRight = 9,
    TriggerLeft   = 10,
    TriggerRight  = 11,
    Start         = 12,
    Back          = 13,
    Home          = 14,
    LeftThumb     = 15,
    RightThumb    = 16,
    Count         = 17,
}
pub type zsdk_button_t = ZsdkButton;

extern "C" {
    pub fn zsdk_input_create() -> *mut zsdk_input_t;
    pub fn zsdk_input_destroy(input: *mut zsdk_input_t);
    pub fn zsdk_input_init(input: *mut zsdk_input_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_input_shutdown(input: *mut zsdk_input_t);
    pub fn zsdk_input_update(input: *mut zsdk_input_t, delta_ms: u32);
    pub fn zsdk_input_connected_count(input: *const zsdk_input_t) -> u8;
    pub fn zsdk_input_is_connected(input: *const zsdk_input_t, player: u8) -> c_int;
    pub fn zsdk_input_is_down(input: *const zsdk_input_t, player: u8, btn: zsdk_button_t) -> c_int;
    pub fn zsdk_input_is_pressed(input: *const zsdk_input_t, player: u8, btn: zsdk_button_t) -> c_int;
    pub fn zsdk_input_is_released(input: *const zsdk_input_t, player: u8, btn: zsdk_button_t) -> c_int;
    pub fn zsdk_input_left_stick(input: *const zsdk_input_t, player: u8, out_x: *mut f32, out_y: *mut f32);
    pub fn zsdk_input_right_stick(input: *const zsdk_input_t, player: u8, out_x: *mut f32, out_y: *mut f32);
    pub fn zsdk_input_set_rumble(input: *mut zsdk_input_t, player: u8, left_motor: u16, right_motor: u16) -> zsdk_result_t;

    pub fn zsdk_kbm_create() -> *mut zsdk_kbm_t;
    pub fn zsdk_kbm_destroy(kbm: *mut zsdk_kbm_t);
    pub fn zsdk_kbm_init(kbm: *mut zsdk_kbm_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_kbm_shutdown(kbm: *mut zsdk_kbm_t);
    pub fn zsdk_kbm_update(kbm: *mut zsdk_kbm_t);
    pub fn zsdk_kbm_is_key_down(kbm: *const zsdk_kbm_t, hid_key: u16) -> c_int;
    pub fn zsdk_kbm_mouse_delta(kbm: *const zsdk_kbm_t, out_dx: *mut f32, out_dy: *mut f32);
    pub fn zsdk_kbm_mouse_buttons(kbm: *const zsdk_kbm_t) -> u8;

    pub fn zsdk_vkbd_create() -> *mut zsdk_vkbd_t;
    pub fn zsdk_vkbd_destroy(vkbd: *mut zsdk_vkbd_t);
    pub fn zsdk_vkbd_open(vkbd: *mut zsdk_vkbd_t, title: *const c_char, initial_text: *const c_char, max_len: u8);
    pub fn zsdk_vkbd_close(vkbd: *mut zsdk_vkbd_t);
    pub fn zsdk_vkbd_update(vkbd: *mut zsdk_vkbd_t, input: *const zsdk_input_t, player: u8);
    pub fn zsdk_vkbd_render(vkbd: *const zsdk_vkbd_t, viewport_w: i32, viewport_h: i32);
    pub fn zsdk_vkbd_is_active(vkbd: *const zsdk_vkbd_t) -> c_int;
    pub fn zsdk_vkbd_is_submitted(vkbd: *const zsdk_vkbd_t) -> c_int;
    pub fn zsdk_vkbd_is_cancelled(vkbd: *const zsdk_vkbd_t) -> c_int;
    pub fn zsdk_vkbd_get_text(vkbd: *const zsdk_vkbd_t) -> *const c_char;
}
