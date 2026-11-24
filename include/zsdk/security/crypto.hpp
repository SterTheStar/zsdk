/**
 * @file crypto.hpp
 * @brief Cryptographic RNG, Hash (MD5/SHA-1), HMAC, Symmetric Cipher (ICipher1), and RSA over BREW.
 */

#ifndef ZSDK_SECURITY_CRYPTO_HPP
#define ZSDK_SECURITY_CRYPTO_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace security {

/**
 * @brief Hardware/OS random byte generator wrapping BREW IRandomSource (with GETRAND fallback).
 */
class Random {
public:
    static Result GetBytes(void* platformContext, void* outBuffer, uint32_t sizeBytes);
    static uint32_t GetUInt32(void* platformContext);
};

/**
 * @brief Supported cryptographic hash algorithms.
 */
enum class HashAlgorithm : uint8_t {
    Md5  = 0,
    Sha1 = 1
};

/**
 * @brief Streaming and one-shot cryptographic hash (MD5 / SHA-1) over BREW IHashCtx.
 */
class Hash : public NonCopyable {
public:
    static constexpr uint32_t kMd5DigestSize  = 16u;
    static constexpr uint32_t kSha1DigestSize = 20u;
    static constexpr uint32_t kMaxDigestSize  = 20u;

    Hash();
    ~Hash();

    /**
     * @brief Begins a streaming hash computation.
     * @param platformContext Platform context (IShell* on Zeebo).
     * @param algorithm       Hash algorithm (Md5 or Sha1).
     */
    Result Begin(void* platformContext, HashAlgorithm algorithm);

    /**
     * @brief Feeds @p sizeBytes of input data into the hash.
     */
    Result Update(const void* data, uint32_t sizeBytes);

    /**
     * @brief Finalizes the hash and writes the digest to @p outDigest.
     * @param outDigest         Destination buffer (at least 16 bytes for MD5, 20 for SHA-1).
     * @param digestBufferBytes Capacity of @p outDigest in bytes.
     * @param outDigestBytes    Optional output receiving the actual digest byte count.
     */
    Result Final(void* outDigest, uint32_t digestBufferBytes, uint32_t* outDigestBytes = nullptr);

    /**
     * @brief One-shot convenience helper to hash a contiguous buffer.
     */
    static Result Compute(void* platformContext,
                          HashAlgorithm algorithm,
                          const void* data,
                          uint32_t sizeBytes,
                          void* outDigest,
                          uint32_t digestBufferBytes,
                          uint32_t* outDigestBytes = nullptr);

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Streaming and one-shot HMAC (HMAC-MD5 / HMAC-SHA1) over BREW IHashCtx.
 */
class Hmac : public NonCopyable {
public:
    Hmac();
    ~Hmac();

    /**
     * @brief Begins a streaming HMAC computation with the given secret key.
     * @param platformContext Platform context (IShell* on Zeebo).
     * @param algorithm       Underlying hash algorithm (Md5 or Sha1).
     * @param key             Secret key bytes.
     * @param keyBytes        Length of @p key in bytes.
     */
    Result Begin(void* platformContext, HashAlgorithm algorithm, const void* key, uint32_t keyBytes);

    /**
     * @brief Feeds @p sizeBytes of message data into the HMAC.
     */
    Result Update(const void* data, uint32_t sizeBytes);

    /**
     * @brief Finalizes the HMAC and writes the authentication tag to @p outMac.
     */
    Result Final(void* outMac, uint32_t macBufferBytes, uint32_t* outMacBytes = nullptr);

    /**
     * @brief One-shot convenience helper to compute HMAC over a contiguous buffer.
     */
    static Result Compute(void* platformContext,
                          HashAlgorithm algorithm,
                          const void* key,
                          uint32_t keyBytes,
                          const void* data,
                          uint32_t sizeBytes,
                          void* outMac,
                          uint32_t macBufferBytes,
                          uint32_t* outMacBytes = nullptr);

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Supported symmetric cipher algorithms.
 */
enum class CipherAlgorithm : uint8_t {
    Aes128 = 0,
    Aes192,
    Aes256,
    Des,
    TripleDes,
    Arc4
};

/**
 * @brief Block cipher chaining modes (use Stream for ARC4).
 */
enum class CipherMode : uint8_t {
    Ecb = 0,
    Cbc,
    Cfb,
    Ctr,
    Stream
};

/**
 * @brief Cipher operation direction.
 */
enum class CipherDirection : uint8_t {
    Encrypt = 0,
    Decrypt = 1
};

/**
 * @brief Block cipher padding scheme.
 */
enum class CipherPadding : uint8_t {
    None  = 0,
    Zero  = 1,
    Pkcs7 = 2
};

/**
 * @brief Symmetric stream/block cipher wrapper over BREW ICipherFactory / ICipher1.
 */
class Cipher : public NonCopyable {
public:
    Cipher();
    ~Cipher();

    /**
     * @brief Initializes the cipher with algorithm, mode, direction, padding, key, and optional IV.
     */
    Result Initialize(void* platformContext,
                      CipherAlgorithm algorithm,
                      CipherMode mode,
                      CipherDirection direction,
                      CipherPadding padding,
                      const void* key,
                      uint32_t keyBytes,
                      const void* iv = nullptr,
                      uint32_t ivBytes = 0u);

    /**
     * @brief Releases the underlying ICipher1 instance.
     */
    void Shutdown();

    /**
     * @brief Processes input bytes through the cipher.
     * @param inData          Input plaintext/ciphertext buffer.
     * @param inBytes         Number of input bytes.
     * @param outData         Destination buffer.
     * @param outBufferBytes  Capacity of @p outData in bytes.
     * @param outBytesWritten Optional output receiving number of bytes written.
     */
    Result Process(const void* inData,
                   uint32_t inBytes,
                   void* outData,
                   uint32_t outBufferBytes,
                   uint32_t* outBytesWritten);

    /**
     * @brief Finalizes block cipher processing (flushing/verifying final padded block).
     */
    Result ProcessLast(void* outData, uint32_t outBufferBytes, uint32_t* outBytesWritten);

    /**
     * @brief Returns the cipher block size in bytes (1 for stream ciphers, 0 if uninitialized).
     */
    uint32_t GetBlockSize() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief RSA public-key helper over BREW IRSA (AEERSA.h).
 */
class Rsa {
public:
    /**
     * @brief Performs synchronous RSA modular exponentiation (in^exp mod mod).
     * @param platformContext Platform context (IShell* on Zeebo).
     * @param modulus         Big-endian modulus bytes.
     * @param modulusBytes    Length of @p modulus in bytes.
     * @param exponent        Big-endian exponent bytes.
     * @param exponentBytes   Length of @p exponent in bytes.
     * @param inData          Input block (must match modulus length).
     * @param inBytes         Length of @p inData in bytes.
     * @param outData         Destination buffer for output block.
     * @param outBufferBytes  Capacity of @p outData in bytes.
     * @param outBytesWritten Optional output receiving actual output length.
     */
    static Result ModExp(void* platformContext,
                         const void* modulus,
                         uint32_t modulusBytes,
                         const void* exponent,
                         uint32_t exponentBytes,
                         const void* inData,
                         uint32_t inBytes,
                         void* outData,
                         uint32_t outBufferBytes,
                         uint32_t* outBytesWritten = nullptr);
};

} // namespace security
} // namespace zsdk

#endif /* ZSDK_SECURITY_CRYPTO_HPP */
