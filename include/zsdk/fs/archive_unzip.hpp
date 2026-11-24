/**
 * @file archive_unzip.hpp
 * @brief Compressed stream decompression over BREW IUnzipStream (AEEUnzipStream.h).
 */

#ifndef ZSDK_FS_ARCHIVE_UNZIP_HPP
#define ZSDK_FS_ARCHIVE_UNZIP_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace fs {

/**
 * @brief Streaming zlib/deflate decompressor wrapping BREW IUnzipStream*.
 */
class UnzipStream : public NonCopyable {
public:
    UnzipStream();
    ~UnzipStream();

    /** @brief Creates an IUnzipStream bound to an open IAStream* or compressed source. */
    Result Initialize(void* platformContext);
    void   Shutdown();

    /** @brief Attaches a native BREW IAStream* as compressed input. */
    Result SetInputStream(void* nativeIAStream);

    /** @brief Reads up to @p bytes decompressed bytes into @p outBuffer. */
    int32_t Read(void* outBuffer, uint32_t bytes);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace fs
} // namespace zsdk

#endif /* ZSDK_FS_ARCHIVE_UNZIP_HPP */
