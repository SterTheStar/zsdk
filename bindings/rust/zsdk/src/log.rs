use core::ffi::CStr;
use zsdk_sys::{zsdk_log_msg, ZsdkLogLevel};

#[inline]
pub fn write(level: ZsdkLogLevel, tag: &CStr, message: &CStr) {
    unsafe { zsdk_log_msg(level, tag.as_ptr(), message.as_ptr()); }
}

#[inline]
pub fn info(tag: &CStr, message: &CStr) { write(ZsdkLogLevel::Info, tag, message); }

#[inline]
pub fn warn(tag: &CStr, message: &CStr) { write(ZsdkLogLevel::Warn, tag, message); }

#[inline]
pub fn error(tag: &CStr, message: &CStr) { write(ZsdkLogLevel::Error, tag, message); }
