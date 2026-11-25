use ::core::ffi::{CStr, c_void};
use ::core::ops::Deref;
use ::core::ptr;
use ::core::slice;
use zsdk_sys::*;
use crate::{check, Error, Result};

pub use zsdk_sys::{ZsdkFileMode as FileMode, ZsdkSeekOrigin as SeekOrigin};

pub struct FsBuffer {
    ptr: *mut c_void,
    len: usize,
}

impl Deref for FsBuffer {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        if self.ptr.is_null() || self.len == 0 {
            &[]
        } else {
            unsafe { slice::from_raw_parts(self.ptr as *const u8, self.len) }
        }
    }
}

impl Drop for FsBuffer {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe { zsdk_fs_free_buffer(self.ptr); }
        }
    }
}

pub struct FileSystem { raw: *mut zsdk_fs_t }
impl FileSystem {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_fs_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_fs_init(raw, platform_ctx) }) {
                unsafe { zsdk_fs_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }
    pub fn exists(&self, path: &CStr) -> bool { unsafe { zsdk_fs_exists(self.raw, path.as_ptr()) != 0 } }
    pub fn remove(&mut self, path: &CStr) -> Result<()> { check(unsafe { zsdk_fs_remove(self.raw, path.as_ptr()) }) }
    pub fn mkdir(&mut self, path: &CStr) -> Result<()> { check(unsafe { zsdk_fs_mkdir(self.raw, path.as_ptr()) }) }
    pub fn rename(&mut self, src: &CStr, dst: &CStr) -> Result<()> { check(unsafe { zsdk_fs_rename(self.raw, src.as_ptr(), dst.as_ptr()) }) }
    pub fn free_bytes(&self) -> (u32, u32) {
        let mut total = 0u32;
        let free = unsafe { zsdk_fs_get_free_bytes(self.raw, &mut total) };
        (free, total)
    }
    pub fn read_all_bytes(&mut self, path: &CStr) -> Result<FsBuffer> {
        let mut ptr: *mut c_void = ptr::null_mut();
        let mut size = 0u32;
        check(unsafe { zsdk_fs_read_all_bytes(self.raw, path.as_ptr(), &mut ptr, &mut size) })?;
        Ok(FsBuffer { ptr, len: size as usize })
    }
    pub fn open(&mut self, path: &CStr, mode: FileMode) -> Result<File<'_>> {
        let mut res = Error::Ok;
        let raw = unsafe { zsdk_file_open(self.raw, path.as_ptr(), mode, &mut res) };
        if raw.is_null() {
            return Err(if res == Error::Ok { Error::ErrFailed } else { res });
        }
        check(res)?;
        Ok(File { raw, _fs: ::core::marker::PhantomData })
    }
}
impl Drop for FileSystem {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_fs_shutdown(self.raw); zsdk_fs_destroy(self.raw); } }
    }
}

pub struct SaveGameManager { raw: *mut zsdk_savegame_t }
impl SaveGameManager {
    pub fn new(platform_ctx: *mut c_void, prefer_sd_card: bool) -> Result<Self> {
        let raw = unsafe { zsdk_savegame_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if let Err(e) = check(unsafe { zsdk_savegame_init(raw, platform_ctx, prefer_sd_card as i32) }) {
            unsafe { zsdk_savegame_destroy(raw); }
            return Err(e);
        }
        Ok(Self { raw })
    }
    pub fn save_slot(&mut self, slot: u8, data: &[u8], game_ver: u32) -> Result<()> {
        check(unsafe { zsdk_savegame_save_slot(self.raw, slot, data.as_ptr() as *const _, data.len() as u32, game_ver) })
    }
    pub fn load_slot(&mut self, slot: u8, out_buf: &mut [u8], expected_ver: u32) -> Result<u32> {
        let mut read = 0u32;
        check(unsafe { zsdk_savegame_load_slot(self.raw, slot, out_buf.as_mut_ptr() as *mut _, out_buf.len() as u32, &mut read, expected_ver) })?;
        Ok(read)
    }
    pub fn exists_slot(&self, slot: u8) -> bool { unsafe { zsdk_savegame_exists_slot(self.raw, slot) != 0 } }
    pub fn delete_slot(&mut self, slot: u8) -> Result<()> { check(unsafe { zsdk_savegame_delete_slot(self.raw, slot) }) }
    pub fn begin_save_slot(&mut self, slot: u8, data: &[u8], game_ver: u32) -> Result<()> {
        check(unsafe { zsdk_savegame_begin_save_slot(self.raw, slot, data.as_ptr() as *const _, data.len() as u32, game_ver) })
    }
    pub fn pump_async(&mut self, budget_bytes: u32) -> i32 { unsafe { zsdk_savegame_pump_async(self.raw, budget_bytes) } }
    pub fn async_status(&self) -> i32 { unsafe { zsdk_savegame_async_status(self.raw) } }
}
impl Drop for SaveGameManager {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_savegame_shutdown(self.raw); zsdk_savegame_destroy(self.raw); } }
    }
}

