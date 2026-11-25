/**
 * @file zsdk_lua_net.c
 * @brief Lua bindings for zsdk_c_net (NetworkManager + Socket).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_net.h"

#define ZSDK_LUA_NET_MT  "zsdk.NetworkManager"
#define ZSDK_LUA_SOCK_MT "zsdk.Socket"

typedef struct { zsdk_net_mgr_t* ptr; } zsdk_lua_net_ud;
typedef struct { zsdk_socket_t*  ptr; } zsdk_lua_sock_ud;

static zsdk_net_mgr_t* check_net(lua_State* L, int idx) {
    return ((zsdk_lua_net_ud*)luaL_checkudata(L, idx, ZSDK_LUA_NET_MT))->ptr;
}

static zsdk_socket_t* check_sock(lua_State* L, int idx) {
    return ((zsdk_lua_sock_ud*)luaL_checkudata(L, idx, ZSDK_LUA_SOCK_MT))->ptr;
}

static int l_net_gc(lua_State* L) {
    zsdk_lua_net_ud* ud = (zsdk_lua_net_ud*)luaL_checkudata(L, 1, ZSDK_LUA_NET_MT);
    if (ud->ptr) {
        zsdk_net_shutdown(ud->ptr);
        zsdk_net_destroy(ud->ptr);
        ud->ptr = NULL;
    }
    return 0;
}

static int l_net_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_net_mgr_t* mgr = zsdk_net_create();
    if (!mgr) return luaL_error(L, "zsdk.net.new_manager: out of memory");
    if (ctx && zsdk_net_init(mgr, ctx) != ZSDK_C_OK) {
        zsdk_net_destroy(mgr);
        return luaL_error(L, "zsdk.net.new_manager: init failed");
    }
    zsdk_lua_net_ud* ud = (zsdk_lua_net_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = mgr;
    luaL_getmetatable(L, ZSDK_LUA_NET_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_net_is_available(lua_State* L) {
    lua_pushboolean(L, zsdk_net_is_available(check_net(L, 1)));
    return 1;
}

static int l_net_get_state(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_net_get_state(check_net(L, 1)));
    return 1;
}

static int l_net_local_ip(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_net_get_local_ip(check_net(L, 1)));
    return 1;
}

static int l_sock_gc(lua_State* L) {
    zsdk_lua_sock_ud* ud = (zsdk_lua_sock_ud*)luaL_checkudata(L, 1, ZSDK_LUA_SOCK_MT);
    if (ud->ptr) {
        zsdk_socket_close(ud->ptr);
        zsdk_socket_destroy(ud->ptr);
        ud->ptr = NULL;
    }
    return 0;
}

static int l_sock_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_socket_proto_t proto = (zsdk_socket_proto_t)luaL_optinteger(L, 2, ZSDK_SOCK_TCP);
    zsdk_socket_t* sock = zsdk_socket_create();
    if (!sock) return luaL_error(L, "zsdk.net.new_socket: out of memory");
    if (ctx && zsdk_socket_open(sock, ctx, proto) != ZSDK_C_OK) {
        zsdk_socket_destroy(sock);
        return luaL_error(L, "zsdk.net.new_socket: open failed");
    }
    zsdk_lua_sock_ud* ud = (zsdk_lua_sock_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = sock;
    luaL_getmetatable(L, ZSDK_LUA_SOCK_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_sock_connect_ipv4(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_socket_connect_ipv4(
        check_sock(L, 1), (uint32_t)luaL_checkinteger(L, 2), (uint16_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_sock_send(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    zsdk_result_t st = ZSDK_C_OK;
    int32_t sent = zsdk_socket_send(check_sock(L, 1), data, (uint32_t)len, &st);
    lua_pushinteger(L, (lua_Integer)sent);
    lua_pushinteger(L, (lua_Integer)st);
    return 2;
}

static int l_sock_close(lua_State* L) {
    zsdk_socket_close(check_sock(L, 1));
    return 0;
}

static int l_sock_recv(lua_State* L) {
    uint32_t max_bytes = (uint32_t)luaL_optinteger(L, 2, 1024);
    char buf[1024];
    if (max_bytes > sizeof(buf)) max_bytes = sizeof(buf);
    zsdk_result_t st = ZSDK_C_OK;
    int32_t n = zsdk_socket_recv(check_sock(L, 1), buf, max_bytes, &st);
    if (n > 0) lua_pushlstring(L, buf, (size_t)n);
    else       lua_pushnil(L);
    lua_pushinteger(L, (lua_Integer)st);
    return 2;
}

static int l_sock_is_open(lua_State* L) {
    lua_pushboolean(L, zsdk_socket_is_open(check_sock(L, 1)));
    return 1;
}

/* HttpClient */
#define ZSDK_LUA_HTTP_MT "zsdk.HttpClient"
typedef struct { zsdk_http_t* ptr; } zsdk_lua_http_ud;

