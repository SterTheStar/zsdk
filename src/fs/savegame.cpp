/**
 * @file savegame.cpp
 * @brief Zeebo `udata` user-data & CRC32-verified SaveGame implementation.
 */

#include "zsdk/fs/savegame.hpp"
#include "zsdk/core/system_info.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO
    #include "AEEStdLib.h"
    #define ZSDK_SNPRINTF SNPRINTF
#else
    #include <stdio.h>
    #define ZSDK_SNPRINTF snprintf
#endif

namespace zsdk {
namespace fs {

namespace {

static const uint32_t kSaveMagic = 0x3156535Au; // 'ZSV1'

struct SaveHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t payloadSize;
    uint32_t crc32;
};

uint32_t ComputeCrc32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0u; i < length; ++i) {
        crc ^= static_cast<uint32_t>(data[i]);
        for (uint8_t b = 0u; b < 8u; ++b) {
            const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1u)));
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

void BuildSlotFilename(uint8_t slotIndex, char* outName, size_t maxLen) {
    ZSDK_SNPRINTF(outName, maxLen, "save_%02u.sav", static_cast<unsigned>(slotIndex));
}

void BuildSlotTmpFilename(uint8_t slotIndex, char* outName, size_t maxLen) {
    ZSDK_SNPRINTF(outName, maxLen, "save_%02u.tmp", static_cast<unsigned>(slotIndex));
}

/* Margin kept free on top of the save itself (directory entries, filesystem metadata). */
static const uint32_t kSaveFreeSpaceMargin = 4096u;

} // namespace

SaveGameManager::SaveGameManager()
    : m_initialized(false)
    , m_useSdCard(false)
    , m_asyncStatus(AsyncSaveStatus::Idle)
    , m_asyncSize(0u)
    , m_asyncOffset(0u)
    , m_asyncVersion(0u)
    , m_asyncSlot(0u) {
    m_asyncTmpPath[0] = '\0';
    m_asyncPath[0] = '\0';
}

SaveGameManager::~SaveGameManager() {
    Shutdown();
}

void SaveGameManager::BuildUdataPath(const char* filename, char* outPath, size_t maxLen) const {
    if (!outPath || maxLen == 0u) {
        return;
    }
#if ZSDK_PLATFORM_ZEEBO
    if (m_useSdCard) {
        ZSDK_SNPRINTF(outPath, maxLen, "fs:/card0/udata/%s", filename ? filename : "");
    } else {
        ZSDK_SNPRINTF(outPath, maxLen, "fs:/~/udata/%s", filename ? filename : "");
    }
#else
    ZSDK_SNPRINTF(outPath, maxLen, "udata/%s", filename ? filename : "");
#endif
}

Result SaveGameManager::Initialize(void* platformContext, bool preferSdCard) {
    if (m_initialized) {
        return Result::AlreadyInitialized;
    }
    Result r = m_fs.Initialize(platformContext);
    if (Failed(r)) {
        return r;
    }

#if ZSDK_PLATFORM_ZEEBO
    m_useSdCard = (preferSdCard && m_fs.Exists("fs:/card0/"));
    if (m_useSdCard) {
        m_fs.MkDir("fs:/card0/udata");
        ZSDK_LOG_INFO("SAVE", "udata -> fs:/card0/udata (SD)");
    } else {
        m_fs.MkDir("fs:/~/udata");
        ZSDK_LOG_INFO("SAVE", "udata -> fs:/~/udata (Home)");
    }
#else
    ZSDK_UNUSED(preferSdCard);
    m_useSdCard = false;
    m_fs.MkDir("udata");
#endif

    m_initialized = true;
    return Result::Ok;
}

void SaveGameManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    m_asyncFile.Close();
    m_asyncStatus = AsyncSaveStatus::Idle;
    m_fs.Shutdown();
    m_initialized = false;
}

