/**
 * @file zeebo_unzip.cpp
 * @brief Qualcomm BREW / Zeebo IUnzipStream implementation.
 */

#include "zsdk/fs/archive_unzip.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEUnzipStream.h"

namespace zsdk {
namespace fs {

struct UnzipStream::Impl {
    IUnzipAStream* unzip;
};

UnzipStream::UnzipStream()
    : m_impl(nullptr) {
}

UnzipStream::~UnzipStream() {
    Shutdown();
}

Result UnzipStream::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Filesystem, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->unzip = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_UNZIPSTREAM,
                              reinterpret_cast<void**>(&m_impl->unzip)) != AEE_SUCCESS || !m_impl->unzip) {
        ZSDK_LOG_ERROR("FS", "Failed to create AEECLSID_UNZIPSTREAM");
        Shutdown();
        return Result::NotSupported;
    }

    return Result::Ok;
}

void UnzipStream::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->unzip) {
        IUNZIPASTREAM_Release(m_impl->unzip);
        m_impl->unzip = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, m_impl);
    m_impl = nullptr;
}

Result UnzipStream::SetInputStream(void* nativeIAStream) {
    if (!m_impl || !m_impl->unzip || !nativeIAStream) {
        return Result::InvalidArg;
    }
    IUNZIPASTREAM_SetStream(m_impl->unzip, static_cast<IAStream*>(nativeIAStream));
    return Result::Ok;
}

int32_t UnzipStream::Read(void* outBuffer, uint32_t bytes) {
    if (!m_impl || !m_impl->unzip || !outBuffer || bytes == 0u) {
        return -1;
    }
    return IUNZIPASTREAM_Read(m_impl->unzip, outBuffer, bytes);
}

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
