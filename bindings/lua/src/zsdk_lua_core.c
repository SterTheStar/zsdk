/**
 * @file zsdk_lua_core.c
 * @brief Lua bindings for zsdk_c_core (clock, memory telemetry, system info, debug overlay).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_core.h"

static int l_core_uptime_ms(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_clock_uptime_ms());
    return 1;
}

static int l_core_wall_ms(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_clock_wall_ms());
    return 1;
}

static int l_core_ram_used(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_memory_ram_used());
    return 1;
}

static int l_core_vram_used(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_memory_vram_used());
    return 1;
}

static int l_core_memory_budget(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_memory_budget());
    return 1;
}

static int l_core_mem_info(lua_State* L) {
    zsdk_sys_mem_info_t info;
    if (zsdk_system_get_memory_info(&info) != ZSDK_C_OK) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    lua_pushinteger(L, (lua_Integer)info.free_ram_bytes);
    lua_setfield(L, -2, "free_ram");
    lua_pushinteger(L, (lua_Integer)info.total_ram_bytes);
    lua_setfield(L, -2, "total_ram");
    lua_pushinteger(L, (lua_Integer)info.free_fs_bytes);
    lua_setfield(L, -2, "free_fs");
    lua_pushinteger(L, (lua_Integer)info.total_fs_bytes);
    lua_setfield(L, -2, "total_fs");
    return 1;
}

static int l_core_language_code(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)zsdk_system_get_language_code(ctx));
    return 1;
}

static int l_core_set_overlay_visible(lua_State* L) {
    zsdk_debug_overlay_set_visible(lua_toboolean(L, 1));
    return 0;
}

static int l_core_is_overlay_visible(lua_State* L) {
    lua_pushboolean(L, zsdk_debug_overlay_is_visible());
    return 1;
}

static int l_core_record_draw_call(lua_State* L) {
    lua_Integer verts = luaL_optinteger(L, 1, 0);
    zsdk_debug_overlay_record_draw_call((uint32_t)(verts > 0 ? verts : 0));
    return 0;
}

static const luaL_Reg kCoreFuncs[] = {
    { "uptime_ms",           l_core_uptime_ms           },
    { "wall_ms",             l_core_wall_ms             },
    { "ram_used",            l_core_ram_used            },
    { "vram_used",           l_core_vram_used           },
    { "memory_budget",       l_core_memory_budget       },
    { "mem_info",            l_core_mem_info            },
    { "language_code",       l_core_language_code       },
    { "set_overlay_visible", l_core_set_overlay_visible },
    { "is_overlay_visible",  l_core_is_overlay_visible  },
    { "record_draw_call",    l_core_record_draw_call    },
    { NULL,                  NULL                       }
};

int luaopen_zsdk_core(lua_State* L) {
    lua_newtable(L);
    zsdk_lua_register_funcs(L, kCoreFuncs);
    return 1;
}
