# Lua C bindings target (auto-uses zsdk_lua_runtime or vendored third_party/lua if ZSDK_LUA_INCLUDE_DIR is not set)
set(ZSDK_LUA_INCLUDE_DIR "" CACHE PATH "Path to Lua headers (lua.h, lauxlib.h)")

set(_ZSDK_LUA_SOURCES
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

if(ZSDK_LUA_INCLUDE_DIR)
    add_library(zsdk_lua STATIC ${_ZSDK_LUA_SOURCES})
    target_include_directories(zsdk_lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${ZSDK_LUA_INCLUDE_DIR}")
    target_link_libraries(zsdk_lua PUBLIC zsdk)
elseif(TARGET zsdk_lua_runtime)
    add_library(zsdk_lua STATIC ${_ZSDK_LUA_SOURCES})
    target_include_directories(zsdk_lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(zsdk_lua PUBLIC zsdk zsdk_lua_runtime)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/lua/lua.h")
    if(NOT TARGET lua54)
        add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/lua" "${CMAKE_BINARY_DIR}/third_party/lua")
    endif()
    add_library(zsdk_lua STATIC ${_ZSDK_LUA_SOURCES})
    target_include_directories(zsdk_lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(zsdk_lua PUBLIC zsdk lua54)
else()
    message(STATUS "ZSDK Lua bindings enabled, but no Lua headers/runtime found; skipping zsdk_lua.")
endif()

if(TARGET zsdk_lua AND ZSDK_BUILD_ZEEBO)
    target_compile_options(zsdk_lua PRIVATE -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -mlong-calls)
endif()
