/**
 * @file zsdk_lua_fs.c
 * @brief Complete Lua bindings for zsdk_c_fs (FileSystem, File, LocalStorage, SaveGameManager, RamCache).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_fs.h"

#define ZSDK_LUA_FS_MT      "zsdk.FileSystem"
#define ZSDK_LUA_FILE_MT    "zsdk.File"
#define ZSDK_LUA_STORAGE_MT "zsdk.LocalStorage"
#define ZSDK_LUA_SG_MT      "zsdk.SaveGameManager"
#define ZSDK_LUA_CACHE_MT   "zsdk.RamCache"

typedef struct { zsdk_fs_t*            ptr; } zsdk_lua_fs_ud;
typedef struct { zsdk_file_t*          ptr; } zsdk_lua_file_ud;
typedef struct { zsdk_local_storage_t* ptr; } zsdk_lua_storage_ud;
typedef struct { zsdk_savegame_t*      ptr; } zsdk_lua_sg_ud;
typedef struct { zsdk_ram_cache_t*     ptr; } zsdk_lua_cache_ud;

static zsdk_fs_t* check_fs(lua_State* L, int idx)                { return ((zsdk_lua_fs_ud*)luaL_checkudata(L, idx, ZSDK_LUA_FS_MT))->ptr; }
static zsdk_file_t* check_file(lua_State* L, int idx)            { return ((zsdk_lua_file_ud*)luaL_checkudata(L, idx, ZSDK_LUA_FILE_MT))->ptr; }
static zsdk_local_storage_t* check_storage(lua_State* L, int idx){ return ((zsdk_lua_storage_ud*)luaL_checkudata(L, idx, ZSDK_LUA_STORAGE_MT))->ptr; }
static zsdk_savegame_t* check_sg(lua_State* L, int idx)          { return ((zsdk_lua_sg_ud*)luaL_checkudata(L, idx, ZSDK_LUA_SG_MT))->ptr; }
static zsdk_ram_cache_t* check_cache(lua_State* L, int idx)      { return ((zsdk_lua_cache_ud*)luaL_checkudata(L, idx, ZSDK_LUA_CACHE_MT))->ptr; }

/* FileSystem */
static int l_fs_gc(lua_State* L) {
    zsdk_lua_fs_ud* ud = (zsdk_lua_fs_ud*)luaL_checkudata(L, 1, ZSDK_LUA_FS_MT);
    if (ud->ptr) { zsdk_fs_shutdown(ud->ptr); zsdk_fs_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_fs_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_fs_t* fs = zsdk_fs_create();
    if (!fs) return luaL_error(L, "zsdk.fs.new_fs: out of memory");
    if (ctx && zsdk_fs_init(fs, ctx) != ZSDK_C_OK) { zsdk_fs_destroy(fs); return luaL_error(L, "init failed"); }
    zsdk_lua_fs_ud* ud = (zsdk_lua_fs_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = fs;
    luaL_getmetatable(L, ZSDK_LUA_FS_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_fs_exists(lua_State* L) { lua_pushboolean(L, zsdk_fs_exists(check_fs(L, 1), luaL_checkstring(L, 2))); return 1; }
static int l_fs_remove(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_fs_remove(check_fs(L, 1), luaL_checkstring(L, 2))); return 1; }
static int l_fs_mkdir(lua_State* L)  { lua_pushinteger(L, (lua_Integer)zsdk_fs_mkdir(check_fs(L, 1), luaL_checkstring(L, 2))); return 1; }

static int l_fs_read_text(lua_State* L) {
    void* buf = NULL;
    uint32_t size = 0;
    zsdk_result_t r = zsdk_fs_read_all_bytes(check_fs(L, 1), luaL_checkstring(L, 2), &buf, &size);
    if (r != ZSDK_C_OK || !buf) { lua_pushnil(L); lua_pushinteger(L, (lua_Integer)r); return 2; }
    lua_pushlstring(L, (const char*)buf, (size_t)size);
    zsdk_fs_free_buffer(buf);
    return 1;
}

static int l_fs_open(lua_State* L) {
    zsdk_result_t r = ZSDK_C_OK;
    zsdk_file_t* f = zsdk_file_open(
        check_fs(L, 1), luaL_checkstring(L, 2),
        (zsdk_file_mode_t)luaL_optinteger(L, 3, ZSDK_FILE_READ), &r);
    if (!f) { lua_pushnil(L); lua_pushinteger(L, (lua_Integer)r); return 2; }
    zsdk_lua_file_ud* ud = (zsdk_lua_file_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = f;
    luaL_getmetatable(L, ZSDK_LUA_FILE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

/* File */
static int l_file_gc(lua_State* L) {
    zsdk_lua_file_ud* ud = (zsdk_lua_file_ud*)luaL_checkudata(L, 1, ZSDK_LUA_FILE_MT);
    if (ud->ptr) { zsdk_file_close(ud->ptr); zsdk_file_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_file_is_open(lua_State* L)  { lua_pushboolean(L, zsdk_file_is_open(check_file(L, 1))); return 1; }
static int l_file_get_size(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_file_get_size(check_file(L, 1))); return 1; }
static int l_file_close(lua_State* L)    { zsdk_file_close(check_file(L, 1)); return 0; }
static int l_file_seek(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_file_seek(
        check_file(L, 1), (zsdk_seek_origin_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3)));
    return 1;
}
static int l_file_read(lua_State* L) {
    uint32_t max_bytes = (uint32_t)luaL_optinteger(L, 2, 1024);
    char buf[1024];
    if (max_bytes > sizeof(buf)) max_bytes = sizeof(buf);
    int32_t n = zsdk_file_read(check_file(L, 1), buf, max_bytes);
    if (n >= 0) lua_pushlstring(L, buf, (size_t)n);
    else        lua_pushnil(L);
    return 1;
}
static int l_file_write(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_file_write(check_file(L, 1), data, (uint32_t)len));
    return 1;
}

/* LocalStorage */
static int l_storage_gc(lua_State* L) {
    zsdk_lua_storage_ud* ud = (zsdk_lua_storage_ud*)luaL_checkudata(L, 1, ZSDK_LUA_STORAGE_MT);
    if (ud->ptr) { zsdk_storage_shutdown(ud->ptr); zsdk_storage_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_storage_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_local_storage_t* st = zsdk_storage_create();
    if (!st) return luaL_error(L, "zsdk.fs.new_storage: out of memory");
    if (ctx && zsdk_storage_init(st, ctx) != ZSDK_C_OK) { zsdk_storage_destroy(st); return luaL_error(L, "init failed"); }
    zsdk_lua_storage_ud* ud = (zsdk_lua_storage_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = st;
    luaL_getmetatable(L, ZSDK_LUA_STORAGE_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_storage_write(lua_State* L) {
    size_t len = 0;
    const char* key = luaL_checkstring(L, 2);
    const char* data = luaL_checklstring(L, 3, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_storage_write(check_storage(L, 1), key, data, (uint32_t)len));
    return 1;
}
static int l_storage_read(lua_State* L) {
    char buf[1024];
    uint32_t read_n = 0;
    zsdk_result_t r = zsdk_storage_read(check_storage(L, 1), luaL_checkstring(L, 2), buf, sizeof(buf), &read_n);
    if (r != ZSDK_C_OK) { lua_pushnil(L); lua_pushinteger(L, (lua_Integer)r); return 2; }
    lua_pushlstring(L, buf, (size_t)read_n);
    return 1;
}
static int l_storage_remove(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_storage_remove(check_storage(L, 1), luaL_checkstring(L, 2)));
    return 1;
}

/* SaveGameManager */
static int l_sg_gc(lua_State* L) {
    zsdk_lua_sg_ud* ud = (zsdk_lua_sg_ud*)luaL_checkudata(L, 1, ZSDK_LUA_SG_MT);
    if (ud->ptr) { zsdk_savegame_shutdown(ud->ptr); zsdk_savegame_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_sg_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_savegame_t* sg = zsdk_savegame_create();
    if (!sg) return luaL_error(L, "zsdk.fs.new_savegame: out of memory");
    if (ctx && zsdk_savegame_init(sg, ctx, lua_toboolean(L, 2)) != ZSDK_C_OK) { zsdk_savegame_destroy(sg); return luaL_error(L, "init failed"); }
    zsdk_lua_sg_ud* ud = (zsdk_lua_sg_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = sg;
    luaL_getmetatable(L, ZSDK_LUA_SG_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_sg_save_slot(lua_State* L) {
    size_t len = 0;
    uint8_t slot = (uint8_t)luaL_checkinteger(L, 2);
    const char* data = luaL_checklstring(L, 3, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_savegame_save_slot(check_sg(L, 1), slot, data, (uint32_t)len, (uint32_t)luaL_optinteger(L, 4, 1)));
    return 1;
}
static int l_sg_load_slot(lua_State* L) {
    char buf[2048];
    uint32_t read_n = 0;
    zsdk_result_t r = zsdk_savegame_load_slot(
        check_sg(L, 1), (uint8_t)luaL_checkinteger(L, 2), buf, sizeof(buf), &read_n,
        (uint32_t)luaL_optinteger(L, 3, 1));
    if (r != ZSDK_C_OK) { lua_pushnil(L); lua_pushinteger(L, (lua_Integer)r); return 2; }
    lua_pushlstring(L, buf, (size_t)read_n);
    return 1;
}
static int l_sg_exists_slot(lua_State* L) { lua_pushboolean(L, zsdk_savegame_exists_slot(check_sg(L, 1), (uint8_t)luaL_checkinteger(L, 2))); return 1; }
static int l_sg_delete_slot(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_savegame_delete_slot(check_sg(L, 1), (uint8_t)luaL_checkinteger(L, 2))); return 1; }

/* RamCache */
static int l_cache_gc(lua_State* L) {
    zsdk_lua_cache_ud* ud = (zsdk_lua_cache_ud*)luaL_checkudata(L, 1, ZSDK_LUA_CACHE_MT);
    if (ud->ptr) { zsdk_ram_cache_shutdown(ud->ptr); zsdk_ram_cache_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_cache_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    uint32_t max_bytes = (uint32_t)luaL_optinteger(L, 2, 65536);
    zsdk_ram_cache_t* c = zsdk_ram_cache_create();
    if (!c) return luaL_error(L, "zsdk.fs.new_ram_cache: out of memory");
    if (ctx && zsdk_ram_cache_init(c, ctx, max_bytes) != ZSDK_C_OK) { zsdk_ram_cache_destroy(c); return luaL_error(L, "init failed"); }
    zsdk_lua_cache_ud* ud = (zsdk_lua_cache_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = c;
    luaL_getmetatable(L, ZSDK_LUA_CACHE_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_cache_put(lua_State* L) {
    size_t len = 0;
    const char* key = luaL_checkstring(L, 2);
    const char* data = luaL_checklstring(L, 3, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_ram_cache_put(check_cache(L, 1), key, data, (uint32_t)len));
    return 1;
}
static int l_cache_get(lua_State* L) {
    char buf[1024];
    uint32_t sz = 0;
    zsdk_result_t r = zsdk_ram_cache_get(check_cache(L, 1), luaL_checkstring(L, 2), buf, sizeof(buf), &sz);
    if (r != ZSDK_C_OK) { lua_pushnil(L); lua_pushinteger(L, (lua_Integer)r); return 2; }
    lua_pushlstring(L, buf, (size_t)sz);
    return 1;
}
static int l_cache_clear(lua_State* L) { zsdk_ram_cache_clear(check_cache(L, 1)); return 0; }

static const luaL_Reg kFsMethods[]      = { { "exists", l_fs_exists }, { "remove", l_fs_remove }, { "mkdir", l_fs_mkdir }, { "read_text", l_fs_read_text }, { "open", l_fs_open }, { "__gc", l_fs_gc }, { NULL, NULL } };
static const luaL_Reg kFileMethods[]    = { { "is_open", l_file_is_open }, { "read", l_file_read }, { "write", l_file_write }, { "seek", l_file_seek }, { "get_size", l_file_get_size }, { "close", l_file_close }, { "__gc", l_file_gc }, { NULL, NULL } };
static const luaL_Reg kStorageMethods[] = { { "write", l_storage_write }, { "read", l_storage_read }, { "remove", l_storage_remove }, { "__gc", l_storage_gc }, { NULL, NULL } };
static const luaL_Reg kSgMethods[]      = { { "save_slot", l_sg_save_slot }, { "load_slot", l_sg_load_slot }, { "exists_slot", l_sg_exists_slot }, { "delete_slot", l_sg_delete_slot }, { "__gc", l_sg_gc }, { NULL, NULL } };
static const luaL_Reg kCacheMethods[]   = { { "put", l_cache_put }, { "get", l_cache_get }, { "clear", l_cache_clear }, { "__gc", l_cache_gc }, { NULL, NULL } };

static const luaL_Reg kFsModule[] = {
    { "new_fs",        l_fs_new      },
    { "new_storage",   l_storage_new },
    { "new_savegame",  l_sg_new      },
    { "new_ram_cache", l_cache_new   },
    { NULL,            NULL          }
};

int luaopen_zsdk_fs(lua_State* L) {
    if (luaL_newmetatable(L, ZSDK_LUA_FS_MT))      { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kFsMethods);      } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_FILE_MT))    { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kFileMethods);    } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_STORAGE_MT)) { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kStorageMethods); } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_SG_MT))      { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kSgMethods);      } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_CACHE_MT))   { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kCacheMethods);   } lua_pop(L, 1);

    lua_newtable(L);
    zsdk_lua_register_funcs(L, kFsModule);
    return 1;
}
