/**
 * @file zeebo_crypto.cpp
 * @brief Qualcomm BREW / Zeebo Random, Hash (IHashCtx), HMAC, Cipher (ICipher1), and RSA (IRSA).
 */

#include "zsdk/security/crypto.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEESource.h"
#include "AEERandomSource.h"
#include "AEEClassIDs.h"
#include "AEEIHashCtx.h"
#include "AEEMD5Ctx.bid"
#include "AEESHA1Ctx.bid"
#include "AEEHMACMD5Ctx.bid"
#include "AEEHMACSHA1Ctx.bid"
#include "ICipher1.h"
#include "ICipherFactory.h"
#include "AEECipherFactory.bid"
#include "AEEBlockCipher.bid"
#include "AEEStreamCipher.bid"
#include "AEECipherWrapper.bid"
#include "AEERSA.h"

namespace zsdk {
namespace security {

namespace {

static Result MapHashAlgo(HashAlgorithm algo, bool isHmac, AEECLSID& outClsid, int& outCtxSize, uint32_t& outDigestSize) {
    if (algo == HashAlgorithm::Md5) {
        outClsid      = isHmac ? AEECLSID_HMAC_MD5CTX : AEECLSID_MD5CTX;
        outCtxSize    = isHmac ? AEECLSID_HMACMD5Ctx_SIZE : AEECLSID_MD5Ctx_SIZE;
        outDigestSize = MD5_DIGEST_LENGTH;
        return Result::Ok;
    }
    if (algo == HashAlgorithm::Sha1) {
        outClsid      = isHmac ? AEECLSID_HMAC_SHA1CTX : AEECLSID_SHA1CTX;
        outCtxSize    = isHmac ? AEECLSID_HMACSHA1Ctx_SIZE : AEECLSID_SHA1Ctx_SIZE;
        outDigestSize = SHA1_DIGEST_LENGTH;
        return Result::Ok;
    }
    return Result::InvalidArg;
}

static AEECLSID MapCipherAlgo(CipherAlgorithm algo) {
    switch (algo) {
        case CipherAlgorithm::Aes128:    return AEECLSID_BlockAES128;
        case CipherAlgorithm::Aes192:    return AEECLSID_BlockAES192;
        case CipherAlgorithm::Aes256:    return AEECLSID_BlockAES256;
        case CipherAlgorithm::Des:       return AEECLSID_BlockDES;
        case CipherAlgorithm::TripleDes: return AEECLSID_Block3DES;
        case CipherAlgorithm::Arc4:      return AEECLSID_StreamARC4;
        default:                         return 0;
    }
}

static AEECLSID MapCipherMode(CipherMode mode) {
    switch (mode) {
        case CipherMode::Ecb:    return AEECLSID_CipherModeECB;
        case CipherMode::Cbc:    return AEECLSID_CipherModeCBC;
        case CipherMode::Cfb:    return AEECLSID_CipherModeCFB;
        case CipherMode::Ctr:    return AEECLSID_CipherModeCTR;
        case CipherMode::Stream:
        default:                 return 0;
    }
}

static int MapCipherPadding(CipherPadding pad) {
    switch (pad) {
        case CipherPadding::Zero:  return CIPHER_PADDING_ZERO;
        case CipherPadding::Pkcs7: return CIPHER_PADDING_RFC2630;
        case CipherPadding::None:
        default:                   return CIPHER_PADDING_NONE;
    }
}

} // namespace

/* --------------------------------------------------------------------------
 * Random
 * -------------------------------------------------------------------------- */
Result Random::GetBytes(void* platformContext, void* outBuffer, uint32_t sizeBytes) {
    if (!outBuffer || sizeBytes == 0u) {
        return Result::InvalidArg;
    }

    IShell* shell = static_cast<IShell*>(platformContext);
    if (shell) {
        ISource* rng = nullptr;
        if (ISHELL_CreateInstance(shell, AEECLSID_RANDOM,
                                  reinterpret_cast<void**>(&rng)) == AEE_SUCCESS && rng) {
            const int32 res = ISOURCE_Read(rng, static_cast<char*>(outBuffer),
                                           static_cast<int32>(sizeBytes));
            ISOURCE_Release(rng);
            if (res == static_cast<int32>(sizeBytes)) {
                return Result::Ok;
            }
        }
    }

    /* Fallback to BREW GETRAND */
    GETRAND(static_cast<byte*>(outBuffer), static_cast<int>(sizeBytes));
    return Result::Ok;
}

uint32_t Random::GetUInt32(void* platformContext) {
    uint32_t val = 0u;
    GetBytes(platformContext, &val, sizeof(val));
    return val;
}

/* --------------------------------------------------------------------------
 * Hash (IHashCtx: MD5 / SHA-1)
 * -------------------------------------------------------------------------- */
struct Hash::Impl {
    IHashCtx* hashCtx;
    int       ctxSize;
    uint32_t  digestSize;
    uint8_t   ctxBuf[AEECLSID_SHA1Ctx_SIZE];
};

Hash::Hash()
    : m_impl(nullptr) {
}

Hash::~Hash() {
    if (m_impl) {
        if (m_impl->hashCtx) {
            IHashCtx_Release(m_impl->hashCtx);
        }
        debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
        m_impl = nullptr;
    }
}

Result Hash::Begin(void* platformContext, HashAlgorithm algorithm) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    AEECLSID clsid     = 0;
    int ctxSize        = 0;
    uint32_t digSize   = 0u;
    if (Failed(MapHashAlgo(algorithm, false, clsid, ctxSize, digSize))) {
        return Result::InvalidArg;
    }

