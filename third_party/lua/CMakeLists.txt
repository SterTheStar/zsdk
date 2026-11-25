file(GLOB LUA_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(lua54 STATIC ${LUA_CORE_SOURCES})
target_include_directories(lua54 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(ZSDK_BUILD_ZEEBO)
    # 32-bit ARM11 / Zeebo: use 32-bit Lua integers/floats and no POSIX popen/tmpnam
    target_compile_definitions(lua54 PUBLIC LUA_32BITS=1)
    target_compile_options(lua54 PRIVATE -O2 -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -mlong-calls)
endif()
