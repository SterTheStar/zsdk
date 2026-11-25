/**
 * @file zeebo_fs.cpp
 * @brief Qualcomm BREW / Zeebo IFileMgr & IFile implementation.
 */

#include "zsdk/fs/filesystem.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEFile.h"

namespace zsdk {
namespace fs {

namespace {

OpenFileMode MapFileMode(FileMode mode) {
    switch (mode) {
        case FileMode::Read:      return _OFM_READ;
        case FileMode::ReadWrite: return _OFM_READWRITE;
        case FileMode::Create:    return static_cast<OpenFileMode>(_OFM_CREATE | _OFM_READWRITE);
        case FileMode::Append:    return static_cast<OpenFileMode>(_OFM_APPEND | _OFM_READWRITE);
        default:                  return _OFM_READ;
    }
}

FileSeekType MapSeekOrigin(SeekOrigin origin) {
    switch (origin) {
        case SeekOrigin::Start:   return _SEEK_START;
        case SeekOrigin::Current: return _SEEK_CURRENT;
        case SeekOrigin::End:     return _SEEK_END;
        default:                  return _SEEK_START;
    }
}

bool StartsWithNoCase(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    while (*prefix) {
        char a = *str++;
        char b = *prefix++;
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b + ('a' - 'A'));
        if (a != b) return false;
    }
    return true;
}

bool IsWritePathAllowed(const char* path) {
#if ZSDK_NAND_WRITE_GUARD
    if (!path || path[0] == '\0') {
        return false;
    }
    /* Block any "fs:/..." path unless it targets SD card (fs:/card0/) or app sandbox (fs:/~/) */
    if (StartsWithNoCase(path, "fs:/")) {
        if (StartsWithNoCase(path, "fs:/card0/") || StartsWithNoCase(path, "fs:/~/")) {
            return true;
        }
        ZSDK_LOG_ERROR("FS", "NAND guard blocked write: %s", path);
        return false;
    }
    /* Relative paths resolve inside fs:/~/ */
    return true;
#else
    ZSDK_UNUSED(path);
    return true;
#endif
}

} // namespace

File::File()
    : m_nativeFile(nullptr) {
}

File::~File() {
    Close();
}

bool File::IsOpen() const {
    return m_nativeFile != nullptr;
}

int32_t File::Read(void* buffer, uint32_t bytes) {
    if (!m_nativeFile || !buffer || bytes == 0u) {
        return -1;
    }
    return IFILE_Read(static_cast<IFile*>(m_nativeFile), buffer, bytes);
}

int32_t File::Write(const void* buffer, uint32_t bytes) {
    if (!m_nativeFile || !buffer || bytes == 0u) {
        return -1;
    }
    return static_cast<int32_t>(
        IFILE_Write(static_cast<IFile*>(m_nativeFile), buffer, bytes));
}

Result File::Seek(SeekOrigin origin, int32_t offset) {
    if (!m_nativeFile) {
        return Result::NotInitialized;
    }
    const int res = IFILE_Seek(static_cast<IFile*>(m_nativeFile), MapSeekOrigin(origin), offset);
    return (res == SUCCESS) ? Result::Ok : Result::IoError;
}

uint32_t File::GetSize() const {
    if (!m_nativeFile) {
        return 0u;
    }
    FileInfo info;
    MEMSET(&info, 0, sizeof(info));
    if (IFILE_GetInfo(static_cast<IFile*>(m_nativeFile), &info) != SUCCESS) {
        return 0u;
    }
    return info.dwSize;
}

void File::Flush() {
    /* IFILE_Write has no separate buffer layer; nothing to flush. */
}

void File::Close() {
    if (m_nativeFile) {
        IFILE_Release(static_cast<IFile*>(m_nativeFile));
        m_nativeFile = nullptr;
    }
}

struct FileSystem::Impl {
    IShell*   shell;
    IFileMgr* fileMgr;
};

FileSystem::FileSystem()
    : m_impl(nullptr) {
}

FileSystem::~FileSystem() {
    Shutdown();
}

Result FileSystem::Initialize(void* platformContext) {
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
    m_impl->shell = shell;
    m_impl->fileMgr = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_FILEMGR,
                              reinterpret_cast<void**>(&m_impl->fileMgr)) != AEE_SUCCESS || !m_impl->fileMgr) {
        ZSDK_LOG_ERROR("FS", "Failed to create AEECLSID_FILEMGR");
        Shutdown();
        return Result::IoError;
    }

    return Result::Ok;
}

