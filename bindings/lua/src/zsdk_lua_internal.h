/**
 * @file zsdk_lua_internal.h
 * @brief Internal Lua 5.1 - 5.4 compatibility helpers for ZSDK Lua bindings.
 */

#ifndef ZSDK_LUA_INTERNAL_H
#define ZSDK_LUA_INTERNAL_H

#include <lua.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM >= 502
    /* Lua 5.2+ renamed lua_objlen to lua_rawlen. */
    #ifndef lua_objlen
        #define lua_objlen(L, i) lua_rawlen((L), (i))
    #endif
#endif

static void zsdk_lua_register_funcs(lua_State* L, const luaL_Reg* funcs) {
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, funcs, 0);
#else
    for (; funcs->name != NULL; ++funcs) {
        lua_pushcfunction(L, funcs->func);
        lua_setfield(L, -2, funcs->name);
    }
#endif
}

static size_t zsdk_lua_rawlen(lua_State* L, int idx) {
#if LUA_VERSION_NUM >= 502
    return lua_rawlen(L, idx);
#else
    return lua_objlen(L, idx);
#endif
}

#endif /* ZSDK_LUA_INTERNAL_H */
