use core::ffi::{c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkHashAlgo {
    Md5  = 0,
    Sha1 = 1,
}

pub type zsdk_hash_algo_t = ZsdkHashAlgo;

extern "C" {
    pub fn zsdk_random_bytes(platform_ctx: *mut c_void, out_buf: *mut c_void, size_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_random_u32(platform_ctx: *mut c_void) -> u32;
    pub fn zsdk_hash_compute(
        platform_ctx: *mut c_void,
        algo: zsdk_hash_algo_t,
        data: *const c_void,
        size_bytes: u32,
        out_digest: *mut c_void,
        digest_buf_bytes: u32,
        out_digest_bytes: *mut u32,
    ) -> zsdk_result_t;
    pub fn zsdk_hmac_compute(
        platform_ctx: *mut c_void,
        algo: zsdk_hash_algo_t,
        key: *const c_void,
        key_bytes: u32,
        data: *const c_void,
        size_bytes: u32,
        out_mac: *mut c_void,
        mac_buf_bytes: u32,
        out_mac_bytes: *mut u32,
    ) -> zsdk_result_t;
    pub fn zsdk_license_check(
        platform_ctx: *mut c_void,
        out_valid: *mut c_int,
        out_expired: *mut c_int,
        out_remaining_days: *mut u32,
    ) -> zsdk_result_t;
}
