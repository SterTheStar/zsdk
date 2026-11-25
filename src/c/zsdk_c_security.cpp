/**
 * @file zsdk_c_security.cpp
 * @brief C ABI bridge for Random, Hash, and LicenseManager.
 */

#include "zsdk/c/zsdk_c_security.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/security/crypto.hpp"
#include "zsdk/security/license.hpp"

extern "C" {

zsdk_result_t zsdk_random_bytes(void* platform_ctx, void* out_buf, uint32_t size_bytes) {
    return zsdk::c_abi::ToCResult(zsdk::security::Random::GetBytes(platform_ctx, out_buf, size_bytes));
}

uint32_t zsdk_random_u32(void* platform_ctx) {
    return zsdk::security::Random::GetUInt32(platform_ctx);
}

zsdk_result_t zsdk_hash_compute(void* platform_ctx, zsdk_hash_algo_t algo,
                                const void* data, uint32_t size_bytes,
                                void* out_digest, uint32_t digest_buf_bytes, uint32_t* out_digest_bytes) {
    return zsdk::c_abi::ToCResult(zsdk::security::Hash::Compute(
        platform_ctx,
        static_cast<zsdk::security::HashAlgorithm>(algo),
        data, size_bytes, out_digest, digest_buf_bytes, out_digest_bytes));
}

zsdk_result_t zsdk_hmac_compute(void* platform_ctx, zsdk_hash_algo_t algo,
                                const void* key, uint32_t key_bytes,
                                const void* data, uint32_t size_bytes,
                                void* out_mac, uint32_t mac_buf_bytes, uint32_t* out_mac_bytes) {
    return zsdk::c_abi::ToCResult(zsdk::security::Hmac::Compute(
        platform_ctx,
        static_cast<zsdk::security::HashAlgorithm>(algo),
        key, key_bytes, data, size_bytes, out_mac, mac_buf_bytes, out_mac_bytes));
}

zsdk_result_t zsdk_license_check(void* platform_ctx, int* out_valid,
                                 int* out_expired, uint32_t* out_remaining_days) {
    zsdk::security::LicenseStatus st = { false, false, 0u };
    zsdk::Result r = zsdk::security::LicenseManager::CheckLicense(platform_ctx, st);
    if (out_valid)          *out_valid          = st.valid ? 1 : 0;
    if (out_expired)        *out_expired        = st.expired ? 1 : 0;
    if (out_remaining_days) *out_remaining_days = st.remainingDays;
    return zsdk::c_abi::ToCResult(r);
}

} /* extern "C" */
