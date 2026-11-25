# Optional Lua C bindings target (built when ZSDK_BUILD_LUA_BINDINGS=ON and ZSDK_LUA_INCLUDE_DIR is set)
set(ZSDK_LUA_INCLUDE_DIR "" CACHE PATH "Path to Lua headers (lua.h, lauxlib.h)")

if(ZSDK_LUA_INCLUDE_DIR)
    add_library(zsdk_lua STATIC
        src/zsdk_lua.c
        src/zsdk_lua_log.c
        src/zsdk_lua_core.c
        src/zsdk_lua_input.c
        src/zsdk_lua_audio.c
        src/zsdk_lua_graphics.c
        src/zsdk_lua_fs.c
        src/zsdk_lua_net.c
        src/zsdk_lua_security.c
    )

    target_include_directories(zsdk_lua
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${ZSDK_LUA_INCLUDE_DIR}"
    )

    target_link_libraries(zsdk_lua PUBLIC zsdk)
else()
    message(STATUS "ZSDK Lua bindings enabled, but ZSDK_LUA_INCLUDE_DIR is not set; skipping zsdk_lua target.")
endif()
