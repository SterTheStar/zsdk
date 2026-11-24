/**
 * @file zeebo_security_extra.cpp
 * @brief Qualcomm BREW / Zeebo LicenseManager (AEELicense.h).
 */

#include "zsdk/security/license.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEELicense.h"

namespace zsdk {
namespace security {

Result LicenseManager::CheckLicense(void* platformContext, LicenseStatus& outStatus) {
    outStatus = LicenseStatus{false, false, 0u};
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    ILicense* lic = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_LICENSE,
                              reinterpret_cast<void**>(&lic)) != AEE_SUCCESS || !lic) {
        return Result::NotSupported;
    }

    uint32 infoVal = 0u;
    outStatus.expired = (ILICENSE_IsExpired(lic) != FALSE);
    if (ILICENSE_GetInfo(lic, &infoVal) == SUCCESS) {
        outStatus.valid = !outStatus.expired;
        outStatus.remainingDays = infoVal;
    }
    ILICENSE_Release(lic);
    return Result::Ok;
}

} // namespace security
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
