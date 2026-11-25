use core::ffi::c_char;
use crate::types::zsdk_log_level_t;

extern "C" {
    pub fn zsdk_log(level: zsdk_log_level_t, tag: *const c_char, fmt: *const c_char, ...);
    pub fn zsdk_log_msg(level: zsdk_log_level_t, tag: *const c_char, message: *const c_char);
}
