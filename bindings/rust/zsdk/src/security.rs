use ::core::ffi::c_void;
use zsdk_sys::*;
use crate::{check, Result};

pub use zsdk_sys::ZsdkHashAlgo as HashAlgo;

#[inline]
pub fn random_u32(platform_ctx: *mut c_void) -> u32 {
    unsafe { zsdk_random_u32(platform_ctx) }
}

pub fn random_bytes(platform_ctx: *mut c_void, out_buf: &mut [u8]) -> Result<()> {
    check(unsafe { zsdk_random_bytes(platform_ctx, out_buf.as_mut_ptr() as *mut _, out_buf.len() as u32) })
}

pub fn hash_compute(platform_ctx: *mut c_void, algo: HashAlgo, data: &[u8], out_digest: &mut [u8]) -> Result<u32> {
    let mut written = 0u32;
    check(unsafe {
        zsdk_hash_compute(platform_ctx, algo, data.as_ptr() as *const _, data.len() as u32,
                          out_digest.as_mut_ptr() as *mut _, out_digest.len() as u32, &mut written)
    })?;
    Ok(written)
}

pub fn hmac_compute(platform_ctx: *mut c_void, algo: HashAlgo, key: &[u8], data: &[u8], out_mac: &mut [u8]) -> Result<u32> {
    let mut written = 0u32;
    check(unsafe {
        zsdk_hmac_compute(platform_ctx, algo, key.as_ptr() as *const _, key.len() as u32,
                          data.as_ptr() as *const _, data.len() as u32,
                          out_mac.as_mut_ptr() as *mut _, out_mac.len() as u32, &mut written)
    })?;
    Ok(written)
}

pub fn check_license(platform_ctx: *mut c_void) -> Result<(bool, bool, u32)> {
    let (mut valid, mut expired, mut days) = (0, 0, 0u32);
    check(unsafe { zsdk_license_check(platform_ctx, &mut valid, &mut expired, &mut days) })?;
    Ok((valid != 0, expired != 0, days))
}