    if (!m_impl) {
        m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
            debug::MemorySubsystem::Core, sizeof(Impl)));
        if (!m_impl) {
            return Result::OutOfMemory;
        }
        MEMSET(m_impl, 0, sizeof(Impl));
    } else if (m_impl->hashCtx) {
        IHashCtx_Release(m_impl->hashCtx);
        m_impl->hashCtx = nullptr;
    }

    if (ISHELL_CreateInstance(shell, clsid,
                              reinterpret_cast<void**>(&m_impl->hashCtx)) != AEE_SUCCESS ||
        !m_impl->hashCtx) {
        return Result::NotSupported;
    }

    m_impl->ctxSize    = ctxSize;
    m_impl->digestSize = digSize;
    IHashCtx_Init(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize);
    return Result::Ok;
}

Result Hash::Update(const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->hashCtx) {
        return Result::NotInitialized;
    }
    if (!data && sizeBytes > 0u) {
        return Result::InvalidArg;
    }
    if (sizeBytes > 0u) {
        IHashCtx_Update(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize,
                        data, static_cast<int>(sizeBytes));
    }
    return Result::Ok;
}

Result Hash::Final(void* outDigest, uint32_t digestBufferBytes, uint32_t* outDigestBytes) {
    if (!m_impl || !m_impl->hashCtx) {
        return Result::NotInitialized;
    }
    if (!outDigest || digestBufferBytes < m_impl->digestSize) {
        return Result::InvalidArg;
    }

    int len = static_cast<int>(digestBufferBytes);
    const int res = IHashCtx_Final(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize,
                                   static_cast<byte*>(outDigest), &len);
    IHashCtx_Release(m_impl->hashCtx);
    m_impl->hashCtx = nullptr;

    if (res != SUCCESS) {
        return Result::Error;
    }
    if (outDigestBytes) {
        *outDigestBytes = static_cast<uint32_t>(len);
    }
    return Result::Ok;
}

Result Hash::Compute(void* platformContext,
                     HashAlgorithm algorithm,
                     const void* data,
                     uint32_t sizeBytes,
                     void* outDigest,
                     uint32_t digestBufferBytes,
                     uint32_t* outDigestBytes) {
    Hash hasher;
    Result r = hasher.Begin(platformContext, algorithm);
    if (Failed(r)) {
        return r;
    }
    r = hasher.Update(data, sizeBytes);
    if (Failed(r)) {
        return r;
    }
    return hasher.Final(outDigest, digestBufferBytes, outDigestBytes);
}

/* --------------------------------------------------------------------------
 * Hmac (IHashCtx: HMAC-MD5 / HMAC-SHA1)
 * -------------------------------------------------------------------------- */
struct Hmac::Impl {
    IHashCtx* hashCtx;
    int       ctxSize;
    uint32_t  digestSize;
    uint8_t   ctxBuf[AEECLSID_HMACSHA1Ctx_SIZE];
};

Hmac::Hmac()
    : m_impl(nullptr) {
}

Hmac::~Hmac() {
    if (m_impl) {
        if (m_impl->hashCtx) {
            IHashCtx_Release(m_impl->hashCtx);
        }
        MEMSET(m_impl->ctxBuf, 0, sizeof(m_impl->ctxBuf));
        debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
        m_impl = nullptr;
    }
}