static int l_http_gc(lua_State* L) {
    zsdk_lua_http_ud* ud = (zsdk_lua_http_ud*)luaL_checkudata(L, 1, ZSDK_LUA_HTTP_MT);
    if (ud->ptr) { zsdk_http_shutdown(ud->ptr); zsdk_http_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_http_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_http_t* h = zsdk_http_create();
    if (!h) return luaL_error(L, "zsdk.net.new_http: out of memory");
    if (ctx && zsdk_http_init(h, ctx) != ZSDK_C_OK) { zsdk_http_destroy(h); return luaL_error(L, "init failed"); }
    zsdk_lua_http_ud* ud = (zsdk_lua_http_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = h;
    luaL_getmetatable(L, ZSDK_LUA_HTTP_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_net_cancel_dns(lua_State* L) { zsdk_net_cancel_dns(check_net(L, 1)); return 0; }
static int l_http_set_max_response_bytes(lua_State* L) { zsdk_http_set_max_response_bytes(((zsdk_lua_http_ud*)luaL_checkudata(L, 1, ZSDK_LUA_HTTP_MT))->ptr, (uint32_t)luaL_checkinteger(L, 2)); return 0; }
static int l_http_cancel_pending(lua_State* L) { zsdk_http_cancel_pending(((zsdk_lua_http_ud*)luaL_checkudata(L, 1, ZSDK_LUA_HTTP_MT))->ptr); return 0; }

static const luaL_Reg kNetMethods[] = {
    { "is_available", l_net_is_available },
    { "get_state",    l_net_get_state    },
    { "local_ip",     l_net_local_ip     },
    { "cancel_dns",   l_net_cancel_dns   },
    { "__gc",         l_net_gc           },
    { NULL,           NULL               }
};

static const luaL_Reg kSockMethods[] = {
    { "connect_ipv4", l_sock_connect_ipv4 },
    { "send",         l_sock_send         },
    { "recv",         l_sock_recv         },
    { "is_open",      l_sock_is_open      },
    { "close",        l_sock_close        },
    { "__gc",         l_sock_gc           },
    { NULL,           NULL                }
};

static const luaL_Reg kHttpMethods[] = {
    { "set_max_response_bytes", l_http_set_max_response_bytes },
    { "cancel_pending",         l_http_cancel_pending         },
    { "__gc",                   l_http_gc                     },
    { NULL,                     NULL                          }
};

static const luaL_Reg kNetModule[] = {
    { "new_manager", l_net_new  },
    { "new_socket",  l_sock_new },
    { "new_http",    l_http_new },
    { NULL,          NULL       }
};

int luaopen_zsdk_net(lua_State* L) {
    if (luaL_newmetatable(L, ZSDK_LUA_NET_MT)) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kNetMethods);
    }
    lua_pop(L, 1);

    if (luaL_newmetatable(L, ZSDK_LUA_SOCK_MT)) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kSockMethods);
    }
    lua_pop(L, 1);

    if (luaL_newmetatable(L, ZSDK_LUA_HTTP_MT)) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        zsdk_lua_register_funcs(L, kHttpMethods);
    }
    lua_pop(L, 1);

    lua_newtable(L);
    zsdk_lua_register_funcs(L, kNetModule);
    lua_pushinteger(L, ZSDK_SOCK_TCP); lua_setfield(L, -2, "SOCK_TCP");
    lua_pushinteger(L, ZSDK_SOCK_UDP); lua_setfield(L, -2, "SOCK_UDP");
    return 1;
}
