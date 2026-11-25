/**
 * @file zsdk_c_fs.h
 * @brief C99 ABI for ZSDK FileSystem, File, LocalStorage, and SaveGameManager.
 */

#ifndef ZSDK_C_FS_H
#define ZSDK_C_FS_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_fs_t            zsdk_fs_t;
typedef struct zsdk_file_t          zsdk_file_t;
typedef struct zsdk_local_storage_t zsdk_local_storage_t;
typedef struct zsdk_savegame_t      zsdk_savegame_t;
typedef struct zsdk_ram_cache_t     zsdk_ram_cache_t;

typedef enum zsdk_file_mode_t {
    ZSDK_FILE_READ       = 0,
    ZSDK_FILE_READ_WRITE = 1,
    ZSDK_FILE_CREATE     = 2,
    ZSDK_FILE_APPEND     = 3
} zsdk_file_mode_t;

typedef enum zsdk_seek_origin_t {
    ZSDK_SEEK_START   = 0,
    ZSDK_SEEK_CURRENT = 1,
    ZSDK_SEEK_END     = 2
} zsdk_seek_origin_t;

/* FileSystem */
ZSDK_C_API zsdk_fs_t*    zsdk_fs_create(void);
ZSDK_C_API void          zsdk_fs_destroy(zsdk_fs_t* fs);
ZSDK_C_API zsdk_result_t zsdk_fs_init(zsdk_fs_t* fs, void* platform_ctx);
ZSDK_C_API void          zsdk_fs_shutdown(zsdk_fs_t* fs);
ZSDK_C_API int           zsdk_fs_exists(const zsdk_fs_t* fs, const char* path);
ZSDK_C_API zsdk_result_t zsdk_fs_remove(zsdk_fs_t* fs, const char* path);
ZSDK_C_API zsdk_result_t zsdk_fs_mkdir(zsdk_fs_t* fs, const char* path);
ZSDK_C_API zsdk_result_t zsdk_fs_read_all_bytes(zsdk_fs_t* fs, const char* path, void** out_buf, uint32_t* out_size);
ZSDK_C_API void          zsdk_fs_free_buffer(void* buf);

/* File */
ZSDK_C_API zsdk_file_t*  zsdk_file_open(zsdk_fs_t* fs, const char* path, zsdk_file_mode_t mode, zsdk_result_t* out_result);
ZSDK_C_API void          zsdk_file_destroy(zsdk_file_t* file);
ZSDK_C_API int           zsdk_file_is_open(const zsdk_file_t* file);
ZSDK_C_API int32_t       zsdk_file_read(zsdk_file_t* file, void* buffer, uint32_t bytes);
ZSDK_C_API int32_t       zsdk_file_write(zsdk_file_t* file, const void* buffer, uint32_t bytes);
ZSDK_C_API zsdk_result_t zsdk_file_seek(zsdk_file_t* file, zsdk_seek_origin_t origin, int32_t offset);
ZSDK_C_API uint32_t      zsdk_file_get_size(const zsdk_file_t* file);
ZSDK_C_API void          zsdk_file_close(zsdk_file_t* file);

/* LocalStorage */
ZSDK_C_API zsdk_local_storage_t* zsdk_storage_create(void);
ZSDK_C_API void                  zsdk_storage_destroy(zsdk_local_storage_t* storage);
ZSDK_C_API zsdk_result_t         zsdk_storage_init(zsdk_local_storage_t* storage, void* platform_ctx);
ZSDK_C_API void                  zsdk_storage_shutdown(zsdk_local_storage_t* storage);
ZSDK_C_API zsdk_result_t         zsdk_storage_write(zsdk_local_storage_t* storage, const char* key, const void* data, uint32_t size_bytes);
ZSDK_C_API zsdk_result_t         zsdk_storage_read(zsdk_local_storage_t* storage, const char* key, void* out_buf, uint32_t max_bytes, uint32_t* out_read);
ZSDK_C_API zsdk_result_t         zsdk_storage_remove(zsdk_local_storage_t* storage, const char* key);

/* SaveGameManager */
ZSDK_C_API zsdk_savegame_t* zsdk_savegame_create(void);
ZSDK_C_API void             zsdk_savegame_destroy(zsdk_savegame_t* sg);
ZSDK_C_API zsdk_result_t    zsdk_savegame_init(zsdk_savegame_t* sg, void* platform_ctx, int prefer_sd_card);
ZSDK_C_API void             zsdk_savegame_shutdown(zsdk_savegame_t* sg);
ZSDK_C_API zsdk_result_t    zsdk_savegame_save_slot(zsdk_savegame_t* sg, uint8_t slot, const void* data, uint32_t size_bytes, uint32_t game_ver);
ZSDK_C_API zsdk_result_t    zsdk_savegame_load_slot(zsdk_savegame_t* sg, uint8_t slot, void* out_buf, uint32_t max_bytes, uint32_t* out_read, uint32_t expected_ver);
ZSDK_C_API int              zsdk_savegame_exists_slot(const zsdk_savegame_t* sg, uint8_t slot);
ZSDK_C_API zsdk_result_t    zsdk_savegame_delete_slot(zsdk_savegame_t* sg, uint8_t slot);

/* RamCache */
ZSDK_C_API zsdk_ram_cache_t* zsdk_ram_cache_create(void);
ZSDK_C_API void              zsdk_ram_cache_destroy(zsdk_ram_cache_t* cache);
ZSDK_C_API zsdk_result_t     zsdk_ram_cache_init(zsdk_ram_cache_t* cache, void* platform_ctx, uint32_t max_bytes);
ZSDK_C_API void              zsdk_ram_cache_shutdown(zsdk_ram_cache_t* cache);
ZSDK_C_API zsdk_result_t     zsdk_ram_cache_put(zsdk_ram_cache_t* cache, const char* key, const void* data, uint32_t size_bytes);
ZSDK_C_API zsdk_result_t     zsdk_ram_cache_get(zsdk_ram_cache_t* cache, const char* key, void* out_buf, uint32_t max_bytes, uint32_t* out_size);
ZSDK_C_API void              zsdk_ram_cache_clear(zsdk_ram_cache_t* cache);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_FS_H */