Result Hmac::Begin(void* platformContext, HashAlgorithm algorithm, const void* key, uint32_t keyBytes) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || (!key && keyBytes > 0u)) {
        return Result::InvalidArg;
    }

    AEECLSID clsid   = 0;
    int ctxSize      = 0;
    uint32_t digSize = 0u;
    if (Failed(MapHashAlgo(algorithm, true, clsid, ctxSize, digSize))) {
        return Result::InvalidArg;
    }

    if (!m_impl) {
        m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
            debug::MemorySubsystem::Core, sizeof(Impl)));
        if (!m_impl) {
            return Result::OutOfMemory;
        }
        MEMSET(m_impl, 0, sizeof(Impl));
    } else if (m_impl->hashCtx) {
        IHashCtx_Release(m_impl->hashCtx);
        m_impl->hashCtx = nullptr;
    }

    if (ISHELL_CreateInstance(shell, clsid,
                              reinterpret_cast<void**>(&m_impl->hashCtx)) != AEE_SUCCESS ||
        !m_impl->hashCtx) {
        return Result::NotSupported;
    }

    m_impl->ctxSize    = ctxSize;
    m_impl->digestSize = digSize;
    IHashCtx_Init(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize);
    if (IHashCtx_SetKey(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize,
                        static_cast<const byte*>(key), static_cast<int>(keyBytes)) != SUCCESS) {
        IHashCtx_Release(m_impl->hashCtx);
        m_impl->hashCtx = nullptr;
        return Result::Error;
    }
    return Result::Ok;
}

Result Hmac::Update(const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->hashCtx) {
        return Result::NotInitialized;
    }
    if (!data && sizeBytes > 0u) {
        return Result::InvalidArg;
    }
    if (sizeBytes > 0u) {
        IHashCtx_Update(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize,
                        data, static_cast<int>(sizeBytes));
    }
    return Result::Ok;
}

Result Hmac::Final(void* outMac, uint32_t macBufferBytes, uint32_t* outMacBytes) {
    if (!m_impl || !m_impl->hashCtx) {
        return Result::NotInitialized;
    }
    if (!outMac || macBufferBytes < m_impl->digestSize) {
        return Result::InvalidArg;
    }

    int len = static_cast<int>(macBufferBytes);
    const int res = IHashCtx_Final(m_impl->hashCtx, m_impl->ctxBuf, m_impl->ctxSize,
                                   static_cast<byte*>(outMac), &len);
    IHashCtx_Release(m_impl->hashCtx);
    m_impl->hashCtx = nullptr;

    if (res != SUCCESS) {
        return Result::Error;
    }
    if (outMacBytes) {
        *outMacBytes = static_cast<uint32_t>(len);
    }
    return Result::Ok;
}

Result Hmac::Compute(void* platformContext,
                     HashAlgorithm algorithm,
                     const void* key,
                     uint32_t keyBytes,
                     const void* data,
                     uint32_t sizeBytes,
                     void* outMac,
                     uint32_t macBufferBytes,
                     uint32_t* outMacBytes) {
    Hmac hmac;
    Result r = hmac.Begin(platformContext, algorithm, key, keyBytes);
    if (Failed(r)) {
        return r;
    }
    r = hmac.Update(data, sizeBytes);
    if (Failed(r)) {
        return r;
    }
    return hmac.Final(outMac, macBufferBytes, outMacBytes);
}

/* --------------------------------------------------------------------------
 * Cipher (ICipherFactory / ICipher1)
 * -------------------------------------------------------------------------- */
struct Cipher::Impl {
    ICipher1* cipher;
    uint32_t  blockSize;
};

Cipher::Cipher()
    : m_impl(nullptr) {
}

Cipher::~Cipher() {
    Shutdown();
}

Result Cipher::Initialize(void* platformContext,
                          CipherAlgorithm algorithm,
                          CipherMode mode,
                          CipherDirection direction,
                          CipherPadding padding,
                          const void* key,
                          uint32_t keyBytes,
                          const void* iv,
                          uint32_t ivBytes) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !key || keyBytes == 0u) {
        return Result::InvalidArg;
    }
    if (m_impl) {
        return Result::AlreadyInitialized;
    }

    const AEECLSID cipherClsid = MapCipherAlgo(algorithm);
    if (cipherClsid == 0) {
        return Result::InvalidArg;
    }

    ICipherFactory* factory = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_CipherFactory,
                              reinterpret_cast<void**>(&factory)) != AEE_SUCCESS || !factory) {
        return Result::NotSupported;
    }

    ICipher1* cipher = nullptr;
    const int dir    = (direction == CipherDirection::Decrypt) ? CIPHER_DIRECTION_DECRYPT : CIPHER_DIRECTION_ENCRYPT;
    const AEECLSID modeClsid = MapCipherMode(mode);
    const int pad            = MapCipherPadding(padding);

    const int createRes = ICIPHERFACTORY_CreateCipher(factory, cipherClsid, dir, modeClsid, pad, &cipher);
    ICIPHERFACTORY_Release(factory);
    if (createRes != SUCCESS || !cipher) {
        return Result::NotSupported;
    }

    if (ICIPHER1_SetParam(cipher, CIPHER_PARAM_KEY, key, static_cast<unsigned>(keyBytes)) != SUCCESS) {
        ICIPHER1_Release(cipher);
        return Result::InvalidArg;
    }

    if (iv && ivBytes > 0u) {
        if (ICIPHER1_SetParam(cipher, CIPHER_PARAM_IV, iv, static_cast<unsigned>(ivBytes)) != SUCCESS) {
            ICIPHER1_Release(cipher);
            return Result::InvalidArg;
        }
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Core, sizeof(Impl)));
    if (!m_impl) {
        ICIPHER1_Release(cipher);
        return Result::OutOfMemory;
    }
    m_impl->cipher    = cipher;
    m_impl->blockSize = 1u;

    int bsz          = 1;
    unsigned bszLen  = sizeof(bsz);
    if (ICIPHER1_GetParam(cipher, CIPHER_PARAM_BLOCKSIZE, &bsz, &bszLen) == SUCCESS && bsz > 0) {
        m_impl->blockSize = static_cast<uint32_t>(bsz);
    }
    return Result::Ok;
}

