/**
 * @file zsdk_c_fs.cpp
 * @brief C ABI bridge for FileSystem, File, LocalStorage, and SaveGameManager.
 */

#include "zsdk/c/zsdk_c_fs.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/fs/filesystem.hpp"
#include "zsdk/fs/local_storage.hpp"
#include "zsdk/fs/savegame.hpp"
#include "zsdk/fs/ram_cache.hpp"

struct zsdk_fs_t {
    zsdk::fs::FileSystem fs;
};

struct zsdk_file_t {
    zsdk::fs::File file;
};

struct zsdk_local_storage_t {
    zsdk::fs::LocalStorage storage;
};

struct zsdk_savegame_t {
    zsdk::fs::SaveGameManager sg;
};

struct zsdk_ram_cache_t {
    zsdk::fs::RamCache cache;
};

extern "C" {

zsdk_fs_t* zsdk_fs_create(void) {
    return zsdk::c_abi::Alloc<zsdk_fs_t>(zsdk::debug::MemorySubsystem::Filesystem);
}

void zsdk_fs_destroy(zsdk_fs_t* fs) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, fs);
}

zsdk_result_t zsdk_fs_init(zsdk_fs_t* fs, void* platform_ctx) {
    if (!fs) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(fs->fs.Initialize(platform_ctx));
}

void zsdk_fs_shutdown(zsdk_fs_t* fs) {
    if (fs) fs->fs.Shutdown();
}

int zsdk_fs_exists(const zsdk_fs_t* fs, const char* path) {
    return (fs && fs->fs.Exists(path)) ? 1 : 0;
}

zsdk_result_t zsdk_fs_remove(zsdk_fs_t* fs, const char* path) {
    if (!fs) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(fs->fs.Remove(path));
}

zsdk_result_t zsdk_fs_rename(zsdk_fs_t* fs, const char* src_path, const char* dst_path) {
    if (!fs) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(fs->fs.Rename(src_path, dst_path));
}

uint32_t zsdk_fs_get_free_bytes(const zsdk_fs_t* fs, uint32_t* out_total) {
    if (!fs) {
        if (out_total) *out_total = 0u;
        return 0u;
    }
    return fs->fs.GetFreeBytes(out_total);
}

zsdk_result_t zsdk_fs_mkdir(zsdk_fs_t* fs, const char* path) {
    if (!fs) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(fs->fs.MkDir(path));
}

zsdk_result_t zsdk_fs_read_all_bytes(zsdk_fs_t* fs, const char* path, void** out_buf, uint32_t* out_size) {
    if (!fs || !out_buf || !out_size) return ZSDK_C_ERR_INVALID_ARG;
    uint32_t sz = 0u;
    zsdk::Result r = fs->fs.ReadAllBytes(path, out_buf, sz);
    *out_size = sz;
    return zsdk::c_abi::ToCResult(r);
}

void zsdk_fs_free_buffer(void* buf) {
    zsdk::fs::FileSystem::FreeBuffer(buf);
}

zsdk_file_t* zsdk_file_open(zsdk_fs_t* fs, const char* path, zsdk_file_mode_t mode, zsdk_result_t* out_result) {
    if (!fs || !path) {
        if (out_result) *out_result = ZSDK_C_ERR_INVALID_ARG;
        return nullptr;
    }
    zsdk_file_t* f = zsdk::c_abi::Alloc<zsdk_file_t>(zsdk::debug::MemorySubsystem::Filesystem);
    if (!f) {
        if (out_result) *out_result = ZSDK_C_ERR_OUT_OF_MEMORY;
        return nullptr;
    }
    zsdk::Result r = fs->fs.Open(path, static_cast<zsdk::fs::FileMode>(mode), f->file);
    if (out_result) *out_result = zsdk::c_abi::ToCResult(r);
    if (zsdk::Failed(r)) {
        zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, f);
        return nullptr;
    }
    return f;
}

void zsdk_file_destroy(zsdk_file_t* file) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, file);
}

int zsdk_file_is_open(const zsdk_file_t* file) {
    return (file && file->file.IsOpen()) ? 1 : 0;
}