Result SaveGameManager::WriteUserFile(const char* filename, const void* data, uint32_t sizeBytes) {
    if (!m_initialized || !filename || (!data && sizeBytes > 0u)) {
        return Result::InvalidArg;
    }
    if (sizeBytes > kMaxSaveDataBytes) {
        ZSDK_LOG_ERROR("SAVE", "WriteUserFile(%u B) exceeds Zeebo 64KB limit", sizeBytes);
        return Result::InvalidArg;
    }
    if (!m_useSdCard) {
        SystemMemoryInfo mem;
        if (Succeeded(SystemInfo::GetMemoryInfo(mem)) &&
            mem.freeFsBytes > 0u &&
            mem.freeFsBytes < (sizeBytes + 4096u)) {
            ZSDK_LOG_ERROR("SAVE", "Insufficient flash (%u B free, need %u B)",
                           mem.freeFsBytes, sizeBytes);
            return Result::IoError;
        }
    }
    char path[128];
    BuildUdataPath(filename, path, sizeof(path));

    File f;
    Result r = m_fs.Open(path, FileMode::Create, f);
    if (Failed(r)) {
        return r;
    }
    if (sizeBytes > 0u) {
        const int32_t written = f.Write(data, sizeBytes);
        if (written < 0 || static_cast<uint32_t>(written) != sizeBytes) {
            return Result::IoError;
        }
    }
    return Result::Ok;
}

Result SaveGameManager::ReadUserFile(const char* filename, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead) {
    outBytesRead = 0u;
    if (!m_initialized || !filename || !outBuffer || maxBytes == 0u) {
        return Result::InvalidArg;
    }
    char path[128];
    BuildUdataPath(filename, path, sizeof(path));

    File f;
    Result r = m_fs.Open(path, FileMode::Read, f);
    if (Failed(r)) {
        return r;
    }
    const int32_t read = f.Read(outBuffer, maxBytes);
    if (read < 0) {
        return Result::IoError;
    }
    outBytesRead = static_cast<uint32_t>(read);
    return Result::Ok;
}

bool SaveGameManager::ExistsUserFile(const char* filename) const {
    if (!m_initialized || !filename) {
        return false;
    }
    char path[128];
    BuildUdataPath(filename, path, sizeof(path));
    return m_fs.Exists(path);
}

Result SaveGameManager::DeleteUserFile(const char* filename) {
    if (!m_initialized || !filename) {
        return Result::InvalidArg;
    }
    char path[128];
    BuildUdataPath(filename, path, sizeof(path));
    return m_fs.Remove(path);
}

bool SaveGameManager::HasRoomFor(uint32_t payloadBytes) const {
    const uint32_t freeBytes = m_fs.GetFreeBytes();
    const uint32_t need = payloadBytes + static_cast<uint32_t>(sizeof(SaveHeader)) + kSaveFreeSpaceMargin;
    if (freeBytes < need) {
        ZSDK_LOG_ERROR("SAVE", "Not enough free space (%u free, %u needed)", freeBytes, need);
        return false;
    }
    return true;
}

Result SaveGameManager::Commit(const char* tmpPath, const char* finalPath) {
    /* BREW Rename fails when the destination exists, so drop the old file first. If we die
       between the two calls the complete .tmp survives and LoadSlot() falls back to it. */
    m_fs.Remove(finalPath);
    const Result r = m_fs.Rename(tmpPath, finalPath);
    if (Failed(r)) {
        ZSDK_LOG_ERROR("SAVE", "Rename failed; keeping %s", tmpPath);
    }
    return r;
}

