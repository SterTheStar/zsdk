/**
 * @file zsdk_lua.h
 * @brief Modular Lua C API bindings for all ZSDK subsystems (Lua 5.1 - 5.4 / LuaJIT).
 *
 * Built strictly on top of the ZSDK C99 ABI (zsdk/c/zsdk_c.h).
 */

#ifndef ZSDK_LUA_H
#define ZSDK_LUA_H

#include "zsdk/c/zsdk_c_types.h"
#include <lua.h>

ZSDK_C_EXTERN_BEGIN

ZSDK_C_API int luaopen_zsdk(lua_State* L);
ZSDK_C_API int luaopen_zsdk_log(lua_State* L);
ZSDK_C_API int luaopen_zsdk_core(lua_State* L);
ZSDK_C_API int luaopen_zsdk_input(lua_State* L);
ZSDK_C_API int luaopen_zsdk_audio(lua_State* L);
ZSDK_C_API int luaopen_zsdk_graphics(lua_State* L);
ZSDK_C_API int luaopen_zsdk_fs(lua_State* L);
ZSDK_C_API int luaopen_zsdk_net(lua_State* L);
ZSDK_C_API int luaopen_zsdk_security(lua_State* L);

ZSDK_C_EXTERN_END

#endif /* ZSDK_LUA_H */
