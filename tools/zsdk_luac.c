/**
 * @file zsdk_luac.c
 * @brief Minimal host Lua 5.4 bytecode compiler for ZSDK (built with LUA_32BITS=1
 *        so dumped .luac files match Zeebo ARM32 and Windows 32-bit Lua VM).
 *
 * Usage: zsdk_luac [-s] <input.lua> <output.luac>
 */

#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

static int dump_writer(lua_State* L, const void* p, size_t sz, void* ud) {
    FILE* f = (FILE*)ud;
    (void)L;
    if (sz == 0) {
        return 0;
    }
    return (fwrite(p, 1, sz, f) == sz) ? 0 : 1;
}

int main(int argc, char** argv) {
    int strip = 1;
    const char* in_path = NULL;
    const char* out_path = NULL;
    int i;
    lua_State* L;
    FILE* out;
    int rc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-s") == 0) {
            strip = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            strip = 0;
        } else if (!in_path) {
            in_path = argv[i];
        } else if (!out_path) {
            out_path = argv[i];
        } else {
            fprintf(stderr, "Usage: %s [-s|-g] <input.lua> <output.luac>\n", argv[0]);
            return 1;
        }
    }

    if (!in_path || !out_path) {
        fprintf(stderr, "Usage: %s [-s|-g] <input.lua> <output.luac>\n", argv[0]);
        return 1;
    }

    L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "zsdk_luac: out of memory\n");
        return 1;
    }

    if (luaL_loadfile(L, in_path) != LUA_OK) {
        fprintf(stderr, "zsdk_luac: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

#ifdef _MSC_VER
    out = NULL;
    if (fopen_s(&out, out_path, "wb") != 0 || !out) {
#else
    out = fopen(out_path, "wb");
    if (!out) {
#endif
        fprintf(stderr, "zsdk_luac: cannot open output '%s'\n", out_path);
        lua_close(L);
        return 1;
    }

    rc = lua_dump(L, dump_writer, out, strip);
    fclose(out);
    lua_close(L);

    if (rc != 0) {
        fprintf(stderr, "zsdk_luac: failed to write '%s'\n", out_path);
        return 1;
    }
    return 0;
}
