/**
 * @file local_storage.hpp
 * @brief Persistent key-value / record storage over BREW ILocalStorage / IDBMgr.
 */

#ifndef ZSDK_FS_LOCAL_STORAGE_HPP
#define ZSDK_FS_LOCAL_STORAGE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace fs {

/**
 * @brief Wraps BREW ILocalStorage for fast, persistent save-data / settings storage.
 */
class LocalStorage : public NonCopyable {
public:
    LocalStorage();
    ~LocalStorage();

    Result Initialize(void* platformContext);
    void   Shutdown();

    Result Write(const char* key, const void* data, uint32_t sizeBytes);
    Result Read(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outBytesRead);
    Result Remove(const char* key);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_LOCAL_STORAGE_HPP */
