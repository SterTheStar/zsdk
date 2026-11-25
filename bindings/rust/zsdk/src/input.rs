use ::core::ffi::{CStr, c_void};
use zsdk_sys::*;
use crate::{check, Error, Result};

pub use zsdk_sys::ZsdkButton as Button;

pub struct InputManager {
    raw: *mut zsdk_input_t,
}

impl InputManager {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_input_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_input_init(raw, platform_ctx) }) {
                unsafe { zsdk_input_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }

    #[inline] pub fn update(&mut self, delta_ms: u32) { unsafe { zsdk_input_update(self.raw, delta_ms); } }
    #[inline] pub fn connected_count(&self) -> u8 { unsafe { zsdk_input_connected_count(self.raw) } }
    #[inline] pub fn is_connected(&self, player: u8) -> bool { unsafe { zsdk_input_is_connected(self.raw, player) != 0 } }
    #[inline] pub fn is_down(&self, player: u8, btn: Button) -> bool { unsafe { zsdk_input_is_down(self.raw, player, btn) != 0 } }
    #[inline] pub fn is_pressed(&self, player: u8, btn: Button) -> bool { unsafe { zsdk_input_is_pressed(self.raw, player, btn) != 0 } }
    #[inline] pub fn is_released(&self, player: u8, btn: Button) -> bool { unsafe { zsdk_input_is_released(self.raw, player, btn) != 0 } }

    #[inline]
    pub fn left_stick(&self, player: u8) -> (f32, f32) {
        let (mut x, mut y) = (0.0f32, 0.0f32);
        unsafe { zsdk_input_left_stick(self.raw, player, &mut x, &mut y); }
        (x, y)
    }

    #[inline]
    pub fn right_stick(&self, player: u8) -> (f32, f32) {
        let (mut x, mut y) = (0.0f32, 0.0f32);
        unsafe { zsdk_input_right_stick(self.raw, player, &mut x, &mut y); }
        (x, y)
    }

    #[inline]
    pub fn set_rumble(&mut self, player: u8, left_motor: u16, right_motor: u16) -> Result<()> {
        check(unsafe { zsdk_input_set_rumble(self.raw, player, left_motor, right_motor) })
    }

    #[inline] pub fn hold_duration_ms(&self, player: u8, btn: Button) -> u32 { unsafe { zsdk_input_hold_duration_ms(self.raw, player, btn) } }
    #[inline] pub fn is_combo_triggered(&self, player: u8, combo_mask: u32) -> bool { unsafe { zsdk_input_is_combo_triggered(self.raw, player, combo_mask) != 0 } }
    #[inline]
    pub fn motion(&self, player: u8) -> Option<([f32; 3], [f32; 3])> {
        let (mut ax, mut ay, mut az, mut gx, mut gy, mut gz) = (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        let sup = unsafe { zsdk_input_motion(self.raw, player, &mut ax, &mut ay, &mut az, &mut gx, &mut gy, &mut gz) };
        if sup != 0 { Some(([ax, ay, az], [gx, gy, gz])) } else { None }
    }

    #[inline] pub fn as_raw(&self) -> *const zsdk_input_t { self.raw }
}

impl Drop for InputManager {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe { zsdk_input_shutdown(self.raw); zsdk_input_destroy(self.raw); }
        }
    }
}

pub struct KeyboardMouse {
    raw: *mut zsdk_kbm_t,
}

impl KeyboardMouse {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_kbm_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_kbm_init(raw, platform_ctx) }) {
                unsafe { zsdk_kbm_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }

    #[inline] pub fn update(&mut self) { unsafe { zsdk_kbm_update(self.raw); } }
    #[inline] pub fn is_key_down(&self, hid_key: u16) -> bool { unsafe { zsdk_kbm_is_key_down(self.raw, hid_key) != 0 } }

    #[inline]
    pub fn mouse_delta(&self) -> (f32, f32) {
        let (mut dx, mut dy) = (0.0f32, 0.0f32);
        unsafe { zsdk_kbm_mouse_delta(self.raw, &mut dx, &mut dy); }
        (dx, dy)
    }

    #[inline] pub fn mouse_buttons(&self) -> u8 { unsafe { zsdk_kbm_mouse_buttons(self.raw) } }
    #[inline] pub fn has_keyboard(&self) -> bool { unsafe { zsdk_kbm_has_keyboard(self.raw) != 0 } }
    #[inline] pub fn has_mouse(&self) -> bool { unsafe { zsdk_kbm_has_mouse(self.raw) != 0 } }
}

impl Drop for KeyboardMouse {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe { zsdk_kbm_shutdown(self.raw); zsdk_kbm_destroy(self.raw); }
        }
    }
}

pub struct VirtualKeyboard {
    raw: *mut zsdk_vkbd_t,
}

impl VirtualKeyboard {
    pub fn new() -> Result<Self> {
        let raw = unsafe { zsdk_vkbd_create() };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }

    #[inline]
    pub fn open(&mut self, title: &CStr, initial_text: &CStr, max_len: u8) {
        unsafe { zsdk_vkbd_open(self.raw, title.as_ptr(), initial_text.as_ptr(), max_len); }
    }

    #[inline] pub fn close(&mut self) { unsafe { zsdk_vkbd_close(self.raw); } }
    #[inline] pub fn update(&mut self, input: &InputManager, player: u8) { unsafe { zsdk_vkbd_update(self.raw, input.as_raw(), player); } }
    #[inline] pub fn render(&self, viewport_w: i32, viewport_h: i32) { unsafe { zsdk_vkbd_render(self.raw, viewport_w, viewport_h); } }
    #[inline] pub fn is_active(&self) -> bool { unsafe { zsdk_vkbd_is_active(self.raw) != 0 } }
    #[inline] pub fn is_submitted(&self) -> bool { unsafe { zsdk_vkbd_is_submitted(self.raw) != 0 } }
    #[inline] pub fn is_cancelled(&self) -> bool { unsafe { zsdk_vkbd_is_cancelled(self.raw) != 0 } }

    #[inline]
    pub fn text(&self) -> &CStr {
        let ptr = unsafe { zsdk_vkbd_get_text(self.raw) };
        if ptr.is_null() { c"" } else { unsafe { CStr::from_ptr(ptr) } }
    }
}

impl Drop for VirtualKeyboard {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_vkbd_destroy(self.raw); } }
    }
}
