Lua 5.4.7 (https://www.lua.org) - MIT License, (C) 1994-2024 Lua.org, PUC-Rio.
Vendored as-is except luaconf.h (LUA_32BITS wrapped in #if !defined so CMake can set it): core + base/table/string/math/utf8/coroutine libs only.
Omitted: lua.c, luac.c, linit.c, liolib.c, loslib.c, loadlib.c, ldblib.c (no io/os/package/debug on Zeebo).
Built with LUA_32BITS (float + int32) and LUA_USE_C89 - see CMakeLists.txt.
