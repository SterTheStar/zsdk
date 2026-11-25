use core::ffi::{c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)]
#[derive(Copy, Clone, Debug, Default)]
pub struct zsdk_sys_mem_info_t {
    pub free_ram_bytes:           u32,
    pub total_ram_bytes:          u32,
    pub largest_free_block_bytes: u32,
    pub free_fs_bytes:            u32,
    pub total_fs_bytes:           u32,
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Default)]
pub struct zsdk_battery_status_t {
    pub present:       c_int,
    pub charging:      c_int,
    pub level_percent: u8,
}

extern "C" {
    pub fn zsdk_clock_uptime_ms() -> u32;
    pub fn zsdk_clock_wall_ms() -> u32;
    pub fn zsdk_memory_ram_used() -> u32;
    pub fn zsdk_memory_vram_used() -> u32;
    pub fn zsdk_memory_budget() -> u32;
    pub fn zsdk_system_get_memory_info(out_info: *mut zsdk_sys_mem_info_t) -> zsdk_result_t;
    pub fn zsdk_system_get_language_code(platform_ctx: *mut c_void) -> u32;
    pub fn zsdk_system_get_battery_status(platform_ctx: *mut c_void, out_status: *mut zsdk_battery_status_t) -> zsdk_result_t;
    pub fn zsdk_system_set_backlight(platform_ctx: *mut c_void, enabled: c_int, level_percent: u8) -> zsdk_result_t;
    pub fn zsdk_debug_overlay_set_visible(visible: c_int);
    pub fn zsdk_debug_overlay_is_visible() -> c_int;
    pub fn zsdk_debug_overlay_record_draw_call(vertex_count: u32);
    pub fn zsdk_app_set_fps_cap(fps: u8);
    pub fn zsdk_app_request_quit();
}
