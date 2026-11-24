/**
 * @file license.hpp
 * @brief Application license & cryptographic hash abstraction over BREW ILicense / IHashCtx.
 */

#ifndef ZSDK_SECURITY_LICENSE_HPP
#define ZSDK_SECURITY_LICENSE_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace security {

struct LicenseStatus {
    bool     valid;
    bool     expired;
    uint32_t remainingDays;
};

/**
 * @brief Queries BREW application licensing (AEELicense.h).
 */
class LicenseManager {
public:
    static Result CheckLicense(void* platformContext, LicenseStatus& outStatus);
};

} // namespace security
} // namespace zsdk

#endif /* ZSDK_SECURITY_LICENSE_HPP */
