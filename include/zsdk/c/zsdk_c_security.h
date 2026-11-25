/**
 * @file zsdk_c_security.h
 * @brief C99 ABI for ZSDK Random, Hash (MD5/SHA-1), and LicenseManager.
 */

#ifndef ZSDK_C_SECURITY_H
#define ZSDK_C_SECURITY_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef enum zsdk_hash_algo_t {
    ZSDK_HASH_MD5  = 0,
    ZSDK_HASH_SHA1 = 1
} zsdk_hash_algo_t;

ZSDK_C_API zsdk_result_t zsdk_random_bytes(void* platform_ctx, void* out_buf, uint32_t size_bytes);
ZSDK_C_API uint32_t      zsdk_random_u32(void* platform_ctx);

ZSDK_C_API zsdk_result_t zsdk_hash_compute(void* platform_ctx, zsdk_hash_algo_t algo,
                                           const void* data, uint32_t size_bytes,
                                           void* out_digest, uint32_t digest_buf_bytes, uint32_t* out_digest_bytes);

ZSDK_C_API zsdk_result_t zsdk_hmac_compute(void* platform_ctx, zsdk_hash_algo_t algo,
                                           const void* key, uint32_t key_bytes,
                                           const void* data, uint32_t size_bytes,
                                           void* out_mac, uint32_t mac_buf_bytes, uint32_t* out_mac_bytes);

ZSDK_C_API zsdk_result_t zsdk_license_check(void* platform_ctx, int* out_valid,
                                            int* out_expired, uint32_t* out_remaining_days);

ZSDK_C_API zsdk_result_t zsdk_rsa_mod_exp(void* platform_ctx,
                                          const void* modulus, uint32_t modulus_bytes,
                                          const void* exponent, uint32_t exponent_bytes,
                                          const void* in_data, uint32_t in_bytes,
                                          void* out_data, uint32_t out_buf_bytes, uint32_t* out_written);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_SECURITY_H */
