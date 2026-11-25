/**
 * @file savegame.hpp
 * @brief Standardized Zeebo `udata` user-data & CRC32-verified SaveGame system.
 *
 * Follows the Zeebo/BREW convention of storing persistent game user files and
 * save slots under the application's `udata` directory (`fs:/~/udata/` on Zeebo,
 * `udata/` on Windows PC), with zero per-call heap allocations.
 */

#ifndef ZSDK_FS_SAVEGAME_HPP
#define ZSDK_FS_SAVEGAME_HPP

#include "zsdk/fs/filesystem.hpp"

namespace zsdk {
namespace fs {

static const uint8_t  kMaxSaveSlots       = 16u;
static const uint32_t kAsyncSaveMaxBytes  = 4096u;

/** @brief State of the background (frame-budgeted) save started by BeginSaveSlot(). */
enum class AsyncSaveStatus : uint8_t {
    Idle = 0,   /**< Nothing started since Initialize()/last PollAsync() of a finished job. */
    Pending,    /**< PumpAsync() still has bytes to write. */
    Done,       /**< Last job finished OK (stays until the next BeginSaveSlot()). */
    Failed      /**< Last job failed; the slot file is left invalid (magic cleared). */
};

/**
 * @brief Standardized `udata` directory & SaveGame slot manager.
 */
class SaveGameManager : public NonCopyable {
public:
    SaveGameManager();
    ~SaveGameManager();

    /**
     * @brief Initializes underlying FileSystem and ensures `udata/` exists inside the
     *        game's own directory (`fs:/~/udata/` on Zeebo, `udata/` on PC).
     *        Pass @p preferSdCard = true to force `fs:/card0/udata/` when an SD card is mounted.
     * @param platformContext Opaque IShell* (from Application::GetNativePlatformContext()).
     */
    Result Initialize(void* platformContext, bool preferSdCard = false);
    void   Shutdown();

    /** @brief Returns true if saves/udata are currently routed to SD card (`fs:/card0/udata`). */
    bool   IsUsingSdCard() const { return m_useSdCard; }

    /** @brief Writes an arbitrary game user file inside `udata/<filename>`. */
    Result WriteUserFile(const char* filename, const void* data, uint32_t sizeBytes);

    /** @brief Reads an arbitrary game user file from `udata/<filename>`. */
    Result ReadUserFile(const char* filename, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead);

    /** @brief Checks whether `udata/<filename>` exists. */
    bool   ExistsUserFile(const char* filename) const;

    /** @brief Deletes `udata/<filename>`. */
    Result DeleteUserFile(const char* filename);

    /**
     * @brief Saves a structured savegame slot (`udata/save_XX.sav`) with magic,
     *        version tag, and CRC32 integrity checksum. Writes `save_XX.tmp` first and swaps it
     *        in only when complete; fails (IoError) if the volume lacks free space.
     */
    Result SaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion = 1u);

    /**
     * @brief Loads a savegame slot and verifies magic, version, and CRC32.
     */
    Result LoadSlot(uint8_t slotIndex, void* outBuffer, uint32_t maxBytes,
                    uint32_t& outBytesRead, uint32_t expectedVersion = 1u);

    /**
     * @brief Starts a frame-budgeted save: copies @p data (<= kAsyncSaveMaxBytes) and returns at once.
     *        Call PumpAsync() once per frame until it stops returning Pending. The header (magic) is
     *        written last, so an interrupted save is detected as invalid by LoadSlot().
     * @return WouldBlock if a previous job is still Pending.
     */
    Result          BeginSaveSlot(uint8_t slotIndex, const void* data, uint32_t sizeBytes, uint32_t gameVersion = 1u);
    /** @brief Writes up to @p budgetBytes of the pending job. */
    AsyncSaveStatus PumpAsync(uint32_t budgetBytes = 1024u);
    AsyncSaveStatus GetAsyncStatus() const { return m_asyncStatus; }

    bool   HasSlot(uint8_t slotIndex) const;
    Result DeleteSlot(uint8_t slotIndex);

    /** @brief Builds the full platform path for `udata/<filename>` into @p outPath. */
    void BuildUdataPath(const char* filename, char* outPath, size_t maxLen) const;

private:
    /** @brief Reads and validates one save file (magic, version, CRC32). */
    Result LoadPath(const char* path, void* outBuffer, uint32_t maxBytes,
                    uint32_t& outBytesRead, uint32_t expectedVersion);
    /** @brief True if the volume has room for @p payloadBytes plus header and a safety margin. */
    bool   HasRoomFor(uint32_t payloadBytes) const;
    /** @brief Replaces @p finalPath with the fully written @p tmpPath (Remove + Rename). */
    Result Commit(const char* tmpPath, const char* finalPath);

    FileSystem      m_fs;
    bool            m_initialized;
    bool            m_useSdCard;

    File            m_asyncFile;
    AsyncSaveStatus m_asyncStatus;
    uint32_t        m_asyncSize;
    uint32_t        m_asyncOffset;
    uint32_t        m_asyncVersion;
    uint8_t         m_asyncSlot;
    char            m_asyncTmpPath[128];
    char            m_asyncPath[128];
    uint8_t         m_asyncBuf[kAsyncSaveMaxBytes];
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_SAVEGAME_HPP */
