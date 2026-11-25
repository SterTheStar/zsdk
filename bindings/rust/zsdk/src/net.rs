use ::core::ffi::{CStr, c_void};
use zsdk_sys::*;
use crate::{check, Error, Result};

pub use zsdk_sys::{ZsdkNetState as NetState, ZsdkSocketProto as SocketProto};

pub struct NetworkManager { raw: *mut zsdk_net_mgr_t }
impl NetworkManager {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_net_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_net_init(raw, platform_ctx) }) {
                unsafe { zsdk_net_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }
    pub fn state(&self) -> NetState { unsafe { zsdk_net_get_state(self.raw) } }
    pub fn is_available(&self) -> bool { unsafe { zsdk_net_is_available(self.raw) != 0 } }
    pub fn local_ip(&self) -> u32 { unsafe { zsdk_net_get_local_ip(self.raw) } }
    pub fn cancel_dns(&mut self) { unsafe { zsdk_net_cancel_dns(self.raw); } }
    pub unsafe fn resolve_host_raw(&mut self, hostname: &::core::ffi::CStr, cb: zsdk_dns_cb_t, user_data: *mut c_void) -> Result<()> {
        check(zsdk_net_resolve_host(self.raw, hostname.as_ptr(), cb, user_data))
    }
}
impl Drop for NetworkManager {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_net_shutdown(self.raw); zsdk_net_destroy(self.raw); } }
    }
}

pub struct Socket { raw: *mut zsdk_socket_t }
impl Socket {
    pub fn open(platform_ctx: *mut c_void, proto: SocketProto) -> Result<Self> {
        let raw = unsafe { zsdk_socket_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if let Err(e) = check(unsafe { zsdk_socket_open(raw, platform_ctx, proto) }) {
            unsafe { zsdk_socket_destroy(raw); }
            return Err(e);
        }
        Ok(Self { raw })
    }
    pub fn connect_ipv4(&mut self, ipv4_host_order: u32, port: u16) -> Result<()> {
        check(unsafe { zsdk_socket_connect_ipv4(self.raw, ipv4_host_order, port) })
    }
    pub fn send(&mut self, data: &[u8]) -> Result<i32> {
        let mut st = Error::Ok;
        let n = unsafe { zsdk_socket_send(self.raw, data.as_ptr() as *const _, data.len() as u32, &mut st) };
        check(st)?;
        Ok(n)
    }
    pub fn recv(&mut self, buf: &mut [u8]) -> Result<i32> {
        let mut st = Error::Ok;
        let n = unsafe { zsdk_socket_recv(self.raw, buf.as_mut_ptr() as *mut _, buf.len() as u32, &mut st) };
        check(st)?;
        Ok(n)
    }
    pub fn is_open(&self) -> bool { unsafe { zsdk_socket_is_open(self.raw) != 0 } }
}
impl Drop for Socket {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_socket_close(self.raw); zsdk_socket_destroy(self.raw); } }
    }
}

pub struct HttpClient { raw: *mut zsdk_http_t }
impl HttpClient {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_http_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_http_init(raw, platform_ctx) }) {
                unsafe { zsdk_http_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }

    pub unsafe fn get_raw(&mut self, url: &CStr, cb: zsdk_http_cb_t, user_data: *mut c_void) -> Result<()> {
        check(zsdk_http_get(self.raw, url.as_ptr(), cb, user_data))
    }
    pub fn set_max_response_bytes(&mut self, max_bytes: u32) { unsafe { zsdk_http_set_max_response_bytes(self.raw, max_bytes); } }
    pub unsafe fn post_raw(&mut self, url: &CStr, content_type: &CStr, body: &[u8], cb: zsdk_http_cb_t, user_data: *mut c_void) -> Result<()> {
        check(zsdk_http_post(self.raw, url.as_ptr(), content_type.as_ptr(), body.as_ptr() as *const _, body.len() as u32, cb, user_data))
    }
    pub fn cancel_pending(&mut self) { unsafe { zsdk_http_cancel_pending(self.raw); } }
}
impl Drop for HttpClient {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_http_shutdown(self.raw); zsdk_http_destroy(self.raw); } }
    }
}
