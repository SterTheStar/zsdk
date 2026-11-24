/**
 * @file memory_tracker.hpp
 * @brief Zero-overhead / conditionally compiled RAM & VRAM profiler per subsystem.
 *
 * Tracks allocations routed through ZSDK's memory helpers (backed by BREW MALLOC/FREE
 * or IRealloc) as well as GPU VRAM allocations (textures, VBOs, EGL surfaces).
 */

#ifndef ZSDK_DEBUG_MEMORY_TRACKER_HPP
#define ZSDK_DEBUG_MEMORY_TRACKER_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace debug {

/**
 * @brief Subsystems tracked independently in the RAM/VRAM profiler.
 */
enum class MemorySubsystem : uint8_t {
    Core = 0,
    Graphics,
    Audio,
    Filesystem,
    Input,
    Network,
    User,
    Count
};

/**
 * @brief Per-subsystem memory usage snapshot.
 */
struct SubsystemMemStats {
    uint32_t currentBytes;
    uint32_t peakBytes;
    uint32_t allocationCount;
};

/**
 * @brief Global RAM + VRAM usage snapshot for the Debug Overlay and telemetry.
 */
struct MemorySnapshot {
    uint32_t totalRamBytes;
    uint32_t peakRamBytes;
    uint32_t totalVramBytes;
    uint32_t peakVramBytes;
    uint32_t budgetBytes;       /**< Configured limit (default 32 MiB). */
    SubsystemMemStats subsystems[static_cast<size_t>(MemorySubsystem::Count)];
};

/**
 * @brief Static interface for tracking RAM and VRAM allocations across ZSDK.
 */
class MemoryTracker {
public:
    /** @brief Sets the total memory budget in bytes (default: 32 MiB). */
    static void SetBudgetBytes(uint32_t budgetBytes);

    /** @brief Returns the configured memory budget in bytes. */
    static uint32_t GetBudgetBytes();

    /** @brief Allocates @p size bytes via BREW heap and records it under @p subsystem. */
    static void* Alloc(MemorySubsystem subsystem, size_t size);

    /** @brief Reallocates a tracked block. */
    static void* Realloc(MemorySubsystem subsystem, void* ptr, size_t newSize);

    /** @brief Frees a tracked block and updates subsystem counters. */
    static void Free(MemorySubsystem subsystem, void* ptr);

    /** @brief Records a VRAM allocation (e.g. glTexImage2D / VBO upload). */
    static void RecordVramAlloc(uint32_t bytes);

    /** @brief Records a VRAM deallocation (e.g. glDeleteTextures / glDeleteBuffers). */
    static void RecordVramFree(uint32_t bytes);

    /** @brief Fills @p outSnapshot with current and peak RAM/VRAM statistics. */
    static void GetSnapshot(MemorySnapshot& outSnapshot);

    /** @brief Returns a human-readable short name for @p subsystem (e.g. "GFX"). */
    static const char* GetSubsystemName(MemorySubsystem subsystem);
};

} // namespace debug
} // namespace zsdk

#endif /* ZSDK_DEBUG_MEMORY_TRACKER_HPP */
