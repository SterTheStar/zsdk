/**
 * @file zsdk_c_core.cpp
 * @brief C ABI bridge for Clock, MemoryTracker, SystemInfo, and DebugOverlay.
 */

#include "zsdk/c/zsdk_c_core.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/core/app.hpp"
#include "zsdk/core/clock.hpp"
#include "zsdk/core/system_info.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/memory_tracker.hpp"

extern "C" {

uint32_t zsdk_clock_uptime_ms(void) {
    return zsdk::Clock::GetUptimeMs();
}

uint32_t zsdk_clock_wall_ms(void) {
    return zsdk::Clock::GetWallTimeMs();
}

uint32_t zsdk_memory_ram_used(void) {
    zsdk::debug::MemorySnapshot snap;
    zsdk::debug::MemoryTracker::GetSnapshot(snap);
    return snap.totalRamBytes;
}

uint32_t zsdk_memory_vram_used(void) {
    zsdk::debug::MemorySnapshot snap;
    zsdk::debug::MemoryTracker::GetSnapshot(snap);
    return snap.totalVramBytes;
}

uint32_t zsdk_memory_budget(void) {
    return zsdk::debug::MemoryTracker::GetBudgetBytes();
}

zsdk_result_t zsdk_system_get_memory_info(zsdk_sys_mem_info_t* out_info) {
    if (!out_info) {
        return ZSDK_C_ERR_INVALID_ARG;
    }
    zsdk::SystemMemoryInfo info;
    zsdk::Result r = zsdk::SystemInfo::GetMemoryInfo(info);
    if (zsdk::Succeeded(r)) {
        out_info->free_ram_bytes           = info.freeRamBytes;
        out_info->total_ram_bytes          = info.totalRamBytes;
        out_info->largest_free_block_bytes = info.largestFreeBlockBytes;
        out_info->free_fs_bytes            = info.freeFsBytes;
        out_info->total_fs_bytes           = info.totalFsBytes;
    }
    return zsdk::c_abi::ToCResult(r);
}

uint32_t zsdk_system_get_language_code(void* platform_ctx) {
    return zsdk::SystemInfo::GetLanguageCode(platform_ctx);
}

void zsdk_debug_overlay_set_visible(int visible) {
    zsdk::debug::DebugOverlay::SetVisible(visible != 0);
}

int zsdk_debug_overlay_is_visible(void) {
    return zsdk::debug::DebugOverlay::IsVisible() ? 1 : 0;
}

void zsdk_debug_overlay_record_draw_call(uint32_t vertex_count) {
    zsdk::debug::DebugOverlay::RecordDrawCall(vertex_count);
}

void zsdk_app_set_fps_cap(uint8_t fps) {
    if (zsdk::Application* app = zsdk::Application::GetInstance()) {
        app->SetFpsCap(fps <= 30u ? zsdk::FpsCap::Fps30 : zsdk::FpsCap::Fps60);
    }
}

void zsdk_app_request_quit(void) {
    if (zsdk::Application* app = zsdk::Application::GetInstance()) {
        app->RequestQuit();
    }
}

} /* extern "C" */