int32_t zsdk_file_read(zsdk_file_t* file, void* buffer, uint32_t bytes) {
    return file ? file->file.Read(buffer, bytes) : -1;
}

int32_t zsdk_file_write(zsdk_file_t* file, const void* buffer, uint32_t bytes) {
    return file ? file->file.Write(buffer, bytes) : -1;
}

zsdk_result_t zsdk_file_seek(zsdk_file_t* file, zsdk_seek_origin_t origin, int32_t offset) {
    if (!file) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(file->file.Seek(static_cast<zsdk::fs::SeekOrigin>(origin), offset));
}

uint32_t zsdk_file_get_size(const zsdk_file_t* file) {
    return file ? file->file.GetSize() : 0u;
}

void zsdk_file_close(zsdk_file_t* file) {
    if (file) file->file.Close();
}

zsdk_local_storage_t* zsdk_storage_create(void) {
    return zsdk::c_abi::Alloc<zsdk_local_storage_t>(zsdk::debug::MemorySubsystem::Filesystem);
}

void zsdk_storage_destroy(zsdk_local_storage_t* storage) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, storage);
}

zsdk_result_t zsdk_storage_init(zsdk_local_storage_t* storage, void* platform_ctx) {
    if (!storage) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(storage->storage.Initialize(platform_ctx));
}

void zsdk_storage_shutdown(zsdk_local_storage_t* storage) {
    if (storage) storage->storage.Shutdown();
}

zsdk_result_t zsdk_storage_write(zsdk_local_storage_t* storage, const char* key, const void* data, uint32_t size_bytes) {
    if (!storage) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(storage->storage.Write(key, data, size_bytes));
}

zsdk_result_t zsdk_storage_read(zsdk_local_storage_t* storage, const char* key, void* out_buf, uint32_t max_bytes, uint32_t* out_read) {
    if (!storage) return ZSDK_C_ERR_INVALID_ARG;
    uint32_t readBytes = 0u;
    zsdk::Result r = storage->storage.Read(key, out_buf, max_bytes, readBytes);
    if (out_read) *out_read = readBytes;
    return zsdk::c_abi::ToCResult(r);
}

zsdk_result_t zsdk_storage_remove(zsdk_local_storage_t* storage, const char* key) {
    if (!storage) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(storage->storage.Remove(key));
}

zsdk_savegame_t* zsdk_savegame_create(void) {
    return zsdk::c_abi::Alloc<zsdk_savegame_t>(zsdk::debug::MemorySubsystem::Filesystem);
}

void zsdk_savegame_destroy(zsdk_savegame_t* sg) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, sg);
}

zsdk_result_t zsdk_savegame_init(zsdk_savegame_t* sg, void* platform_ctx, int prefer_sd_card) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.Initialize(platform_ctx, prefer_sd_card != 0));
}

void zsdk_savegame_shutdown(zsdk_savegame_t* sg) {
    if (sg) sg->sg.Shutdown();
}

zsdk_result_t zsdk_savegame_save_slot(zsdk_savegame_t* sg, uint8_t slot, const void* data, uint32_t size_bytes, uint32_t game_ver) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.SaveSlot(slot, data, size_bytes, game_ver));
}

zsdk_result_t zsdk_savegame_begin_save_slot(zsdk_savegame_t* sg, uint8_t slot, const void* data, uint32_t size_bytes, uint32_t game_ver) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.BeginSaveSlot(slot, data, size_bytes, game_ver));
}

int zsdk_savegame_pump_async(zsdk_savegame_t* sg, uint32_t budget_bytes) {
    if (!sg) return 0;
    return static_cast<int>(sg->sg.PumpAsync(budget_bytes));
}

int zsdk_savegame_async_status(const zsdk_savegame_t* sg) {
    if (!sg) return 0;
    return static_cast<int>(sg->sg.GetAsyncStatus());
}

zsdk_result_t zsdk_savegame_load_slot(zsdk_savegame_t* sg, uint8_t slot, void* out_buf, uint32_t max_bytes, uint32_t* out_read, uint32_t expected_ver) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    uint32_t readBytes = 0u;
    zsdk::Result r = sg->sg.LoadSlot(slot, out_buf, max_bytes, readBytes, expected_ver);
    if (out_read) *out_read = readBytes;
    return zsdk::c_abi::ToCResult(r);
}

