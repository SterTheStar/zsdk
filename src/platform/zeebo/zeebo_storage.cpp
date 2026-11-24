/**
 * @file zeebo_storage.cpp
 * @brief Qualcomm BREW / Zeebo LocalStorage & RamCache bridges.
 */

#include "zsdk/fs/local_storage.hpp"
#include "zsdk/fs/ram_cache.hpp"
#include "zsdk/fs/filesystem.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEFile.h"
#include "AEERamCache.h"

namespace zsdk {
namespace fs {

/* --------------------------------------------------------------------------
 * LocalStorage (backed by BREW IFileMgr under "zsdk_kv_<key>.bin")
 * -------------------------------------------------------------------------- */
struct LocalStorage::Impl {
    IFileMgr* fileMgr;
};

LocalStorage::LocalStorage()
    : m_impl(nullptr) {
}

LocalStorage::~LocalStorage() {
    Shutdown();
}

Result LocalStorage::Initialize(void* platformContext) {
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
    m_impl->fileMgr = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_FILEMGR,
                              reinterpret_cast<void**>(&m_impl->fileMgr)) != AEE_SUCCESS || !m_impl->fileMgr) {
        Shutdown();
        return Result::IoError;
    }
    return Result::Ok;
}

void LocalStorage::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->fileMgr) {
        IFILEMGR_Release(m_impl->fileMgr);
        m_impl->fileMgr = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, m_impl);
    m_impl = nullptr;
}

static void BuildKvPath(const char* key, char* outPath, size_t maxLen) {
    SNPRINTF(outPath, maxLen, "zsdk_kv_%s.bin", key ? key : "default");
}

Result LocalStorage::Write(const char* key, const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->fileMgr || !key || !data) {
        return Result::InvalidArg;
    }
    char path[96];
    BuildKvPath(key, path, sizeof(path));

    IFile* f = IFILEMGR_OpenFile(m_impl->fileMgr, path, _OFM_CREATE | _OFM_READWRITE);
    if (!f) {
        return Result::IoError;
    }
    const uint32 w = IFILE_Write(f, data, sizeBytes);
    IFILE_Release(f);
    return (w == sizeBytes) ? Result::Ok : Result::IoError;
}

Result LocalStorage::Read(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead) {
    outBytesRead = 0u;
    if (!m_impl || !m_impl->fileMgr || !key || !outBuffer) {
        return Result::InvalidArg;
    }
    char path[96];
    BuildKvPath(key, path, sizeof(path));

    IFile* f = IFILEMGR_OpenFile(m_impl->fileMgr, path, _OFM_READ);
    if (!f) {
        return Result::NotFound;
    }
    const int32 rd = IFILE_Read(f, outBuffer, maxBytes);
    IFILE_Release(f);
    if (rd < 0) {
        return Result::IoError;
    }
    outBytesRead = static_cast<uint32_t>(rd);
    return Result::Ok;
}

Result LocalStorage::Remove(const char* key) {
    if (!m_impl || !m_impl->fileMgr || !key) {
        return Result::InvalidArg;
    }
    char path[96];
    BuildKvPath(key, path, sizeof(path));
    return (IFILEMGR_Remove(m_impl->fileMgr, path) == SUCCESS) ? Result::Ok : Result::IoError;
}

/* --------------------------------------------------------------------------
 * RamCache (wraps BREW IRamCache)
 * -------------------------------------------------------------------------- */
struct RamCache::Impl {
    IRamCache* cache;
};

RamCache::RamCache()
    : m_impl(nullptr) {
}

RamCache::~RamCache() {
    Shutdown();
}

Result RamCache::Initialize(void* platformContext, uint32_t maxBytes) {
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
    m_impl->cache = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_RAMCACHE,
                              reinterpret_cast<void**>(&m_impl->cache)) != AEE_SUCCESS || !m_impl->cache) {
        Shutdown();
        return Result::NotSupported;
    }
    IRAMCACHE_SetMaxSize(m_impl->cache, static_cast<int>(maxBytes));
    return Result::Ok;
}

void RamCache::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->cache) {
        IRAMCACHE_Release(m_impl->cache);
        m_impl->cache = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, m_impl);
    m_impl = nullptr;
}

Result RamCache::Put(const char* key, const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->cache || !key || !data) {
        return Result::InvalidArg;
    }
    const int res = IRAMCACHE_Add(m_impl->cache, key, STRLEN(key),
                                  static_cast<const char*>(data), static_cast<int>(sizeBytes));
    return (res == AEE_SUCCESS) ? Result::Ok : Result::OutOfMemory;
}

Result RamCache::Get(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outSize) {
    outSize = 0u;
    if (!m_impl || !m_impl->cache || !key || !outBuffer) {
        return Result::InvalidArg;
    }
    int len = static_cast<int>(maxBytes);
    if (IRAMCACHE_Find(m_impl->cache, key, STRLEN(key),
                       static_cast<char*>(outBuffer), &len) != AEE_SUCCESS) {
        return Result::NotFound;
    }
    outSize = static_cast<uint32_t>(len);
    return Result::Ok;
}

void RamCache::Clear() {
    if (m_impl && m_impl->cache) {
        IRAMCACHE_SetMaxSize(m_impl->cache, 0);
    }
}

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
