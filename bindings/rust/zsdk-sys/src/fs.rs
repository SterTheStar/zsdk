use core::ffi::{c_char, c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)] pub struct zsdk_fs_t            { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_file_t          { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_local_storage_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_savegame_t      { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_ram_cache_t     { _private: [u8; 0] }

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkFileMode {
    Read      = 0,
    ReadWrite = 1,
    Create    = 2,
    Append    = 3,
}
pub type zsdk_file_mode_t = ZsdkFileMode;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkSeekOrigin {
    Start   = 0,
    Current = 1,
    End     = 2,
}
pub type zsdk_seek_origin_t = ZsdkSeekOrigin;

extern "C" {
    pub fn zsdk_fs_create() -> *mut zsdk_fs_t;
    pub fn zsdk_fs_destroy(fs: *mut zsdk_fs_t);
    pub fn zsdk_fs_init(fs: *mut zsdk_fs_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_fs_shutdown(fs: *mut zsdk_fs_t);
    pub fn zsdk_fs_exists(fs: *const zsdk_fs_t, path: *const c_char) -> c_int;
    pub fn zsdk_fs_remove(fs: *mut zsdk_fs_t, path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_fs_mkdir(fs: *mut zsdk_fs_t, path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_fs_rename(fs: *mut zsdk_fs_t, src_path: *const c_char, dst_path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_fs_get_free_bytes(fs: *const zsdk_fs_t, out_total: *mut u32) -> u32;
    pub fn zsdk_fs_read_all_bytes(fs: *mut zsdk_fs_t, path: *const c_char, out_buf: *mut *mut c_void, out_size: *mut u32) -> zsdk_result_t;
    pub fn zsdk_fs_free_buffer(buf: *mut c_void);

    pub fn zsdk_file_open(fs: *mut zsdk_fs_t, path: *const c_char, mode: zsdk_file_mode_t, out_result: *mut zsdk_result_t) -> *mut zsdk_file_t;
    pub fn zsdk_file_destroy(file: *mut zsdk_file_t);
    pub fn zsdk_file_is_open(file: *const zsdk_file_t) -> c_int;
    pub fn zsdk_file_read(file: *mut zsdk_file_t, buffer: *mut c_void, bytes: u32) -> i32;
    pub fn zsdk_file_write(file: *mut zsdk_file_t, buffer: *const c_void, bytes: u32) -> i32;
    pub fn zsdk_file_seek(file: *mut zsdk_file_t, origin: zsdk_seek_origin_t, offset: i32) -> zsdk_result_t;
    pub fn zsdk_file_get_size(file: *const zsdk_file_t) -> u32;
    pub fn zsdk_file_close(file: *mut zsdk_file_t);

    pub fn zsdk_storage_create() -> *mut zsdk_local_storage_t;
    pub fn zsdk_storage_destroy(storage: *mut zsdk_local_storage_t);
    pub fn zsdk_storage_init(storage: *mut zsdk_local_storage_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_storage_shutdown(storage: *mut zsdk_local_storage_t);
    pub fn zsdk_storage_write(storage: *mut zsdk_local_storage_t, key: *const c_char, data: *const c_void, size_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_storage_read(storage: *mut zsdk_local_storage_t, key: *const c_char, out_buf: *mut c_void, max_bytes: u32, out_read: *mut u32) -> zsdk_result_t;
    pub fn zsdk_storage_remove(storage: *mut zsdk_local_storage_t, key: *const c_char) -> zsdk_result_t;

    pub fn zsdk_savegame_create() -> *mut zsdk_savegame_t;
    pub fn zsdk_savegame_destroy(sg: *mut zsdk_savegame_t);
    pub fn zsdk_savegame_init(sg: *mut zsdk_savegame_t, platform_ctx: *mut c_void, prefer_sd_card: c_int) -> zsdk_result_t;
    pub fn zsdk_savegame_shutdown(sg: *mut zsdk_savegame_t);
    pub fn zsdk_savegame_save_slot(sg: *mut zsdk_savegame_t, slot: u8, data: *const c_void, size_bytes: u32, game_ver: u32) -> zsdk_result_t;
    pub fn zsdk_savegame_load_slot(sg: *mut zsdk_savegame_t, slot: u8, out_buf: *mut c_void, max_bytes: u32, out_read: *mut u32, expected_ver: u32) -> zsdk_result_t;
    pub fn zsdk_savegame_begin_save_slot(sg: *mut zsdk_savegame_t, slot: u8, data: *const c_void, size_bytes: u32, game_ver: u32) -> zsdk_result_t;
    pub fn zsdk_savegame_pump_async(sg: *mut zsdk_savegame_t, budget_bytes: u32) -> c_int;
    pub fn zsdk_savegame_async_status(sg: *const zsdk_savegame_t) -> c_int;
    pub fn zsdk_savegame_exists_slot(sg: *const zsdk_savegame_t, slot: u8) -> c_int;
    pub fn zsdk_savegame_delete_slot(sg: *mut zsdk_savegame_t, slot: u8) -> zsdk_result_t;

    pub fn zsdk_ram_cache_create() -> *mut zsdk_ram_cache_t;
    pub fn zsdk_ram_cache_destroy(cache: *mut zsdk_ram_cache_t);
    pub fn zsdk_ram_cache_init(cache: *mut zsdk_ram_cache_t, platform_ctx: *mut c_void, max_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_ram_cache_shutdown(cache: *mut zsdk_ram_cache_t);
    pub fn zsdk_ram_cache_put(cache: *mut zsdk_ram_cache_t, key: *const c_char, data: *const c_void, size_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_ram_cache_get(cache: *mut zsdk_ram_cache_t, key: *const c_char, out_buf: *mut c_void, max_bytes: u32, out_size: *mut u32) -> zsdk_result_t;
    pub fn zsdk_ram_cache_clear(cache: *mut zsdk_ram_cache_t);

    pub fn zsdk_savegame_is_using_sd_card(sg: *const zsdk_savegame_t) -> c_int;
    pub fn zsdk_savegame_write_user_file(sg: *mut zsdk_savegame_t, filename: *const c_char, data: *const c_void, size_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_savegame_read_user_file(sg: *mut zsdk_savegame_t, filename: *const c_char, out_buf: *mut c_void, max_bytes: u32, out_read: *mut u32) -> zsdk_result_t;
    pub fn zsdk_savegame_exists_user_file(sg: *const zsdk_savegame_t, filename: *const c_char) -> c_int;
    pub fn zsdk_savegame_delete_user_file(sg: *mut zsdk_savegame_t, filename: *const c_char) -> zsdk_result_t;
    pub fn zsdk_fs_enumerate_dir(fs: *mut zsdk_fs_t, dir_path: *const c_char, dirs_only: c_int, cb: zsdk_fs_dir_cb_t, user_data: *mut c_void) -> zsdk_result_t;
}

pub type zsdk_fs_dir_cb_t = Option<unsafe extern "C" fn(name: *const c_char, size_bytes: u32, is_dir: c_int, user_data: *mut c_void) -> c_int>;
