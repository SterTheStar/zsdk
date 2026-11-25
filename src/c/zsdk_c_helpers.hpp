/**
 * @file zsdk_c_helpers.hpp
 * @brief Internal shared helpers for ZSDK C ABI bridges (no RTTI/exceptions, MemoryTracker-backed).
 */

#ifndef ZSDK_C_HELPERS_HPP
#define ZSDK_C_HELPERS_HPP

#include "zsdk/c/zsdk_c_types.h"
#include "zsdk/debug/memory_tracker.hpp"

#include <new>

namespace zsdk {
namespace c_abi {

inline zsdk_result_t ToCResult(Result r) {
    return static_cast<zsdk_result_t>(static_cast<int32_t>(r));
}

template <typename T>
T* Alloc(debug::MemorySubsystem subsys) {
    void* mem = debug::MemoryTracker::Alloc(subsys, sizeof(T));
    if (!mem) {
        return nullptr;
    }
    return new (mem) T();
}

template <typename T, typename Arg>
T* AllocArg(debug::MemorySubsystem subsys, Arg arg) {
    void* mem = debug::MemoryTracker::Alloc(subsys, sizeof(T));
    if (!mem) {
        return nullptr;
    }
    return new (mem) T(arg);
}

template <typename T>
void Free(debug::MemorySubsystem subsys, T* obj) {
    if (!obj) {
        return;
    }
    obj->~T();
    debug::MemoryTracker::Free(subsys, obj);
}

} // namespace c_abi
} // namespace zsdk

#endif /* ZSDK_C_HELPERS_HPP */
