cmake_minimum_required(VERSION 3.16)
project(ZSDK LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ZSDK_ENABLE_DEBUG_OVERLAY "Enable in-game OpenGL ES Debug & RAM/VRAM Overlay" ON)
option(ZSDK_ENABLE_LOGGING "Enable ZSDK_LOG_INFO and ZSDK_LOG_WARN logging" ON)
option(ZSDK_BUILD_ZEEBO "Build for Zeebo / Qualcomm BREW 4.0.2" ON)
option(ZSDK_BUILD_EXAMPLES "Build ZSDK sample applications" ON)

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

if(ZSDK_BUILD_EXAMPLES)
    zsdk_add_brew_app(01_spinning_cube
        SOURCES examples/01_spinning_cube/main.cpp
    )
endif()
