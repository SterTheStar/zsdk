/**
 * @file file_log.hpp
 * @brief Logger sink that appends every line to a file through the Vfs
 *        (e.g. "udata:/zsdk.log", or "sd:/zsdk.log" on the SD card).
 */

#ifndef ZSDK_DEBUG_FILE_LOG_HPP
#define ZSDK_DEBUG_FILE_LOG_HPP

#include "zsdk/debug/logger.hpp"
#include "zsdk/fs/vfs.hpp"

namespace zsdk {
namespace debug {

class FileLogSink : public NonCopyable {
public:
    FileLogSink();
    ~FileLogSink();

    /**
     * @brief Opens @p path and registers as a Logger sink.
     * @param truncate true = start a new file, false = append.
     * Writing stops (and the sink unregisters) after @p maxBytes to protect NAND wear.
     */
    Result Open(fs::Vfs& vfs, const char* path, bool truncate = true, uint32_t maxBytes = 65536u);
    void   Close();
    bool   IsOpen() const { return m_open; }

private:
    static void Thunk(LogLevel level, const char* line, void* user);

    fs::File m_file;
    uint32_t m_written;
    uint32_t m_max;
    bool     m_open;
};

} // namespace debug
} // namespace zsdk

#endif /* ZSDK_DEBUG_FILE_LOG_HPP */