Result SaveGameManager::SaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion) {
    if (!m_initialized || slotIndex >= kMaxSaveSlots || !data || sizeBytes == 0u) {
        return Result::InvalidArg;
    }
    if (m_asyncStatus == AsyncSaveStatus::Pending && m_asyncSlot == slotIndex) {
        return Result::WouldBlock;
    }
    if (!HasRoomFor(sizeBytes)) {
        return Result::IoError;
    }

    char name[32];
    char path[128];
    char tmpName[32];
    char tmpPath[128];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    BuildUdataPath(name, path, sizeof(path));
    BuildSlotTmpFilename(slotIndex, tmpName, sizeof(tmpName));
    BuildUdataPath(tmpName, tmpPath, sizeof(tmpPath));

    SaveHeader hdr;
    hdr.magic       = kSaveMagic;
    hdr.version     = gameVersion;
    hdr.payloadSize = sizeBytes;
    hdr.crc32       = ComputeCrc32(static_cast<const uint8_t*>(data), sizeBytes);

    {
        File f;
        Result r = m_fs.Open(tmpPath, FileMode::Create, f);
        if (Failed(r)) {
            return r;
        }
        if (f.Write(&hdr, sizeof(hdr)) != static_cast<int32_t>(sizeof(hdr)) ||
            f.Write(data, sizeBytes) != static_cast<int32_t>(sizeBytes)) {
            f.Close();
            m_fs.Remove(tmpPath);
            return Result::IoError;
        }
    } /* File closes here, before Rename */

    const Result cr = Commit(tmpPath, path);
    if (Succeeded(cr)) {
        ZSDK_LOG_INFO("SAVE", "Slot %u saved (%u B)", slotIndex, sizeBytes);
    }
    return cr;
}

Result SaveGameManager::LoadPath(const char* path, void* outBuffer, uint32_t maxBytes,
                                 uint32_t& outBytesRead, uint32_t expectedVersion) {
    outBytesRead = 0u;
    File f;
    Result r = m_fs.Open(path, FileMode::Read, f);
    if (Failed(r)) {
        return r;
    }

    SaveHeader hdr;
    if (f.Read(&hdr, sizeof(hdr)) != static_cast<int32_t>(sizeof(hdr))) {
        return Result::IoError;
    }
    if (hdr.magic != kSaveMagic || hdr.version != expectedVersion || hdr.payloadSize > maxBytes) {
        return Result::IoError;
    }

    const int32_t read = f.Read(outBuffer, hdr.payloadSize);
    if (read < 0 || static_cast<uint32_t>(read) != hdr.payloadSize) {
        return Result::IoError;
    }
    if (ComputeCrc32(static_cast<const uint8_t*>(outBuffer), hdr.payloadSize) != hdr.crc32) {
        ZSDK_LOG_ERROR("SAVE", "CRC mismatch in %s", path);
        return Result::IoError;
    }

    outBytesRead = hdr.payloadSize;
    return Result::Ok;
}

Result SaveGameManager::LoadSlot(uint8_t slotIndex, void* outBuffer, uint32_t maxBytes,
                                 uint32_t& outBytesRead, uint32_t expectedVersion) {
    outBytesRead = 0u;
    if (!m_initialized || slotIndex >= kMaxSaveSlots || !outBuffer || maxBytes == 0u) {
        return Result::InvalidArg;
    }

    char name[32];
    char path[128];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    BuildUdataPath(name, path, sizeof(path));

    Result r = LoadPath(path, outBuffer, maxBytes, outBytesRead, expectedVersion);
    if (Succeeded(r)) {
        return r;
    }

    /* Power loss between Remove() and Rename() leaves a complete, CRC-valid .tmp: recover it. */
    char tmpName[32];
    char tmpPath[128];
    BuildSlotTmpFilename(slotIndex, tmpName, sizeof(tmpName));
    BuildUdataPath(tmpName, tmpPath, sizeof(tmpPath));
    if (Succeeded(LoadPath(tmpPath, outBuffer, maxBytes, outBytesRead, expectedVersion))) {
        ZSDK_LOG_WARN("SAVE", "Slot %u recovered from %s", slotIndex, tmpName);
        return Result::Ok;
    }
    return r;
}

