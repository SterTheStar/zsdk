cmake_minimum_required(VERSION 3.16)
project(ZSDK LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ZSDK_ENABLE_DEBUG_OVERLAY "Enable in-game OpenGL ES Debug & RAM/VRAM Overlay" ON)
option(ZSDK_ENABLE_LOGGING "Enable ZSDK_LOG_INFO and ZSDK_LOG_WARN logging" ON)
option(ZSDK_BUILD_ZEEBO "Build for Zeebo / Qualcomm BREW 4.0.2" ON)
option(ZSDK_BUILD_EXAMPLES "Build ZSDK sample applications" ON)
option(ZSDK_BUILD_LUA_BINDINGS "Build modular Lua C bindings" ON)
option(ZSDK_ENABLE_TESTS "Enable C ABI self-verification tests and bindings parity check" ON)

set(BREW_SDK_ROOT "C:/Users/Esther/AppData/Local/Programs/brew-sdk" CACHE PATH "Path to BREW/Zeebo SDK")

set(ZSDK_SOURCES
    src/core/app.cpp
    src/debug/embedded_font.cpp
    src/debug/memory_tracker.cpp
    src/debug/logger.cpp
    src/debug/debug_overlay.cpp
    src/input/input.cpp
    src/input/virtual_keyboard.cpp
    src/graphics/texture.cpp
    src/graphics/mesh_buffer.cpp
    src/graphics/renderer.cpp
    src/graphics/renderer_lighting.cpp
    src/graphics/renderer_state.cpp
    src/graphics/renderer_2d_vbo.cpp
    src/graphics/sprite_batch.cpp
    src/graphics/frustum.cpp
    src/audio/audio_engine.cpp
    src/audio/pcm_mixer.cpp
    src/c/zsdk_c_log.cpp
    src/c/zsdk_c_core.cpp
    src/c/zsdk_c_input.cpp
    src/c/zsdk_c_audio.cpp
    src/c/zsdk_c_graphics.cpp
    src/c/zsdk_c_fs.cpp
    src/c/zsdk_c_net.cpp
    src/c/zsdk_c_security.cpp
    src/fs/savegame.cpp
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

add_library(zsdk STATIC ${ZSDK_SOURCES})

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

if(ZSDK_ENABLE_LOGGING)
    target_compile_definitions(zsdk PUBLIC ZSDK_ENABLE_LOGGING=1)
else()
    target_compile_definitions(zsdk PUBLIC ZSDK_ENABLE_LOGGING=0)
endif()

if(MSVC)
    target_compile_options(zsdk PRIVATE /W4 /GR- /EHs-c-)
else()
    target_compile_options(zsdk PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics>
        -Wall -Wextra -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -mlong-calls
    )
endif()

include(cmake/ZsdkBrew.cmake)

if(ZSDK_BUILD_LUA_BINDINGS)
    add_subdirectory(bindings/lua)
endif()

if(ZSDK_BUILD_EXAMPLES)
    zsdk_add_brew_app(01_spinning_cube
        SOURCES examples/01_spinning_cube/main.cpp
    )
    zsdk_add_brew_app(02_c_spinning_cube
        SOURCES
            examples/02_c_spinning_cube/main.cpp
            examples/02_c_spinning_cube/cube_c.c
        MIF_CLASS_ID 0x01099002
        MIF_APP_NAME "ZSDK_C_Cube"
    )
    zsdk_add_lua_app(03_lua_spinning_cube
        SCRIPT examples/03_lua_spinning_cube/main.lua
        MIF_CLASS_ID 0x01099003
        MIF_APP_NAME "ZSDK_Lua_Cube"
    )
endif()

if(ZSDK_ENABLE_TESTS)
    enable_testing()

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_custom_target(zsdk_check_bindings ALL
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_bindings.py"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Verifying ZSDK C ABI & Lua bindings parity"
        )
        add_test(NAME zsdk_bindings_parity
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_bindings.py"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()

    add_library(zsdk_c_abi_compile_test OBJECT tests/c_abi_smoke_test.c)
    set_target_properties(zsdk_c_abi_compile_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_include_directories(zsdk_c_abi_compile_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()
