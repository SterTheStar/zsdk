/**
 * @file zsdk_c_core.h
 * @brief C99 ABI for ZSDK clock, memory telemetry, system info, and debug overlay.
 */

#ifndef ZSDK_C_CORE_H
#define ZSDK_C_CORE_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_sys_mem_info_t {
    uint32_t free_ram_bytes;
    uint32_t total_ram_bytes;
    uint32_t largest_free_block_bytes;
    uint32_t free_fs_bytes;
    uint32_t total_fs_bytes;
} zsdk_sys_mem_info_t;

/** @brief Monotonic milliseconds since boot/start (Clock::GetUptimeMs). */
ZSDK_C_API uint32_t zsdk_clock_uptime_ms(void);

/** @brief Wall-clock milliseconds (Clock::GetWallTimeMs). */
ZSDK_C_API uint32_t zsdk_clock_wall_ms(void);

/** @brief Current tracked RAM usage in bytes. */
ZSDK_C_API uint32_t zsdk_memory_ram_used(void);

/** @brief Current tracked VRAM usage in bytes. */
ZSDK_C_API uint32_t zsdk_memory_vram_used(void);

/** @brief Configured total memory budget in bytes. */
ZSDK_C_API uint32_t zsdk_memory_budget(void);

/** @brief Queries BREW/OS system RAM & FS free space. */
ZSDK_C_API zsdk_result_t zsdk_system_get_memory_info(zsdk_sys_mem_info_t* out_info);

/** @brief Queries device language code. */
ZSDK_C_API uint32_t zsdk_system_get_language_code(void* platform_ctx);

/** @brief Shows or hides the in-game DebugOverlay. */
ZSDK_C_API void zsdk_debug_overlay_set_visible(int visible);

/** @brief Returns non-zero if the in-game DebugOverlay is visible. */
ZSDK_C_API int zsdk_debug_overlay_is_visible(void);

/** @brief Records a draw call (and optional vertex count) in DebugOverlay telemetry. */
ZSDK_C_API void zsdk_debug_overlay_record_draw_call(uint32_t vertex_count);

/** @brief Sets the application FPS cap (30 or 60). */
ZSDK_C_API void zsdk_app_set_fps_cap(uint8_t fps);

/** @brief Requests clean application shutdown at the end of the current frame. */
ZSDK_C_API void zsdk_app_request_quit(void);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_CORE_H */
