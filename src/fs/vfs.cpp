/**
 * @file vfs.cpp
 * @brief Mount-prefix virtual filesystem implementation.
 */

#include "zsdk/fs/vfs.hpp"
#include "zsdk/debug/logger.hpp"

#include <string.h>

namespace zsdk {
namespace fs {

namespace {

/* Returns length of "xyz:" prefix at the start of @p path, or 0 if there is none. */
size_t PrefixLength(const char* path) {
    for (size_t i = 0; path[i] != '\0' && i <= Vfs::kMaxPrefixLen; ++i) {
        const char c = path[i];
        if (c == ':') {
            return i + 1u;
        }
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) {
            return 0u;
        }
    }
    return 0u;
}

bool HasDotDot(const char* p) {
    while (*p != '\0') {
        while (*p == '/' || *p == '\\') ++p;
        if (p[0] == '.' && p[1] == '.' && (p[2] == '\0' || p[2] == '/' || p[2] == '\\')) {
            return true;
        }
        while (*p != '\0' && *p != '/' && *p != '\\') ++p;
    }
    return false;
}

} // namespace

Vfs::Vfs() : m_initialized(false), m_useSdCard(false) {
    for (uint8_t i = 0u; i < kMaxMounts; ++i) {
        m_mounts[i].used = false;
        m_mounts[i].prefix[0] = '\0';
        m_mounts[i].root[0] = '\0';
    }
}

Vfs::~Vfs() { Shutdown(); }

Result Vfs::Initialize(void* platformContext, bool preferSdCard) {
    if (m_initialized) return Result::AlreadyInitialized;
    Result r = m_fs.Initialize(platformContext);
    if (Failed(r)) return r;

#if ZSDK_PLATFORM_ZEEBO
    m_useSdCard = preferSdCard && m_fs.Exists("fs:/card0/");
    Mount("home:", "fs:/~/");
    Mount("script:", "fs:/~/scripts/");
    if (m_useSdCard) {
        Mount("sd:", "fs:/card0/");
        m_fs.MkDir("fs:/card0/udata");
        Mount("udata:", "fs:/card0/udata/");
    } else {
        m_fs.MkDir("fs:/~/udata");
        Mount("udata:", "fs:/~/udata/");
    }
#else
    ZSDK_UNUSED(preferSdCard);
    m_useSdCard = false;
    Mount("home:", "./");
    Mount("script:", "scripts/");
    m_fs.MkDir("udata");
    Mount("udata:", "udata/");
#endif

    m_initialized = true;
    return Result::Ok;
}

void Vfs::Shutdown() {
    if (!m_initialized) return;
    for (uint8_t i = 0u; i < kMaxMounts; ++i) m_mounts[i].used = false;
    m_fs.Shutdown();
    m_initialized = false;
    m_useSdCard = false;
}

Result Vfs::Mount(const char* prefix, const char* root) {
    if (!prefix || !root) return Result::InvalidArg;
    const size_t pl = strlen(prefix);
    const size_t rl = strlen(root);
    if (pl < 2u || pl > kMaxPrefixLen || prefix[pl - 1u] != ':' || PrefixLength(prefix) != pl) return Result::InvalidArg;
    if (rl > kMaxRootLen) return Result::InvalidArg;

    MountEntry* slot = nullptr;
    for (uint8_t i = 0u; i < kMaxMounts; ++i) {
        if (m_mounts[i].used && strcmp(m_mounts[i].prefix, prefix) == 0) { slot = &m_mounts[i]; break; }
        if (!m_mounts[i].used && slot == nullptr) slot = &m_mounts[i];
    }
    if (!slot) return Result::OutOfMemory;
    memcpy(slot->prefix, prefix, pl + 1u);
    memcpy(slot->root, root, rl + 1u);
    slot->used = true;
    return Result::Ok;
}

bool Vfs::Unmount(const char* prefix) {
    if (!prefix) return false;
    for (uint8_t i = 0u; i < kMaxMounts; ++i) {
        if (m_mounts[i].used && strcmp(m_mounts[i].prefix, prefix) == 0) {
            m_mounts[i].used = false;
            return true;
        }
    }
    return false;
}

bool Vfs::IsMounted(const char* prefix) const {
    if (!prefix) return false;
    for (uint8_t i = 0u; i < kMaxMounts; ++i) {
        if (m_mounts[i].used && strcmp(m_mounts[i].prefix, prefix) == 0) return true;
    }
    return false;
}

const Vfs::MountEntry* Vfs::Find(const char* path, size_t& prefixLen) const {
    prefixLen = PrefixLength(path);
    if (prefixLen == 0u) return nullptr;
    for (uint8_t i = 0u; i < kMaxMounts; ++i) {
        if (m_mounts[i].used && strlen(m_mounts[i].prefix) == prefixLen &&
            strncmp(m_mounts[i].prefix, path, prefixLen) == 0) {
            return &m_mounts[i];
        }
    }
    return nullptr;
}

Result Vfs::Resolve(const char* path, char* outNative, size_t maxLen) const {
    if (!path || !outNative || maxLen == 0u) return Result::InvalidArg;
    size_t pl = 0u;
    const MountEntry* m = Find(path, pl);
    if (m == nullptr) {
        const size_t n = strlen(path);
        if (n + 1u > maxLen) return Result::InvalidArg;
        memcpy(outNative, path, n + 1u);   /* native passthrough */
        return Result::Ok;
    }
    const char* rest = path + pl;
    while (*rest == '/' || *rest == '\\') ++rest;
    if (HasDotDot(rest)) return Result::InvalidArg;
    const size_t rl = strlen(m->root);
    const size_t n  = strlen(rest);
    if (rl + n + 1u > maxLen) return Result::InvalidArg;
    memcpy(outNative, m->root, rl);
    memcpy(outNative + rl, rest, n + 1u);
    return Result::Ok;
}

Result Vfs::Open(const char* path, FileMode mode, File& outFile) {
    char native[kMaxPathLen];
    Result r = Resolve(path, native, sizeof(native));
    return Failed(r) ? r : m_fs.Open(native, mode, outFile);
}

bool Vfs::Exists(const char* path) const {
    char native[kMaxPathLen];
    return Succeeded(Resolve(path, native, sizeof(native))) && m_fs.Exists(native);
}

Result Vfs::Remove(const char* path) {
    char native[kMaxPathLen];
    Result r = Resolve(path, native, sizeof(native));
    return Failed(r) ? r : m_fs.Remove(native);
}

Result Vfs::MkDir(const char* path) {
    char native[kMaxPathLen];
    Result r = Resolve(path, native, sizeof(native));
    return Failed(r) ? r : m_fs.MkDir(native);
}

Result Vfs::Rename(const char* srcPath, const char* dstPath) {
    char a[kMaxPathLen];
    char b[kMaxPathLen];
    Result r = Resolve(srcPath, a, sizeof(a));
    if (Failed(r)) return r;
    r = Resolve(dstPath, b, sizeof(b));
    return Failed(r) ? r : m_fs.Rename(a, b);
}

Result Vfs::ReadAllBytes(const char* path, void** outBuffer, uint32_t& outSize) {
    char native[kMaxPathLen];
    Result r = Resolve(path, native, sizeof(native));
    return Failed(r) ? r : m_fs.ReadAllBytes(native, outBuffer, outSize);
}

} // namespace fs
} // namespace zsdk
