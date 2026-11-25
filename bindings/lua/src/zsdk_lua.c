/**
 * @file zsdk_lua.c
 * @brief Top-level `zsdk` Lua module loader assembling all ZSDK submodules.
 */

#include "zsdk_lua.h"

int luaopen_zsdk(lua_State* L) {
    lua_newtable(L);

    luaopen_zsdk_log(L);      lua_setfield(L, -2, "log");
    luaopen_zsdk_core(L);     lua_setfield(L, -2, "core");
    luaopen_zsdk_input(L);    lua_setfield(L, -2, "input");
    luaopen_zsdk_audio(L);    lua_setfield(L, -2, "audio");
    luaopen_zsdk_graphics(L); lua_setfield(L, -2, "graphics");
    luaopen_zsdk_fs(L);       lua_setfield(L, -2, "fs");
    luaopen_zsdk_net(L);      lua_setfield(L, -2, "net");
    luaopen_zsdk_security(L); lua_setfield(L, -2, "security");

    return 1;
}