pub struct File<'a> {
    raw: *mut zsdk_file_t,
    _fs: ::core::marker::PhantomData<&'a mut FileSystem>,
}

impl<'a> File<'a> {
    pub fn is_open(&self) -> bool { unsafe { zsdk_file_is_open(self.raw) != 0 } }
    pub fn read(&mut self, buf: &mut [u8]) -> i32 { unsafe { zsdk_file_read(self.raw, buf.as_mut_ptr() as *mut _, buf.len() as u32) } }
    pub fn write(&mut self, buf: &[u8]) -> i32 { unsafe { zsdk_file_write(self.raw, buf.as_ptr() as *const _, buf.len() as u32) } }
    pub fn seek(&mut self, origin: SeekOrigin, offset: i32) -> Result<()> { check(unsafe { zsdk_file_seek(self.raw, origin, offset) }) }
    pub fn size(&self) -> u32 { unsafe { zsdk_file_get_size(self.raw) } }
    pub fn close(&mut self) { unsafe { zsdk_file_close(self.raw); } }
}

impl<'a> Drop for File<'a> {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_file_close(self.raw); zsdk_file_destroy(self.raw); } }
    }
}

pub struct LocalStorage { raw: *mut zsdk_local_storage_t }
impl LocalStorage {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_storage_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_storage_init(raw, platform_ctx) }) {
                unsafe { zsdk_storage_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }
    pub fn write(&mut self, key: &CStr, data: &[u8]) -> Result<()> {
        check(unsafe { zsdk_storage_write(self.raw, key.as_ptr(), data.as_ptr() as *const _, data.len() as u32) })
    }
    pub fn read(&mut self, key: &CStr, out_buf: &mut [u8]) -> Result<u32> {
        let mut read = 0u32;
        check(unsafe { zsdk_storage_read(self.raw, key.as_ptr(), out_buf.as_mut_ptr() as *mut _, out_buf.len() as u32, &mut read) })?;
        Ok(read)
    }
    pub fn remove(&mut self, key: &CStr) -> Result<()> { check(unsafe { zsdk_storage_remove(self.raw, key.as_ptr()) }) }
}
impl Drop for LocalStorage {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_storage_shutdown(self.raw); zsdk_storage_destroy(self.raw); } }
    }
}

pub struct RamCache { raw: *mut zsdk_ram_cache_t }
impl RamCache {
    pub fn new(platform_ctx: *mut c_void, max_bytes: u32) -> Result<Self> {
        let raw = unsafe { zsdk_ram_cache_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if let Err(e) = check(unsafe { zsdk_ram_cache_init(raw, platform_ctx, max_bytes) }) {
            unsafe { zsdk_ram_cache_destroy(raw); }
            return Err(e);
        }
        Ok(Self { raw })
    }
    pub fn put(&mut self, key: &CStr, data: &[u8]) -> Result<()> {
        check(unsafe { zsdk_ram_cache_put(self.raw, key.as_ptr(), data.as_ptr() as *const _, data.len() as u32) })
    }
    pub fn get(&mut self, key: &CStr, out_buf: &mut [u8]) -> Result<u32> {
        let mut size = 0u32;
        check(unsafe { zsdk_ram_cache_get(self.raw, key.as_ptr(), out_buf.as_mut_ptr() as *mut _, out_buf.len() as u32, &mut size) })?;
        Ok(size)
    }
    pub fn clear(&mut self) { unsafe { zsdk_ram_cache_clear(self.raw); } }
}
impl Drop for RamCache {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_ram_cache_shutdown(self.raw); zsdk_ram_cache_destroy(self.raw); } }
    }
}