Result SaveGameManager::BeginSaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion) {
    if (!m_initialized || slotIndex >= kMaxSaveSlots || !data || sizeBytes == 0u || sizeBytes > kAsyncSaveMaxBytes) {
        return Result::InvalidArg;
    }
    if (m_asyncStatus == AsyncSaveStatus::Pending) {
        return Result::WouldBlock;
    }

    if (!HasRoomFor(sizeBytes)) {
        m_asyncStatus = AsyncSaveStatus::Failed;
        return Result::IoError;
    }

    char name[32];
    char tmpName[32];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    BuildUdataPath(name, m_asyncPath, sizeof(m_asyncPath));
    BuildSlotTmpFilename(slotIndex, tmpName, sizeof(tmpName));
    BuildUdataPath(tmpName, m_asyncTmpPath, sizeof(m_asyncTmpPath));

    Result r = m_fs.Open(m_asyncTmpPath, FileMode::Create, m_asyncFile);
    if (Failed(r)) {
        m_asyncStatus = AsyncSaveStatus::Failed;
        return r;
    }

    /* Placeholder header with an invalid magic: valid only once the payload is complete. */
    SaveHeader hdr;
    hdr.magic = 0u; hdr.version = 0u; hdr.payloadSize = 0u; hdr.crc32 = 0u;
    if (m_asyncFile.Write(&hdr, sizeof(hdr)) != static_cast<int32_t>(sizeof(hdr))) {
        m_asyncFile.Close();
        m_asyncStatus = AsyncSaveStatus::Failed;
        return Result::IoError;
    }

    const uint8_t* src = static_cast<const uint8_t*>(data);
    for (uint32_t i = 0u; i < sizeBytes; ++i) {
        m_asyncBuf[i] = src[i];
    }
    m_asyncSize    = sizeBytes;
    m_asyncOffset  = 0u;
    m_asyncVersion = gameVersion;
    m_asyncSlot    = slotIndex;
    m_asyncStatus  = AsyncSaveStatus::Pending;
    return Result::Ok;
}

AsyncSaveStatus SaveGameManager::PumpAsync(uint32_t budgetBytes) {
    if (m_asyncStatus != AsyncSaveStatus::Pending) {
        return m_asyncStatus;
    }
    if (budgetBytes == 0u) {
        budgetBytes = 1u;
    }

    const uint32_t remaining = m_asyncSize - m_asyncOffset;
    const uint32_t chunk = (remaining < budgetBytes) ? remaining : budgetBytes;
    if (chunk > 0u) {
        const int32_t w = m_asyncFile.Write(m_asyncBuf + m_asyncOffset, chunk);
        if (w != static_cast<int32_t>(chunk)) {
            m_asyncFile.Close();
            m_fs.Remove(m_asyncTmpPath);
            m_asyncStatus = AsyncSaveStatus::Failed;
            ZSDK_LOG_ERROR("SAVE", "Async slot %u write failed", m_asyncSlot);
            return m_asyncStatus;
        }
        m_asyncOffset += chunk;
    }

    if (m_asyncOffset >= m_asyncSize) {
        SaveHeader hdr;
        hdr.magic       = kSaveMagic;
        hdr.version     = m_asyncVersion;
        hdr.payloadSize = m_asyncSize;
        hdr.crc32       = ComputeCrc32(m_asyncBuf, m_asyncSize);
        const bool ok = Succeeded(m_asyncFile.Seek(SeekOrigin::Start, 0)) &&
                        m_asyncFile.Write(&hdr, sizeof(hdr)) == static_cast<int32_t>(sizeof(hdr));
        m_asyncFile.Close();
        const bool done = ok && Succeeded(Commit(m_asyncTmpPath, m_asyncPath));
        m_asyncStatus = done ? AsyncSaveStatus::Done : AsyncSaveStatus::Failed;
        if (done) {
            ZSDK_LOG_INFO("SAVE", "Async slot %u saved (%u B)", m_asyncSlot, m_asyncSize);
        } else {
            if (!ok) {
                m_fs.Remove(m_asyncTmpPath);   /* only drop the tmp if it is incomplete */
            }
            ZSDK_LOG_ERROR("SAVE", "Async slot %u finalize failed", m_asyncSlot);
        }
    }
    return m_asyncStatus;
}

bool SaveGameManager::HasSlot(uint8_t slotIndex) const {
    if (slotIndex >= kMaxSaveSlots) {
        return false;
    }
    char name[32];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    return ExistsUserFile(name);
}

Result SaveGameManager::DeleteSlot(uint8_t slotIndex) {
    if (slotIndex >= kMaxSaveSlots) {
        return Result::InvalidArg;
    }
    char name[32];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    return DeleteUserFile(name);
}

} // namespace fs
} // namespace zsdk
