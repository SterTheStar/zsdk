/**
 * @file memory_tracker.cpp
 * @brief Tracked heap & VRAM allocator implementation backed by BREW MALLOC/FREE.
 */

#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO
    #include "AEEStdLib.h"
    #define ZSDK_RAW_MALLOC(sz)      MALLOC(static_cast<uint32>(sz))
    #define ZSDK_RAW_REALLOC(p, sz)  REALLOC((p), static_cast<uint32>(sz))
    #define ZSDK_RAW_FREE(p)         FREE(p)
#else
    #include <stdlib.h>
    #include <string.h>
    #define ZSDK_RAW_MALLOC(sz)      ::malloc(sz)
    #define ZSDK_RAW_REALLOC(p, sz)  ::realloc((p), (sz))
    #define ZSDK_RAW_FREE(p)         ::free(p)
#endif

namespace zsdk {
namespace debug {

namespace {

struct AllocHeader {
    uint32_t size;
    uint8_t  subsystem;
    uint8_t  reserved[3];
};

MemorySnapshot g_snapshot = {
    0, 0, 0, 0, ZSDK_DEFAULT_MEMORY_LIMIT_BYTES, {}
};

#if ZSDK_PLATFORM_WINDOWS
bool g_strictBudget = true;
#endif

bool WouldExceedStrictBudget(uint64_t additionalBytes) {
#if ZSDK_PLATFORM_WINDOWS
    if (!g_strictBudget) {
        return false;
    }
    const uint64_t used = static_cast<uint64_t>(g_snapshot.totalRamBytes) +
                          static_cast<uint64_t>(g_snapshot.totalVramBytes);
    return (used + additionalBytes) > static_cast<uint64_t>(g_snapshot.budgetBytes);
#else
    ZSDK_UNUSED(additionalBytes);
    return false;
#endif
}

} // namespace

void MemoryTracker::SetBudgetBytes(uint32_t budgetBytes) {
    g_snapshot.budgetBytes = (budgetBytes > 0u) ? budgetBytes : ZSDK_DEFAULT_MEMORY_LIMIT_BYTES;
}

uint32_t MemoryTracker::GetBudgetBytes() {
    return g_snapshot.budgetBytes;
}

void MemoryTracker::SetStrictBudget(bool enabled) {
#if ZSDK_PLATFORM_WINDOWS
    g_strictBudget = enabled;
#else
    ZSDK_UNUSED(enabled);
#endif
}

bool MemoryTracker::IsStrictBudget() {
#if ZSDK_PLATFORM_WINDOWS
    return g_strictBudget;
#else
    return false;
#endif
}

bool MemoryTracker::CanAllocateVram(uint32_t bytes) {
    return !WouldExceedStrictBudget(static_cast<uint64_t>(bytes));
}

void* MemoryTracker::Alloc(MemorySubsystem subsystem, size_t size) {
    if (size == 0) {
        return nullptr;
    }

#if ZSDK_ENABLE_MEMORY_TRACKING
    if (WouldExceedStrictBudget(static_cast<uint64_t>(size))) {
        return nullptr;
    }
    const size_t totalSize = sizeof(AllocHeader) + size;
    AllocHeader* hdr = static_cast<AllocHeader*>(ZSDK_RAW_MALLOC(totalSize));
    if (!hdr) {
        return nullptr;
    }

    const uint8_t idx = static_cast<uint8_t>(subsystem);
    hdr->size = static_cast<uint32_t>(size);
    hdr->subsystem = idx;

    if (idx < static_cast<uint8_t>(MemorySubsystem::Count)) {
        SubsystemMemStats& sub = g_snapshot.subsystems[idx];
        sub.currentBytes += hdr->size;
        sub.allocationCount += 1u;
        if (sub.currentBytes > sub.peakBytes) {
            sub.peakBytes = sub.currentBytes;
        }
    }

    g_snapshot.totalRamBytes += hdr->size;
    if (g_snapshot.totalRamBytes > g_snapshot.peakRamBytes) {
        g_snapshot.peakRamBytes = g_snapshot.totalRamBytes;
    }

    return static_cast<void*>(hdr + 1);
#else
    ZSDK_UNUSED(subsystem);
    return ZSDK_RAW_MALLOC(size);
#endif
}

void* MemoryTracker::Realloc(MemorySubsystem subsystem, void* ptr, size_t newSize) {
    if (!ptr) {
        return Alloc(subsystem, newSize);
    }
    if (newSize == 0) {
        Free(subsystem, ptr);
        return nullptr;
    }

#if ZSDK_ENABLE_MEMORY_TRACKING
    AllocHeader* oldHdr = static_cast<AllocHeader*>(ptr) - 1;
    const uint32_t oldSize = oldHdr->size;
    const uint8_t idx = oldHdr->subsystem;

    const uint64_t extraBytes = (newSize > oldSize) ? static_cast<uint64_t>(newSize - oldSize) : 0ULL;
    if (WouldExceedStrictBudget(extraBytes)) {
        return nullptr;
    }

    AllocHeader* newHdr = static_cast<AllocHeader*>(
        ZSDK_RAW_REALLOC(oldHdr, sizeof(AllocHeader) + newSize));
    if (!newHdr) {
        return nullptr;
    }

    const uint32_t uNewSize = static_cast<uint32_t>(newSize);
    newHdr->size = uNewSize;

    if (idx < static_cast<uint8_t>(MemorySubsystem::Count)) {
        SubsystemMemStats& sub = g_snapshot.subsystems[idx];
        sub.currentBytes = (sub.currentBytes >= oldSize) ? (sub.currentBytes - oldSize + uNewSize) : uNewSize;
        if (sub.currentBytes > sub.peakBytes) {
            sub.peakBytes = sub.currentBytes;
        }
    }

    g_snapshot.totalRamBytes = (g_snapshot.totalRamBytes >= oldSize)
                                   ? (g_snapshot.totalRamBytes - oldSize + uNewSize)
                                   : uNewSize;
    if (g_snapshot.totalRamBytes > g_snapshot.peakRamBytes) {
        g_snapshot.peakRamBytes = g_snapshot.totalRamBytes;
    }

    return static_cast<void*>(newHdr + 1);
#else
    ZSDK_UNUSED(subsystem);
    return ZSDK_RAW_REALLOC(ptr, newSize);
#endif
}

void MemoryTracker::Free(MemorySubsystem subsystem, void* ptr) {
    if (!ptr) {
        return;
    }

#if ZSDK_ENABLE_MEMORY_TRACKING
    ZSDK_UNUSED(subsystem);
    AllocHeader* hdr = static_cast<AllocHeader*>(ptr) - 1;
    const uint32_t oldSize = hdr->size;
    const uint8_t idx = hdr->subsystem;

    if (idx < static_cast<uint8_t>(MemorySubsystem::Count)) {
        SubsystemMemStats& sub = g_snapshot.subsystems[idx];
        sub.currentBytes = (sub.currentBytes >= oldSize) ? (sub.currentBytes - oldSize) : 0u;
        if (sub.allocationCount > 0u) {
            sub.allocationCount -= 1u;
        }
    }

    g_snapshot.totalRamBytes = (g_snapshot.totalRamBytes >= oldSize)
                                   ? (g_snapshot.totalRamBytes - oldSize)
                                   : 0u;

    ZSDK_RAW_FREE(hdr);
#else
    ZSDK_UNUSED(subsystem);
    ZSDK_RAW_FREE(ptr);
#endif
}

void MemoryTracker::RecordVramAlloc(uint32_t bytes) {
#if ZSDK_ENABLE_MEMORY_TRACKING
    g_snapshot.totalVramBytes += bytes;
    if (g_snapshot.totalVramBytes > g_snapshot.peakVramBytes) {
        g_snapshot.peakVramBytes = g_snapshot.totalVramBytes;
    }
#else
    ZSDK_UNUSED(bytes);
#endif
}

void MemoryTracker::RecordVramFree(uint32_t bytes) {
#if ZSDK_ENABLE_MEMORY_TRACKING
    g_snapshot.totalVramBytes = (g_snapshot.totalVramBytes >= bytes)
                                    ? (g_snapshot.totalVramBytes - bytes)
                                    : 0u;
#else
    ZSDK_UNUSED(bytes);
#endif
}

void MemoryTracker::GetSnapshot(MemorySnapshot& outSnapshot) {
    outSnapshot = g_snapshot;
}

const char* MemoryTracker::GetSubsystemName(MemorySubsystem subsystem) {
    switch (subsystem) {
        case MemorySubsystem::Core:       return "CORE";
        case MemorySubsystem::Graphics:   return "GFX";
        case MemorySubsystem::Audio:      return "AUD";
        case MemorySubsystem::Filesystem: return "FS";
        case MemorySubsystem::Input:      return "INP";
        case MemorySubsystem::Network:    return "NET";
        case MemorySubsystem::User:       return "USR";
        default:                          return "UNK";
    }
}

} // namespace debug
} // namespace zsdk
