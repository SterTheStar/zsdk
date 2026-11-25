#![no_std]

pub use zsdk_sys as sys;
pub use zsdk_sys::{ZsdkLogLevel as LogLevel, ZsdkResult as Error};

pub type Result<T> = core::result::Result<T, Error>;

#[inline]
pub(crate) fn check(res: zsdk_sys::zsdk_result_t) -> Result<()> {
    if res == Error::Ok { Ok(()) } else { Err(res) }
}

pub mod log;
pub mod core_sys;
pub mod input;
pub mod audio;
pub mod graphics;
pub mod fs;
pub mod net;
pub mod security;
