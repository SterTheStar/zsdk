/**
 * @file zsdk_lua_input.c
 * @brief Lua bindings for zsdk_c_input (InputManager + GamepadState).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_input.h"

#define ZSDK_LUA_INPUT_MT "zsdk.InputManager"

typedef struct zsdk_lua_input_ud {
    zsdk_input_t* ptr;
    int           owned;
} zsdk_lua_input_ud;

static zsdk_input_t* check_input(lua_State* L, int idx) {
    zsdk_lua_input_ud* ud = (zsdk_lua_input_ud*)luaL_checkudata(L, idx, ZSDK_LUA_INPUT_MT);
    return ud->ptr;
}

static int l_input_gc(lua_State* L) {
    zsdk_lua_input_ud* ud = (zsdk_lua_input_ud*)luaL_checkudata(L, 1, ZSDK_LUA_INPUT_MT);
    if (ud->ptr && ud->owned) {
        zsdk_input_shutdown(ud->ptr);
        zsdk_input_destroy(ud->ptr);
    }
    ud->ptr = NULL;
    return 0;
}

void zsdk_lua_push_input_view(lua_State* L, void* zsdk_input_ptr) {
    zsdk_lua_input_ud* ud = (zsdk_lua_input_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr   = (zsdk_input_t*)zsdk_input_ptr;
    ud->owned = 0;
    luaL_getmetatable(L, ZSDK_LUA_INPUT_MT);
    lua_setmetatable(L, -2);
}

static int l_input_new(lua_State* L) {
    void* platform_ctx = lua_touserdata(L, 1);
    zsdk_input_t* inp = zsdk_input_create();
    if (!inp) return luaL_error(L, "zsdk.input.new: out of memory");
    if (platform_ctx && zsdk_input_init(inp, platform_ctx) != ZSDK_C_OK) {
        zsdk_input_destroy(inp);
        return luaL_error(L, "zsdk.input.new: init failed");
    }
    zsdk_lua_input_ud* ud = (zsdk_lua_input_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr   = inp;
    ud->owned = 1;
    luaL_getmetatable(L, ZSDK_LUA_INPUT_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_input_update(lua_State* L) {
    zsdk_input_update(check_input(L, 1), (uint32_t)luaL_checkinteger(L, 2));
    return 0;
}

static int l_input_connected_count(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_input_connected_count(check_input(L, 1)));
    return 1;
}

static int l_input_is_connected(lua_State* L) {
    lua_pushboolean(L, zsdk_input_is_connected(check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0)));
    return 1;
}

/* KeyboardMouseManager */
#define ZSDK_LUA_KBM_MT  "zsdk.KeyboardMouse"
#define ZSDK_LUA_VKBD_MT "zsdk.VirtualKeyboard"
typedef struct { zsdk_kbm_t*  ptr; } zsdk_lua_kbm_ud;
typedef struct { zsdk_vkbd_t* ptr; } zsdk_lua_vkbd_ud;

