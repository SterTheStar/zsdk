/**
 * @file zsdk_lua_log.c
 * @brief Lua bindings for zsdk_c_log (zsdk.log.info/warn/error/write).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_log.h"

#include <string.h>

static zsdk_log_level_t parse_log_level(lua_State* L, int idx) {
    if (lua_type(L, idx) == LUA_TNUMBER) {
        lua_Integer lvl = lua_tointeger(L, idx);
        if (lvl == ZSDK_LOG_LEVEL_WARN)  return ZSDK_LOG_LEVEL_WARN;
        if (lvl == ZSDK_LOG_LEVEL_ERROR) return ZSDK_LOG_LEVEL_ERROR;
        return ZSDK_LOG_LEVEL_INFO;
    }
    if (lua_type(L, idx) == LUA_TSTRING) {
        const char* s = lua_tostring(L, idx);
        if (s != NULL) {
            if (strcmp(s, "warn") == 0 || strcmp(s, "WARN") == 0)   return ZSDK_LOG_LEVEL_WARN;
            if (strcmp(s, "error") == 0 || strcmp(s, "ERROR") == 0) return ZSDK_LOG_LEVEL_ERROR;
        }
    }
    return ZSDK_LOG_LEVEL_INFO;
}

static int l_log_info(lua_State* L) {
    const char* tag = luaL_optstring(L, 1, "LUA");
    const char* msg = luaL_checkstring(L, 2);
    zsdk_log_msg(ZSDK_LOG_LEVEL_INFO, tag, msg);
    return 0;
}

static int l_log_warn(lua_State* L) {
    const char* tag = luaL_optstring(L, 1, "LUA");
    const char* msg = luaL_checkstring(L, 2);
    zsdk_log_msg(ZSDK_LOG_LEVEL_WARN, tag, msg);
    return 0;
}

static int l_log_error(lua_State* L) {
    const char* tag = luaL_optstring(L, 1, "LUA");
    const char* msg = luaL_checkstring(L, 2);
    zsdk_log_msg(ZSDK_LOG_LEVEL_ERROR, tag, msg);
    return 0;
}

static int l_log_write(lua_State* L) {
    zsdk_log_level_t level = parse_log_level(L, 1);
    const char* tag = luaL_optstring(L, 2, "LUA");
    const char* msg = luaL_checkstring(L, 3);
    zsdk_log_msg(level, tag, msg);
    return 0;
}

static const luaL_Reg kLogFuncs[] = {
    { "info",  l_log_info  },
    { "warn",  l_log_warn  },
    { "error", l_log_error },
    { "write", l_log_write },
    { NULL,    NULL        }
};

int luaopen_zsdk_log(lua_State* L) {
    lua_newtable(L);
    zsdk_lua_register_funcs(L, kLogFuncs);

    lua_pushinteger(L, ZSDK_LOG_LEVEL_INFO);
    lua_setfield(L, -2, "INFO");
    lua_pushinteger(L, ZSDK_LOG_LEVEL_WARN);
    lua_setfield(L, -2, "WARN");
    lua_pushinteger(L, ZSDK_LOG_LEVEL_ERROR);
    lua_setfield(L, -2, "ERROR");

    return 1;
}
