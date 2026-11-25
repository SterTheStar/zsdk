/**
 * @file filesystem.hpp
 * @brief Deterministic Filesystem & Storage abstraction over BREW IFileMgr / IFile.
 *
 * Isolates AEEFile.h types from user code and routes whole-file reads through
 * debug::MemoryTracker under MemorySubsystem::Filesystem.
 */

#ifndef ZSDK_FS_FILESYSTEM_HPP
#define ZSDK_FS_FILESYSTEM_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace fs {

enum class FileMode : uint8_t {
    Read = 0,       /**< _OFM_READ */
    ReadWrite,      /**< _OFM_READWRITE */
    Create,         /**< _OFM_CREATE | _OFM_READWRITE */
    Append          /**< _OFM_APPEND | _OFM_READWRITE */
};

enum class SeekOrigin : uint8_t {
    Start = 0,      /**< _SEEK_START */
    Current,        /**< _SEEK_CURRENT */
    End             /**< _SEEK_END */
};

/**
 * @brief Metadata for a single entry yielded by FileSystem::EnumerateDirectory().
 */
struct FileEntry {
    static constexpr uint32_t kMaxNameLength = 64u;
    char     name[kMaxNameLength];  /**< Null-terminated entry path/name (AEE_MAX_FILE_NAME). */
    uint32_t size;                  /**< File size in bytes (0 for directories). */
    uint32_t creationDate;          /**< BREW creation timestamp. */
    bool     isDirectory;           /**< True if AEE_FA_DIR is set. */
    bool     isReadOnly;            /**< True if AEE_FA_READONLY is set. */
    bool     isHidden;              /**< True if AEE_FA_HIDDEN is set. */
};

/**
 * @brief Zero-allocation callback invoked for each entry during directory enumeration.
 * @param entry    Metadata of the current file or directory.
 * @param userData Opaque user pointer passed to FileSystem::EnumerateDirectory().
 * @return true to continue enumeration, false to stop early.
 */
typedef bool (*EnumerateDirCallbackFn)(const FileEntry& entry, void* userData);

/**
 * @brief RAII file handle wrapping BREW IFile*.
 */
class File : public NonCopyable {
public:
    File();
    ~File();

    bool     IsOpen() const;
    int32_t  Read(void* buffer, uint32_t bytes);
    int32_t  Write(const void* buffer, uint32_t bytes);
    Result   Seek(SeekOrigin origin, int32_t offset);
    uint32_t GetSize() const;
    /** @brief Pushes buffered writes to storage (PC: fflush; BREW IFile writes are unbuffered, no-op). */
    void     Flush();
    void     Close();

private:
    friend class FileSystem;
    void* m_nativeFile;
};

/**
 * @brief Filesystem service wrapping BREW IFileMgr*.
 */
class FileSystem : public NonCopyable {
public:
    FileSystem();
    ~FileSystem();

    Result Initialize(void* platformContext);
    void   Shutdown();

    Result Open(const char* path, FileMode mode, File& outFile);
    bool   Exists(const char* path) const;
    Result Remove(const char* path);
    Result MkDir(const char* path);

    /**
     * @brief Renames @p srcPath to @p dstPath (same volume). Fails if @p dstPath exists on BREW,
     *        so callers replacing a file should Remove() the destination first.
     */
    Result Rename(const char* srcPath, const char* dstPath);

    /**
     * @brief Free bytes on the volume (IFILEMGR_GetFreeSpace); 0 on error. Check before saving.
     * @param outTotal Optional: receives the total size in bytes.
     */
    uint32_t GetFreeBytes(uint32_t* outTotal = nullptr) const;

    /**
     * @brief Enumerates files or subdirectories in @p dirPath without heap allocations.
     * @param dirPath         Directory path (e.g. "fs:/~" or "fs:/~/saves").
     * @param directoriesOnly If true, enumerates only subdirectories; otherwise files.
     * @param callback        Zero-alloc callback invoked per entry.
     * @param userData        Optional user pointer forwarded to @p callback.
     * @return Result::Ok on completion, or an error code.
     */
    Result EnumerateDirectory(const char* dirPath,
                              bool directoriesOnly,
                              EnumerateDirCallbackFn callback,
                              void* userData = nullptr);

    /**
     * @brief Reads an entire file into a MemoryTracker-allocated buffer (MemorySubsystem::Filesystem).
     *        Caller must release @p outBuffer via FileSystem::FreeBuffer().
     */
    Result ReadAllBytes(const char* path, void** outBuffer, uint32_t& outSize);
    static void FreeBuffer(void* buffer);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_FILESYSTEM_HPP */