int zsdk_savegame_exists_slot(const zsdk_savegame_t* sg, uint8_t slot) {
    return (sg && sg->sg.HasSlot(slot)) ? 1 : 0;
}

zsdk_result_t zsdk_savegame_delete_slot(zsdk_savegame_t* sg, uint8_t slot) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.DeleteSlot(slot));
}

zsdk_ram_cache_t* zsdk_ram_cache_create(void) {
    return zsdk::c_abi::Alloc<zsdk_ram_cache_t>(zsdk::debug::MemorySubsystem::Filesystem);
}

void zsdk_ram_cache_destroy(zsdk_ram_cache_t* cache) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Filesystem, cache);
}

zsdk_result_t zsdk_ram_cache_init(zsdk_ram_cache_t* cache, void* platform_ctx, uint32_t max_bytes) {
    if (!cache) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(cache->cache.Initialize(platform_ctx, max_bytes));
}

void zsdk_ram_cache_shutdown(zsdk_ram_cache_t* cache) {
    if (cache) cache->cache.Shutdown();
}

zsdk_result_t zsdk_ram_cache_put(zsdk_ram_cache_t* cache, const char* key, const void* data, uint32_t size_bytes) {
    if (!cache) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(cache->cache.Put(key, data, size_bytes));
}

zsdk_result_t zsdk_ram_cache_get(zsdk_ram_cache_t* cache, const char* key, void* out_buf, uint32_t max_bytes, uint32_t* out_size) {
    if (!cache) return ZSDK_C_ERR_INVALID_ARG;
    uint32_t sz = 0u;
    zsdk::Result r = cache->cache.Get(key, out_buf, max_bytes, sz);
    if (out_size) *out_size = sz;
    return zsdk::c_abi::ToCResult(r);
}

void zsdk_ram_cache_clear(zsdk_ram_cache_t* cache) {
    if (cache) cache->cache.Clear();
}

int zsdk_savegame_is_using_sd_card(const zsdk_savegame_t* sg) {
    return (sg && sg->sg.IsUsingSdCard()) ? 1 : 0;
}

zsdk_result_t zsdk_savegame_write_user_file(zsdk_savegame_t* sg, const char* filename, const void* data, uint32_t size_bytes) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.WriteUserFile(filename, data, size_bytes));
}

zsdk_result_t zsdk_savegame_read_user_file(zsdk_savegame_t* sg, const char* filename, void* out_buf, uint32_t max_bytes, uint32_t* out_read) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    uint32_t readBytes = 0u;
    zsdk::Result r = sg->sg.ReadUserFile(filename, out_buf, max_bytes, readBytes);
    if (out_read) *out_read = readBytes;
    return zsdk::c_abi::ToCResult(r);
}

int zsdk_savegame_exists_user_file(const zsdk_savegame_t* sg, const char* filename) {
    return (sg && sg->sg.ExistsUserFile(filename)) ? 1 : 0;
}

zsdk_result_t zsdk_savegame_delete_user_file(zsdk_savegame_t* sg, const char* filename) {
    if (!sg) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sg->sg.DeleteUserFile(filename));
}

namespace {
struct ZsdkCDirBridge {
    zsdk_fs_dir_cb_t cb;
    void*            userData;
};
bool EnumDirThunk(const zsdk::fs::FileEntry& entry, void* ud) {
    auto* b = static_cast<ZsdkCDirBridge*>(ud);
    return b && b->cb ? (b->cb(entry.name, entry.size, entry.isDirectory ? 1 : 0, b->userData) != 0) : false;
}
} // namespace

zsdk_result_t zsdk_fs_enumerate_dir(zsdk_fs_t* fs, const char* dir_path, int dirs_only,
                                    zsdk_fs_dir_cb_t cb, void* user_data) {
    if (!fs || !cb) return ZSDK_C_ERR_INVALID_ARG;
    ZsdkCDirBridge bridge{cb, user_data};
    return zsdk::c_abi::ToCResult(fs->fs.EnumerateDirectory(dir_path, dirs_only != 0, EnumDirThunk, &bridge));
}

} /* extern "C" */
