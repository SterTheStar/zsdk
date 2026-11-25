# ==============================================================================
# ZsdkBrew.cmake - Helper for building Qualcomm BREW / Zeebo .mod + .mif apps
# ==============================================================================
#
# Usage:
#   include(cmake/ZsdkBrew.cmake)
#   zsdk_add_brew_app(my_game
#       MIF_CLASS_ID 0x01099001
#       SOURCES src/main.cpp
#   )
#
# This automatically:
#   1. Includes BREW CRT / extension glue (third_party/brew_crt/*.c)
#   2. Links with tools/elf2mod/elf2mod.x and -Wl,--emit-relocs
#   3. Runs tools/elf2mod/elf2mod.exe to produce <target>.mod
#   4. Configures tools/mif/zsdk_template.mif.in and compiles/copies <target>.mif
# ==============================================================================

set(ZSDK_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

function(zsdk_add_brew_app TARGET_NAME)
    cmake_parse_arguments(ARG "" "MIF_CLASS_ID;MIF_APP_NAME;MIF_COMPANY;MIF_COPYRIGHT;MIF_VERSION" "SOURCES" ${ARGN})

    if(ZSDK_BUILD_ZEEBO)
        set(BREW_CRT_SOURCES
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/AEEModGen.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/AEEAppGen.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/AEEHIDButtons.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/AEEHIDThumbsticks.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/AEEMediaUtil.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/EGL_1x.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/GLES_1x.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/glibc_stubs.c"
            "${ZSDK_ROOT_DIR}/third_party/brew_crt/mod_malloc.c"
        )

        add_executable(${TARGET_NAME} ${ARG_SOURCES} ${BREW_CRT_SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES
            SUFFIX ".elf"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/${TARGET_NAME}"
        )

        target_include_directories(${TARGET_NAME} PRIVATE
            "${BREW_SDK_ROOT}/platform_402/BREW 4.0.2 SP19/sdk/inc"
            "${BREW_SDK_ROOT}/platform_402/BREW 4.0.2 SP19/inc"
            "${BREW_SDK_ROOT}/ogles_ext/unpacked/sdk/inc"
            "${BREW_SDK_ROOT}/zeebo_msi_extracted/BREW402SP09 IHID Extension Folder/sdk/inc"
        )

        target_compile_definitions(${TARGET_NAME} PRIVATE DYNAMIC_APP)
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics>
            -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -mlong-calls
        )

        target_link_libraries(${TARGET_NAME} PRIVATE zsdk -lm -lc -lgcc)
        target_link_options(${TARGET_NAME} PRIVATE
            -T "${ZSDK_ROOT_DIR}/tools/elf2mod/elf2mod.x"
            -nostartfiles
            --specs=nosys.specs
            -Wl,--gc-sections
            -Wl,--emit-relocs
            -Wl,--strip-debug
            -Wl,-e,AEEMod_Load
        )

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/mod/${TARGET_NAME}"
            COMMAND "${ZSDK_ROOT_DIR}/tools/elf2mod/elf2mod.exe"
                    -output "$<TARGET_FILE_DIR:${TARGET_NAME}>/mod/${TARGET_NAME}/${TARGET_NAME}.mod"
                    "$<TARGET_FILE:${TARGET_NAME}>"
            COMMENT "Packaging mod/${TARGET_NAME}/${TARGET_NAME}.mod via elf2mod"
        )

        # ----------------------------------------------------------------------
        # Module Information File (.mif) generation
        # ----------------------------------------------------------------------
        if(ARG_MIF_CLASS_ID)
            set(ZSDK_MIF_CLASS_ID "${ARG_MIF_CLASS_ID}")
        else()
            set(ZSDK_MIF_CLASS_ID "0x01099001")
        endif()

        if(ARG_MIF_APP_NAME)
            set(ZSDK_MIF_APP_NAME "${ARG_MIF_APP_NAME}")
        else()
            set(ZSDK_MIF_APP_NAME "${TARGET_NAME}")
        endif()

        if(ARG_MIF_COMPANY)
            set(ZSDK_MIF_COMPANY "${ARG_MIF_COMPANY}")
        else()
            set(ZSDK_MIF_COMPANY "ZSDK")
        endif()

        if(ARG_MIF_COPYRIGHT)
            set(ZSDK_MIF_COPYRIGHT "${ARG_MIF_COPYRIGHT}")
        else()
            set(ZSDK_MIF_COPYRIGHT "ZSDK")
        endif()

        if(ARG_MIF_VERSION)
            set(ZSDK_MIF_VERSION "${ARG_MIF_VERSION}")
        else()
            set(ZSDK_MIF_VERSION "1.0.0")
        endif()

        set(ZSDK_GENERATED_CIF "${CMAKE_BINARY_DIR}/apps/${TARGET_NAME}/${TARGET_NAME}.cif")
        configure_file(
            "${ZSDK_ROOT_DIR}/tools/mif/zsdk_template.mif.in"
            "${ZSDK_GENERATED_CIF}"
            @ONLY
        )

        find_program(ZSDK_BREW_CIFC
            NAMES cifc cifc.exe
            HINTS
                "${BREW_SDK_ROOT}/toolset/Toolset 7.10 Rev 12.0.1549664/bin"
                "${BREW_SDK_ROOT}/platform_402/BREW 4.0.2 SP19/bin"
        )

        if(ZSDK_BREW_CIFC)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/mif"
                COMMAND "${ZSDK_BREW_CIFC}"
                        -mif "$<TARGET_FILE_DIR:${TARGET_NAME}>/mif/${TARGET_NAME}.mif"
                        "${ZSDK_GENERATED_CIF}"
                COMMENT "Packaging mif/${TARGET_NAME}.mif via cifc (${ZSDK_MIF_CLASS_ID})"
            )
        else()
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/mif"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${ZSDK_GENERATED_CIF}"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>/mif/${TARGET_NAME}.mif"
                COMMENT "Copying configured mif/${TARGET_NAME}.mif (cifc not found)"
            )
        endif()
    else()
        add_executable(${TARGET_NAME} ${ARG_SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/${TARGET_NAME}"
        )
        target_link_libraries(${TARGET_NAME} PRIVATE zsdk opengl32 user32 gdi32 winmm)
    endif()
endfunction()

# Builds a pure-Lua ZSDK application using the built-in zsdk_lua_host.cpp (no C/C++ boilerplate needed).
function(zsdk_add_lua_app TARGET_NAME)
    cmake_parse_arguments(ARG "" "SCRIPT;MIF_CLASS_ID;MIF_APP_NAME;MIF_COMPANY;MIF_COPYRIGHT;MIF_VERSION" "" ${ARGN})
    if(NOT TARGET zsdk_lua)
        return()
    endif()

    zsdk_add_brew_app(${TARGET_NAME}
        SOURCES "${ZSDK_ROOT_DIR}/bindings/lua/host/zsdk_lua_host.cpp"
        MIF_CLASS_ID "${ARG_MIF_CLASS_ID}"
        MIF_APP_NAME "${ARG_MIF_APP_NAME}"
        MIF_COMPANY "${ARG_MIF_COMPANY}"
        MIF_COPYRIGHT "${ARG_MIF_COPYRIGHT}"
        MIF_VERSION "${ARG_MIF_VERSION}"
    )

    if(ARG_MIF_APP_NAME)
        target_compile_definitions(${TARGET_NAME} PRIVATE ZSDK_LUA_APP_NAME="${ARG_MIF_APP_NAME}")
    endif()
    target_link_libraries(${TARGET_NAME} PRIVATE zsdk_lua)

    if(ARG_SCRIPT)
        get_filename_component(_SCRIPT_ABS "${ARG_SCRIPT}" ABSOLUTE)
        if(ZSDK_BUILD_ZEEBO)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_SCRIPT_ABS}"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>/mod/${TARGET_NAME}/main.lua"
            )
        else()
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_SCRIPT_ABS}"
                        "$<TARGET_FILE_DIR:${TARGET_NAME}>/main.lua"
            )
        endif()
    endif()
endfunction()
