/**
 * @file ram_cache.hpp
 * @brief Deterministic in-memory cache over BREW IRamCache (AEERamCache.h).
 */

#ifndef ZSDK_FS_RAM_CACHE_HPP
#define ZSDK_FS_RAM_CACHE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace fs {

/**
 * @brief Wraps BREW IRamCache for bounded-memory asset caching.
 */
class RamCache : public NonCopyable {
public:
    RamCache();
    ~RamCache();

    Result Initialize(void* platformContext, uint32_t maxBytes);
    void   Shutdown();

    Result Put(const char* key, const void* data, uint32_t sizeBytes);
    Result Get(const char* key, void* outBuffer, uint32_t maxBytes, uint32_t& outSize);
    void   Clear();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_RAM_CACHE_HPP */
