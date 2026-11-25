use ::core::ffi::c_void;
use zsdk_sys::*;
use crate::{check, Result};

#[inline] pub fn uptime_ms() -> u32 { unsafe { zsdk_clock_uptime_ms() } }
#[inline] pub fn wall_ms() -> u32 { unsafe { zsdk_clock_wall_ms() } }
#[inline] pub fn ram_used() -> u32 { unsafe { zsdk_memory_ram_used() } }
#[inline] pub fn vram_used() -> u32 { unsafe { zsdk_memory_vram_used() } }
#[inline] pub fn memory_budget() -> u32 { unsafe { zsdk_memory_budget() } }

pub fn memory_info() -> Result<zsdk_sys_mem_info_t> {
    let mut info = zsdk_sys_mem_info_t::default();
    check(unsafe { zsdk_system_get_memory_info(&mut info) })?;
    Ok(info)
}

#[inline]
pub fn language_code(platform_ctx: *mut c_void) -> u32 {
    unsafe { zsdk_system_get_language_code(platform_ctx) }
}

pub struct DebugOverlay;
impl DebugOverlay {
    #[inline] pub fn set_visible(visible: bool) { unsafe { zsdk_debug_overlay_set_visible(visible as i32); } }
    #[inline] pub fn is_visible() -> bool { unsafe { zsdk_debug_overlay_is_visible() != 0 } }
    #[inline] pub fn record_draw_call(vertex_count: u32) { unsafe { zsdk_debug_overlay_record_draw_call(vertex_count); } }
}

#[inline] pub fn set_fps_cap(fps: u8) { unsafe { zsdk_app_set_fps_cap(fps); } }
#[inline] pub fn request_quit() { unsafe { zsdk_app_request_quit(); } }

pub fn battery_status(platform_ctx: *mut c_void) -> Result<zsdk_battery_status_t> {
    let mut bs = zsdk_battery_status_t::default();
    check(unsafe { zsdk_system_get_battery_status(platform_ctx, &mut bs) })?;
    Ok(bs)
}

#[inline]
pub fn set_backlight(platform_ctx: *mut c_void, enabled: bool, level_percent: u8) -> Result<()> {
    check(unsafe { zsdk_system_set_backlight(platform_ctx, enabled as i32, level_percent) })
}