static int l_kbm_gc(lua_State* L) {
    zsdk_lua_kbm_ud* ud = (zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT);
    if (ud->ptr) { zsdk_kbm_shutdown(ud->ptr); zsdk_kbm_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_kbm_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_kbm_t* k = zsdk_kbm_create();
    if (!k) return luaL_error(L, "zsdk.input.new_kbm: out of memory");
    if (ctx && zsdk_kbm_init(k, ctx) != ZSDK_C_OK) { zsdk_kbm_destroy(k); return luaL_error(L, "init failed"); }
    zsdk_lua_kbm_ud* ud = (zsdk_lua_kbm_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = k;
    luaL_getmetatable(L, ZSDK_LUA_KBM_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_kbm_update(lua_State* L) {
    zsdk_kbm_update(((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr);
    return 0;
}

static int l_kbm_is_key_down(lua_State* L) {
    lua_pushboolean(L, zsdk_kbm_is_key_down(
        ((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr,
        (uint16_t)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_kbm_mouse_delta(lua_State* L) {
    float dx = 0.0f, dy = 0.0f;
    zsdk_kbm_mouse_delta(((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr, &dx, &dy);
    lua_pushnumber(L, (lua_Number)dx);
    lua_pushnumber(L, (lua_Number)dy);
    return 2;
}

static int l_kbm_mouse_buttons(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_kbm_mouse_buttons(
        ((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr));
    return 1;
}

/* VirtualKeyboard */
static int l_vkbd_gc(lua_State* L) {
    zsdk_lua_vkbd_ud* ud = (zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT);
    if (ud->ptr) { zsdk_vkbd_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_vkbd_new(lua_State* L) {
    zsdk_vkbd_t* v = zsdk_vkbd_create();
    if (!v) return luaL_error(L, "zsdk.input.new_vkbd: out of memory");
    zsdk_lua_vkbd_ud* ud = (zsdk_lua_vkbd_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = v;
    luaL_getmetatable(L, ZSDK_LUA_VKBD_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_vkbd_open(lua_State* L) {
    zsdk_vkbd_open(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr,
                   luaL_checkstring(L, 2), luaL_optstring(L, 3, ""), (uint8_t)luaL_optinteger(L, 4, 32));
    return 0;
}

static int l_vkbd_close(lua_State* L) {
    zsdk_vkbd_close(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr);
    return 0;
}

static int l_vkbd_update(lua_State* L) {
    zsdk_vkbd_update(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr,
                     check_input(L, 2), (uint8_t)luaL_optinteger(L, 3, 0));
    return 0;
}

static int l_vkbd_render(lua_State* L) {
    zsdk_vkbd_render(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr,
                     (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_vkbd_is_active(lua_State* L) {
    lua_pushboolean(L, zsdk_vkbd_is_active(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr));
    return 1;
}

static int l_vkbd_is_submitted(lua_State* L) {
    lua_pushboolean(L, zsdk_vkbd_is_submitted(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr));
    return 1;
}

static int l_vkbd_is_cancelled(lua_State* L) {
    lua_pushboolean(L, zsdk_vkbd_is_cancelled(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr));
    return 1;
}

static int l_vkbd_get_text(lua_State* L) {
    lua_pushstring(L, zsdk_vkbd_get_text(((zsdk_lua_vkbd_ud*)luaL_checkudata(L, 1, ZSDK_LUA_VKBD_MT))->ptr));
    return 1;
}

static int l_input_is_down(lua_State* L) {
    lua_pushboolean(L, zsdk_input_is_down(
        check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0),
        (zsdk_button_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_input_is_pressed(lua_State* L) {
    lua_pushboolean(L, zsdk_input_is_pressed(
        check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0),
        (zsdk_button_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_input_is_released(lua_State* L) {
    lua_pushboolean(L, zsdk_input_is_released(
        check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0),
        (zsdk_button_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_input_left_stick(lua_State* L) {
    float x = 0.0f, y = 0.0f;
    zsdk_input_left_stick(check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0), &x, &y);
    lua_pushnumber(L, (lua_Number)x);
    lua_pushnumber(L, (lua_Number)y);
    return 2;
}

static int l_input_right_stick(lua_State* L) {
    float x = 0.0f, y = 0.0f;
    zsdk_input_right_stick(check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0), &x, &y);
    lua_pushnumber(L, (lua_Number)x);
    lua_pushnumber(L, (lua_Number)y);
    return 2;
}

static int l_input_set_rumble(lua_State* L) {
    zsdk_result_t r = zsdk_input_set_rumble(
        check_input(L, 1),
        (uint8_t)luaL_optinteger(L, 2, 0),
        (uint16_t)luaL_checkinteger(L, 3),
        (uint16_t)luaL_checkinteger(L, 4));
    lua_pushinteger(L, (lua_Integer)r);
    return 1;
}

static int l_input_hold_duration_ms(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_input_hold_duration_ms(
        check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0), (zsdk_button_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_input_is_combo_triggered(lua_State* L) {
    lua_pushboolean(L, zsdk_input_is_combo_triggered(
        check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0), (uint32_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_input_motion(lua_State* L) {
    float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    int sup = zsdk_input_motion(check_input(L, 1), (uint8_t)luaL_optinteger(L, 2, 0),
                                &ax, &ay, &az, &gx, &gy, &gz);
    lua_pushboolean(L, sup);
    lua_pushnumber(L, (lua_Number)ax); lua_pushnumber(L, (lua_Number)ay); lua_pushnumber(L, (lua_Number)az);
    lua_pushnumber(L, (lua_Number)gx); lua_pushnumber(L, (lua_Number)gy); lua_pushnumber(L, (lua_Number)gz);
    return 7;
}

static int l_kbm_has_keyboard(lua_State* L) {
    lua_pushboolean(L, zsdk_kbm_has_keyboard(((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr));
    return 1;
}

static int l_kbm_has_mouse(lua_State* L) {
    lua_pushboolean(L, zsdk_kbm_has_mouse(((zsdk_lua_kbm_ud*)luaL_checkudata(L, 1, ZSDK_LUA_KBM_MT))->ptr));
    return 1;
}

static const luaL_Reg kInputMethods[] = {
    { "update",             l_input_update             },
    { "connected_count",    l_input_connected_count    },
    { "is_connected",       l_input_is_connected       },
    { "is_down",            l_input_is_down            },
    { "is_pressed",         l_input_is_pressed         },
    { "is_released",        l_input_is_released        },
    { "left_stick",         l_input_left_stick         },
    { "right_stick",        l_input_right_stick        },
    { "set_rumble",         l_input_set_rumble         },
    { "hold_duration_ms",   l_input_hold_duration_ms   },
    { "is_combo_triggered", l_input_is_combo_triggered },
    { "motion",             l_input_motion             },
    { "__gc",               l_input_gc                 },
    { NULL,                 NULL                       }
};

static const luaL_Reg kKbmMethods[] = {
    { "update",        l_kbm_update        },
    { "is_key_down",   l_kbm_is_key_down   },
    { "mouse_delta",   l_kbm_mouse_delta   },
    { "mouse_buttons", l_kbm_mouse_buttons },
    { "has_keyboard",  l_kbm_has_keyboard  },
    { "has_mouse",     l_kbm_has_mouse     },
    { "__gc",          l_kbm_gc            },
    { NULL,            NULL                }
};

static const luaL_Reg kVkbdMethods[] = {
    { "open",         l_vkbd_open         },
    { "close",        l_vkbd_close        },
    { "update",       l_vkbd_update       },
    { "render",       l_vkbd_render       },
    { "is_active",    l_vkbd_is_active    },
    { "is_submitted", l_vkbd_is_submitted },
    { "is_cancelled", l_vkbd_is_cancelled },
    { "get_text",     l_vkbd_get_text     },
    { "__gc",         l_vkbd_gc           },
    { NULL,           NULL                }
};

static const luaL_Reg kInputModule[] = {
    { "new",      l_input_new },
    { "new_kbm",  l_kbm_new   },
    { "new_vkbd", l_vkbd_new  },
    { NULL,       NULL        }
};

int luaopen_zsdk_input(lua_State* L) {
    if (luaL_newmetatable(L, ZSDK_LUA_INPUT_MT)) {
        lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kInputMethods);
    }
    lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_KBM_MT)) {
        lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kKbmMethods);
    }
    lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_VKBD_MT)) {
        lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kVkbdMethods);
    }
    lua_pop(L, 1);

    lua_newtable(L);
    zsdk_lua_register_funcs(L, kInputModule);

    lua_pushinteger(L, ZSDK_BTN_DPAD_UP);        lua_setfield(L, -2, "BTN_DPAD_UP");
    lua_pushinteger(L, ZSDK_BTN_DPAD_DOWN);      lua_setfield(L, -2, "BTN_DPAD_DOWN");
    lua_pushinteger(L, ZSDK_BTN_DPAD_LEFT);      lua_setfield(L, -2, "BTN_DPAD_LEFT");
    lua_pushinteger(L, ZSDK_BTN_DPAD_RIGHT);     lua_setfield(L, -2, "BTN_DPAD_RIGHT");
    lua_pushinteger(L, ZSDK_BTN_ACTION1);        lua_setfield(L, -2, "BTN_ACTION1");
    lua_pushinteger(L, ZSDK_BTN_ACTION2);        lua_setfield(L, -2, "BTN_ACTION2");
    lua_pushinteger(L, ZSDK_BTN_ACTION3);        lua_setfield(L, -2, "BTN_ACTION3");
    lua_pushinteger(L, ZSDK_BTN_ACTION4);        lua_setfield(L, -2, "BTN_ACTION4");
    lua_pushinteger(L, ZSDK_BTN_SHOULDER_LEFT);  lua_setfield(L, -2, "BTN_SHOULDER_LEFT");
    lua_pushinteger(L, ZSDK_BTN_SHOULDER_RIGHT); lua_setfield(L, -2, "BTN_SHOULDER_RIGHT");
    lua_pushinteger(L, ZSDK_BTN_START);          lua_setfield(L, -2, "BTN_START");
    lua_pushinteger(L, ZSDK_BTN_BACK);           lua_setfield(L, -2, "BTN_BACK");
    lua_pushinteger(L, ZSDK_BTN_HOME);           lua_setfield(L, -2, "BTN_HOME");
    return 1;
}
