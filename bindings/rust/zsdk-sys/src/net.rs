use core::ffi::{c_char, c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)] pub struct zsdk_net_mgr_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_socket_t  { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_http_t    { _private: [u8; 0] }

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkNetState {
    Disconnected = 0,
    Connecting   = 1,
    Connected    = 2,
}
pub type zsdk_net_state_t = ZsdkNetState;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkSocketProto {
    Tcp = 0,
    Udp = 1,
}
pub type zsdk_socket_proto_t = ZsdkSocketProto;

pub type zsdk_http_cb_t = Option<
    unsafe extern "C" fn(http_status: i32, body: *const c_void, body_bytes: u32, user_data: *mut c_void),
>;

extern "C" {
    pub fn zsdk_net_create() -> *mut zsdk_net_mgr_t;
    pub fn zsdk_net_destroy(mgr: *mut zsdk_net_mgr_t);
    pub fn zsdk_net_init(mgr: *mut zsdk_net_mgr_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_net_shutdown(mgr: *mut zsdk_net_mgr_t);
    pub fn zsdk_net_get_state(mgr: *const zsdk_net_mgr_t) -> zsdk_net_state_t;
    pub fn zsdk_net_is_available(mgr: *const zsdk_net_mgr_t) -> c_int;
    pub fn zsdk_net_get_local_ip(mgr: *const zsdk_net_mgr_t) -> u32;

    pub fn zsdk_http_create() -> *mut zsdk_http_t;
    pub fn zsdk_http_destroy(http: *mut zsdk_http_t);
    pub fn zsdk_http_init(http: *mut zsdk_http_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_http_shutdown(http: *mut zsdk_http_t);
    pub fn zsdk_http_get(http: *mut zsdk_http_t, url: *const c_char, cb: zsdk_http_cb_t, user_data: *mut c_void) -> zsdk_result_t;

    pub fn zsdk_socket_create() -> *mut zsdk_socket_t;
    pub fn zsdk_socket_destroy(sock: *mut zsdk_socket_t);
    pub fn zsdk_socket_open(sock: *mut zsdk_socket_t, platform_ctx: *mut c_void, proto: zsdk_socket_proto_t) -> zsdk_result_t;
    pub fn zsdk_socket_close(sock: *mut zsdk_socket_t);
    pub fn zsdk_socket_connect_ipv4(sock: *mut zsdk_socket_t, ipv4_host_order: u32, port: u16) -> zsdk_result_t;
    pub fn zsdk_socket_send(sock: *mut zsdk_socket_t, data: *const c_void, size_bytes: u32, out_status: *mut zsdk_result_t) -> i32;
    pub fn zsdk_socket_recv(sock: *mut zsdk_socket_t, buffer: *mut c_void, max_bytes: u32, out_status: *mut zsdk_result_t) -> i32;
    pub fn zsdk_socket_is_open(sock: *const zsdk_socket_t) -> c_int;

    pub fn zsdk_net_cancel_dns(mgr: *mut zsdk_net_mgr_t);
    pub fn zsdk_http_set_max_response_bytes(http: *mut zsdk_http_t, max_bytes: u32);
    pub fn zsdk_http_post(http: *mut zsdk_http_t, url: *const c_char, content_type: *const c_char, body: *const c_void, body_bytes: u32, cb: zsdk_http_cb_t, user_data: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_http_cancel_pending(http: *mut zsdk_http_t);
}