void FileSystem::Shutdown() {
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

Result FileSystem::Open(const char* path, FileMode mode, File& outFile) {
    if (!m_impl || !m_impl->fileMgr || !path) {
        return Result::InvalidArg;
    }
    if (mode != FileMode::Read && !IsWritePathAllowed(path)) {
        return Result::IoError;
    }
    outFile.Close();
    IFile* f = IFILEMGR_OpenFile(m_impl->fileMgr, path, MapFileMode(mode));
    if (!f) {
        return Result::NotFound;
    }
    outFile.m_nativeFile = f;
    return Result::Ok;
}

bool FileSystem::Exists(const char* path) const {
    if (!m_impl || !m_impl->fileMgr || !path) {
        return false;
    }
    return IFILEMGR_Test(m_impl->fileMgr, path) == SUCCESS;
}

Result FileSystem::Remove(const char* path) {
    if (!m_impl || !m_impl->fileMgr || !path) {
        return Result::InvalidArg;
    }
    if (!IsWritePathAllowed(path)) {
        return Result::IoError;
    }
    return (IFILEMGR_Remove(m_impl->fileMgr, path) == SUCCESS) ? Result::Ok : Result::IoError;
}

Result FileSystem::Rename(const char* srcPath, const char* dstPath) {
    if (!m_impl || !m_impl->fileMgr || !srcPath || !dstPath) {
        return Result::InvalidArg;
    }
    if (!IsWritePathAllowed(srcPath) || !IsWritePathAllowed(dstPath)) {
        return Result::IoError;
    }
    return (IFILEMGR_Rename(m_impl->fileMgr, srcPath, dstPath) == SUCCESS) ? Result::Ok : Result::IoError;
}

uint32_t FileSystem::GetFreeBytes(uint32_t* outTotal) const {
    uint32 total = 0u;
    if (!m_impl || !m_impl->fileMgr) {
        if (outTotal) *outTotal = 0u;
        return 0u;
    }
    const uint32 freeBytes = IFILEMGR_GetFreeSpace(m_impl->fileMgr, &total);
    if (outTotal) *outTotal = static_cast<uint32_t>(total);
    return static_cast<uint32_t>(freeBytes);
}

Result FileSystem::MkDir(const char* path) {
    if (!m_impl || !m_impl->fileMgr || !path) {
        return Result::InvalidArg;
    }
    if (!IsWritePathAllowed(path)) {
        return Result::IoError;
    }
    return (IFILEMGR_MkDir(m_impl->fileMgr, path) == SUCCESS) ? Result::Ok : Result::IoError;
}

Result FileSystem::EnumerateDirectory(const char* dirPath,
                                      bool directoriesOnly,
                                      EnumerateDirCallbackFn callback,
                                      void* userData) {
    if (!m_impl || !m_impl->fileMgr) {
        return Result::NotInitialized;
    }
    if (!dirPath || !callback) {
        return Result::InvalidArg;
    }
    if (IFILEMGR_EnumInit(m_impl->fileMgr, dirPath,
                          directoriesOnly ? TRUE : FALSE) != SUCCESS) {
        return Result::IoError;
    }

    FileInfo info;
    FileEntry entry;
    MEMSET(&info, 0, sizeof(info));

    while (IFILEMGR_EnumNext(m_impl->fileMgr, &info)) {
        MEMSET(&entry, 0, sizeof(entry));
        STRLCPY(entry.name, info.szName, FileEntry::kMaxNameLength);
        entry.size         = info.dwSize;
        entry.creationDate = info.dwCreationDate;
        entry.isDirectory  = (info.attrib & AEE_FA_DIR) != 0;
        entry.isReadOnly   = (info.attrib & AEE_FA_READONLY) != 0;
        entry.isHidden     = (info.attrib & AEE_FA_HIDDEN) != 0;

        if (!callback(entry, userData)) {
            break;
        }
        MEMSET(&info, 0, sizeof(info));
    }

    return Result::Ok;
}

Result FileSystem::ReadAllBytes(const char* path, void** outBuffer, uint32_t& outSize) {
    outSize = 0u;
    if (!outBuffer) {
        return Result::InvalidArg;
    }
    *outBuffer = nullptr;

    File file;
    Result r = Open(path, FileMode::Read, file);
    if (Failed(r)) {
        return r;
    }

    const uint32_t size = file.GetSize();
    if (size == 0u) {
        return Result::Ok;
    }

    void* buf = debug::MemoryTracker::Alloc(debug::MemorySubsystem::Filesystem, size);
    if (!buf) {
        return Result::OutOfMemory;
    }

    const int32_t readBytes = file.Read(buf, size);
    if (readBytes < 0 || static_cast<uint32_t>(readBytes) != size) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, buf);
        return Result::IoError;
    }

    *outBuffer = buf;
    outSize = size;
    return Result::Ok;
}

void FileSystem::FreeBuffer(void* buffer) {
    debug::MemoryTracker::Free(debug::MemorySubsystem::Filesystem, buffer);
}

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