void Cipher::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->cipher) {
        ICIPHER1_Release(m_impl->cipher);
        m_impl->cipher = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
    m_impl = nullptr;
}

Result Cipher::Process(const void* inData,
                       uint32_t inBytes,
                       void* outData,
                       uint32_t outBufferBytes,
                       uint32_t* outBytesWritten) {
    if (!m_impl || !m_impl->cipher) {
        return Result::NotInitialized;
    }
    if ((!inData && inBytes > 0u) || !outData) {
        return Result::InvalidArg;
    }
    unsigned outLen = static_cast<unsigned>(outBufferBytes);
    const int res = ICIPHER1_Process(m_impl->cipher,
                                     static_cast<const byte*>(inData),
                                     static_cast<unsigned>(inBytes),
                                     static_cast<byte*>(outData),
                                     &outLen);
    if (res != SUCCESS) {
        return Result::Error;
    }
    if (outBytesWritten) {
        *outBytesWritten = static_cast<uint32_t>(outLen);
    }
    return Result::Ok;
}

Result Cipher::ProcessLast(void* outData, uint32_t outBufferBytes, uint32_t* outBytesWritten) {
    if (!m_impl || !m_impl->cipher) {
        return Result::NotInitialized;
    }
    if (!outData && outBufferBytes > 0u) {
        return Result::InvalidArg;
    }
    unsigned outLen = static_cast<unsigned>(outBufferBytes);
    const int res = ICIPHER1_ProcessLast(m_impl->cipher,
                                         static_cast<byte*>(outData),
                                         &outLen);
    if (res != SUCCESS) {
        return Result::Error;
    }
    if (outBytesWritten) {
        *outBytesWritten = static_cast<uint32_t>(outLen);
    }
    return Result::Ok;
}

uint32_t Cipher::GetBlockSize() const {
    return m_impl ? m_impl->blockSize : 0u;
}

/* --------------------------------------------------------------------------
 * Rsa (IRSA)
 * -------------------------------------------------------------------------- */
Result Rsa::ModExp(void* platformContext,
                   const void* modulus,
                   uint32_t modulusBytes,
                   const void* exponent,
                   uint32_t exponentBytes,
                   const void* inData,
                   uint32_t inBytes,
                   void* outData,
                   uint32_t outBufferBytes,
                   uint32_t* outBytesWritten) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !modulus || modulusBytes == 0u || !exponent || exponentBytes == 0u ||
        !inData || inBytes == 0u || !outData) {
        return Result::InvalidArg;
    }

    IRSA* rsa = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_RSA,
                              reinterpret_cast<void**>(&rsa)) != AEE_SUCCESS || !rsa) {
        return Result::NotSupported;
    }

    if (IRSA_Init(rsa,
                  static_cast<const byte*>(modulus), static_cast<int>(modulusBytes),
                  static_cast<const byte*>(exponent), static_cast<int>(exponentBytes)) != SUCCESS) {
        IRSA_Release(rsa);
        return Result::InvalidArg;
    }

    byte* rsaOut = nullptr;
    int rsaOutLen = 0;
    const int res = IRSA_RSA(rsa,
                             static_cast<const byte*>(inData), static_cast<int>(inBytes),
                             &rsaOut, &rsaOutLen, 0);
    if (res != SUCCESS || !rsaOut || rsaOutLen <= 0) {
        IRSA_Release(rsa);
        return Result::Error;
    }

    if (static_cast<uint32_t>(rsaOutLen) > outBufferBytes) {
        IRSA_Release(rsa);
        return Result::InvalidArg;
    }

    MEMCPY(outData, rsaOut, static_cast<uint32_t>(rsaOutLen));
    if (outBytesWritten) {
        *outBytesWritten = static_cast<uint32_t>(rsaOutLen);
    }
    IRSA_Release(rsa);
    return Result::Ok;
}

} // namespace security
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
