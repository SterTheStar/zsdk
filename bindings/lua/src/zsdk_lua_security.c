/**
 * @file zsdk_lua_security.c
 * @brief Lua bindings for zsdk_c_security (Random, Hash, LicenseManager).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_security.h"

static int l_sec_random_u32(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    lua_pushinteger(L, (lua_Integer)zsdk_random_u32(ctx));
    return 1;
}

static int l_sec_random_bytes(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    uint32_t len = (uint32_t)luaL_checkinteger(L, 2);
    uint8_t buf[256];
    if (len == 0 || len > sizeof(buf)) {
        return luaL_error(L, "random_bytes: len must be 1..256");
    }
    zsdk_result_t r = zsdk_random_bytes(ctx, buf, len);
    if (r != ZSDK_C_OK) {
        lua_pushnil(L);
        lua_pushinteger(L, (lua_Integer)r);
        return 2;
    }
    lua_pushlstring(L, (const char*)buf, (size_t)len);
    return 1;
}

static int l_sec_hmac(lua_State* L, zsdk_hash_algo_t algo) {
    void* ctx = lua_touserdata(L, 1);
    size_t klen = 0, dlen = 0;
    const char* key  = luaL_checklstring(L, 2, &klen);
    const char* data = luaL_checklstring(L, 3, &dlen);
    uint8_t mac[20];
    uint32_t mac_len = 0;
    zsdk_result_t r = zsdk_hmac_compute(ctx, algo, key, (uint32_t)klen, data, (uint32_t)dlen, mac, sizeof(mac), &mac_len);
    if (r != ZSDK_C_OK) {
        lua_pushnil(L);
        lua_pushinteger(L, (lua_Integer)r);
        return 2;
    }
    lua_pushlstring(L, (const char*)mac, (size_t)mac_len);
    return 1;
}

static int l_sec_hmac_md5(lua_State* L)  { return l_sec_hmac(L, ZSDK_HASH_MD5); }
static int l_sec_hmac_sha1(lua_State* L) { return l_sec_hmac(L, ZSDK_HASH_SHA1); }

static int l_sec_hash(lua_State* L, zsdk_hash_algo_t algo) {
    void* ctx = lua_touserdata(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    uint8_t digest[20];
    uint32_t digest_len = 0;
    zsdk_result_t r = zsdk_hash_compute(ctx, algo, data, (uint32_t)len, digest, sizeof(digest), &digest_len);
    if (r != ZSDK_C_OK) {
        lua_pushnil(L);
        lua_pushinteger(L, (lua_Integer)r);
        return 2;
    }
    lua_pushlstring(L, (const char*)digest, (size_t)digest_len);
    return 1;
}

static int l_sec_hash_md5(lua_State* L)  { return l_sec_hash(L, ZSDK_HASH_MD5); }
static int l_sec_hash_sha1(lua_State* L) { return l_sec_hash(L, ZSDK_HASH_SHA1); }

static int l_sec_check_license(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    int valid = 0, expired = 0;
    uint32_t days = 0;
    zsdk_result_t r = zsdk_license_check(ctx, &valid, &expired, &days);
    lua_pushinteger(L, (lua_Integer)r);
    lua_pushboolean(L, valid);
    lua_pushboolean(L, expired);
    lua_pushinteger(L, (lua_Integer)days);
    return 4;
}

static const luaL_Reg kSecFuncs[] = {
    { "random_u32",    l_sec_random_u32    },
    { "random_bytes",  l_sec_random_bytes  },
    { "hash_md5",      l_sec_hash_md5      },
    { "hash_sha1",     l_sec_hash_sha1     },
    { "hmac_md5",      l_sec_hmac_md5      },
    { "hmac_sha1",     l_sec_hmac_sha1     },
    { "check_license", l_sec_check_license },
    { NULL,            NULL                }
};

int luaopen_zsdk_security(lua_State* L) {
    lua_newtable(L);
    zsdk_lua_register_funcs(L, kSecFuncs);
    return 1;
}
