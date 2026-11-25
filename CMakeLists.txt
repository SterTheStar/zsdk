cmake_minimum_required(VERSION 3.16)
project(ZSDK LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ZSDK_ENABLE_DEBUG_OVERLAY "Enable in-game OpenGL ES Debug & RAM/VRAM Overlay" ON)
option(ZSDK_BUILD_ZEEBO "Build for Zeebo / Qualcomm BREW 4.0.2" ON)
option(ZSDK_ENABLE_LUA "Embed the Lua 5.4 scripting VM (zsdk::script::LuaVM)" ON)
option(ZSDK_BUILD_EXAMPLES "Build ZSDK sample applications" ON)

set(BREW_SDK_ROOT "C:/Users/Esther/AppData/Local/Programs/brew-sdk" CACHE PATH "Path to BREW/Zeebo SDK")

set(ZSDK_SOURCES
    src/core/app.cpp
    src/debug/embedded_font.cpp
    src/debug/memory_tracker.cpp
    src/debug/logger.cpp
    src/debug/debug_overlay.cpp
    src/input/input.cpp
    src/graphics/texture.cpp
    src/graphics/mesh_buffer.cpp
    src/graphics/renderer.cpp
    src/graphics/renderer_lighting.cpp
    src/graphics/renderer_state.cpp
    src/graphics/renderer_2d_vbo.cpp
    src/graphics/sprite_batch.cpp
    src/graphics/frustum.cpp
    src/graphics/gl_extensions.cpp
    src/audio/audio_engine.cpp
    src/fs/savegame.cpp
    src/fs/vfs.cpp
    src/debug/file_log.cpp
)

if(ZSDK_BUILD_ZEEBO)
    list(APPEND ZSDK_SOURCES
        src/platform/zeebo/zeebo_core.cpp
        src/platform/zeebo/zeebo_signal.cpp
        src/platform/zeebo/zeebo_system_info.cpp
        src/platform/zeebo/zeebo_thread.cpp
        src/platform/zeebo/zeebo_input.cpp
        src/platform/zeebo/zeebo_keyboard_mouse.cpp
        src/platform/zeebo/zeebo_gl_context.cpp
        src/platform/zeebo/zeebo_display2d.cpp
        src/platform/zeebo/zeebo_image_decoder.cpp
        src/platform/zeebo/zeebo_sprite2d.cpp
        src/platform/zeebo/zeebo_fs.cpp
        src/platform/zeebo/zeebo_storage.cpp
        src/platform/zeebo/zeebo_database.cpp
        src/platform/zeebo/zeebo_audio.cpp
        src/platform/zeebo/zeebo_unzip.cpp
        src/platform/zeebo/zeebo_socket.cpp
        src/platform/zeebo/zeebo_net_extra.cpp
        src/platform/zeebo/zeebo_crypto.cpp
        src/platform/zeebo/zeebo_security_extra.cpp
    )
else()
    list(APPEND ZSDK_SOURCES
        src/platform/windows/windows_backend.cpp
    )
endif()

if(ZSDK_ENABLE_LUA)
    list(APPEND ZSDK_SOURCES src/script/lua_vm.cpp)
    file(GLOB ZSDK_LUA_SOURCES CONFIGURE_DEPENDS third_party/lua/*.c)
    add_library(zsdk_lua STATIC ${ZSDK_LUA_SOURCES})
    target_include_directories(zsdk_lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua)
    # LUAI_MAXCCALLS: Zeebo gives apps only 32 KB of stack; default 200 nested C calls could overflow it.
    target_compile_definitions(zsdk_lua PUBLIC LUA_32BITS=1 LUA_USE_C89=1 LUAI_MAXCCALLS=48)
    if(NOT MSVC)
        target_compile_options(zsdk_lua PRIVATE -O2 -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -w)
        if(ZSDK_BUILD_ZEEBO)
            target_compile_options(zsdk_lua PRIVATE -mlong-calls)
        endif()
    endif()
endif()

add_library(zsdk STATIC ${ZSDK_SOURCES})
if(ZSDK_ENABLE_LUA)
    target_link_libraries(zsdk PUBLIC zsdk_lua)
endif()

target_include_directories(zsdk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ZSDK_BUILD_ZEEBO)
    target_compile_definitions(zsdk PUBLIC ZSDK_PLATFORM_ZEEBO=1 ZSDK_PLATFORM_WINDOWS=0)
    target_include_directories(zsdk PRIVATE
        "${BREW_SDK_ROOT}/platform_402/BREW 4.0.2 SP19/sdk/inc"
        "${BREW_SDK_ROOT}/platform_402/BREW 4.0.2 SP19/inc"
        "${BREW_SDK_ROOT}/ogles_ext/unpacked/sdk/inc"
        "${BREW_SDK_ROOT}/zeebo_msi_extracted/BREW402SP09 IHID Extension Folder/sdk/inc"
    )
else()
    target_compile_definitions(zsdk PUBLIC ZSDK_PLATFORM_ZEEBO=0 ZSDK_PLATFORM_WINDOWS=1)
endif()

if(ZSDK_ENABLE_DEBUG_OVERLAY)
    target_compile_definitions(zsdk PUBLIC ZSDK_ENABLE_DEBUG_OVERLAY=1)
else()
    target_compile_definitions(zsdk PUBLIC ZSDK_ENABLE_DEBUG_OVERLAY=0)
endif()

if(MSVC)
    target_compile_options(zsdk PRIVATE /W4 /GR- /EHs-c-)
else()
    target_compile_options(zsdk PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections)
    if(ZSDK_BUILD_ZEEBO)
        target_compile_options(zsdk PRIVATE -mlong-calls)   # ARM-only flag
    endif()
endif()

include(cmake/ZsdkBrew.cmake)

if(ZSDK_BUILD_EXAMPLES)
    zsdk_add_brew_app(01_spinning_cube
        SOURCES examples/01_spinning_cube/main.cpp
    )
    if(ZSDK_ENABLE_LUA)
        # Embed scripts/star_catcher.lua as the fallback used when script:/ has no copy.
        set(ZSDK_DEMO_LUA "${CMAKE_CURRENT_SOURCE_DIR}/examples/02_lua_demo/scripts/star_catcher.lua")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ZSDK_DEMO_LUA}")
        file(READ "${ZSDK_DEMO_LUA}" ZSDK_DEMO_LUA_TEXT)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/star_catcher_lua.h"
             "static const char kEmbeddedScript[] = R\"LUA(${ZSDK_DEMO_LUA_TEXT})LUA\";\n")
        zsdk_add_brew_app(02_lua_demo
            SOURCES examples/02_lua_demo/main.cpp
        )
        target_include_directories(02_lua_demo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gen")
    endif()
endif()
