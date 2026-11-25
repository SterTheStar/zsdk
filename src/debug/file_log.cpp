/**
 * @file file_log.cpp
 */

#include "zsdk/debug/file_log.hpp"

#include <string.h>

namespace zsdk {
namespace debug {

FileLogSink::FileLogSink() : m_written(0u), m_max(0u), m_open(false) {}
FileLogSink::~FileLogSink() { Close(); }

Result FileLogSink::Open(fs::Vfs& vfs, const char* path, bool truncate, uint32_t maxBytes) {
    if (m_open) return Result::AlreadyInitialized;
    Result r = vfs.Open(path, truncate ? fs::FileMode::Create : fs::FileMode::Append, m_file);
    if (Failed(r)) return r;
    m_written = 0u;
    m_max = maxBytes;
    m_open = true;
    Logger::AddSink(&FileLogSink::Thunk, this);
    return Result::Ok;
}

void FileLogSink::Close() {
    if (!m_open) return;
    Logger::RemoveSink(&FileLogSink::Thunk, this);
    m_file.Close();
    m_open = false;
}

void FileLogSink::Thunk(LogLevel, const char* line, void* user) {
    FileLogSink* self = static_cast<FileLogSink*>(user);
    if (!self->m_open) return;
    const uint32_t n = static_cast<uint32_t>(strlen(line));
    if (self->m_written + n + 1u > self->m_max) {
        /* cap reached: stop writing without recursing into the logger */
        Logger::RemoveSink(&FileLogSink::Thunk, self);
        return;
    }
    self->m_file.Write(line, n);
    self->m_file.Write("\n", 1u);
    self->m_written += n + 1u;
    self->m_file.Flush();   /* keep the log useful if the app dies */
}

} // namespace debug
} // namespace zsdk
