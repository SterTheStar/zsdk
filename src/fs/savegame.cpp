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

} // namespace

SaveGameManager::SaveGameManager()
    : m_initialized(false)
    , m_useSdCard(false) {
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

Result SaveGameManager::SaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion) {
    if (!m_initialized || slotIndex >= kMaxSaveSlots || !data || sizeBytes == 0u) {
        return Result::InvalidArg;
    }

    char name[32];
    char path[128];
    BuildSlotFilename(slotIndex, name, sizeof(name));
    BuildUdataPath(name, path, sizeof(path));

    SaveHeader hdr;
    hdr.magic       = kSaveMagic;
    hdr.version     = gameVersion;
    hdr.payloadSize = sizeBytes;
    hdr.crc32       = ComputeCrc32(static_cast<const uint8_t*>(data), sizeBytes);

    File f;
    Result r = m_fs.Open(path, FileMode::Create, f);
    if (Failed(r)) {
        return r;
    }
    if (f.Write(&hdr, sizeof(hdr)) != static_cast<int32_t>(sizeof(hdr))) {
        return Result::IoError;
    }
    if (f.Write(data, sizeBytes) != static_cast<int32_t>(sizeBytes)) {
        return Result::IoError;
    }

    ZSDK_LOG_INFO("SAVE", "Slot %u saved (%u B)", slotIndex, sizeBytes);
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
        ZSDK_LOG_ERROR("SAVE", "Slot %u CRC mismatch", slotIndex);
        return Result::IoError;
    }

    outBytesRead = hdr.payloadSize;
    return Result::Ok;
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
