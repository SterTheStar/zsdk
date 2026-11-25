/**
 * @file vfs.hpp
 * @brief Mount-prefix virtual filesystem over FileSystem (design inspired by the
 *        mount-point model of other console SDKs; no code shared).
 *
 * Game code uses portable paths such as "udata:/save_00.sav" or "script:/main.lua".
 * Each prefix maps to a native root (Zeebo: "fs:/~/udata/", PC: "udata/").
 * A path whose prefix is not mounted is passed through unchanged (native path).
 * Mounted paths reject ".." segments, so scripts cannot escape their mount.
 *
 * Default mounts after Initialize():
 *   home:   application home        (Zeebo fs:/~/        | PC ./)
 *   udata:  user data / saves       (Zeebo fs:/~/udata/ or fs:/card0/udata/ | PC udata/)
 *   script: Lua scripts             (Zeebo fs:/~/scripts/ | PC scripts/)
 *   sd:     removable card          (Zeebo fs:/card0/, only if preferSdCard and card present; path unverified)
 */

#ifndef ZSDK_FS_VFS_HPP
#define ZSDK_FS_VFS_HPP

#include "zsdk/fs/filesystem.hpp"

namespace zsdk {
namespace fs {

class Vfs : public NonCopyable {
public:
    static const uint8_t  kMaxMounts     = 8u;
    static const uint8_t  kMaxPrefixLen  = 12u;   /* including ':' */
    static const uint16_t kMaxRootLen    = 64u;
    static const uint16_t kMaxPathLen    = 128u;

    Vfs();
    ~Vfs();

    /** @brief Initializes the underlying FileSystem and installs the default mounts. */
    Result Initialize(void* platformContext, bool preferSdCard = false);
    void   Shutdown();

    /** @brief Maps @p prefix (e.g. "mods:") to native @p root. Replaces an existing mount. */
    Result Mount(const char* prefix, const char* root);
    bool   Unmount(const char* prefix);
    bool   IsMounted(const char* prefix) const;

    /**
     * @brief Resolves a virtual path into a native path.
     * @return Ok; NotFound never (unmounted prefixes pass through); InvalidArg for ".." or overflow.
     */
    Result Resolve(const char* path, char* outNative, size_t maxLen) const;

    Result Open(const char* path, FileMode mode, File& outFile);
    bool   Exists(const char* path) const;
    Result Remove(const char* path);
    Result MkDir(const char* path);
    Result Rename(const char* srcPath, const char* dstPath);
    uint32_t GetFreeBytes(uint32_t* outTotal = nullptr) const { return m_fs.GetFreeBytes(outTotal); }

    /** @brief Reads a whole file; release with FileSystem::FreeBuffer(). */
    Result ReadAllBytes(const char* path, void** outBuffer, uint32_t& outSize);

    bool   IsUsingSdCard() const { return m_useSdCard; }
    FileSystem& Raw() { return m_fs; }

private:
    struct MountEntry {
        char prefix[kMaxPrefixLen + 1u];
        char root[kMaxRootLen + 1u];
        bool used;
    };

    const MountEntry* Find(const char* path, size_t& prefixLen) const;

    FileSystem m_fs;
    MountEntry m_mounts[kMaxMounts];
    bool       m_initialized;
    bool       m_useSdCard;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_VFS_HPP */
