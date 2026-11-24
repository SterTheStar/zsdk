/**
 * @file zeebo_system_info.cpp
 * @brief Qualcomm BREW / Zeebo SystemInfo (RAM/FS free, IBattery, IBacklight, Locale).
 */

#include "zsdk/core/system_info.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEBattery.h"
#include "AEEBacklight.h"

namespace zsdk {

Result SystemInfo::GetMemoryInfo(SystemMemoryInfo& outInfo) {
    uint32 totalRam = 0u, largestBlock = 0u, totalFs = 0u;
    outInfo.freeRamBytes          = GETRAMFREE(&totalRam, &largestBlock);
    outInfo.totalRamBytes         = totalRam;
    outInfo.largestFreeBlockBytes = largestBlock;
    outInfo.freeFsBytes           = GETFSFREE(&totalFs);
    outInfo.totalFsBytes          = totalFs;
    return Result::Ok;
}

Result SystemInfo::GetBatteryStatus(void* platformContext, BatteryStatus& outStatus) {
    outStatus = BatteryStatus{false, false, 100u};
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    IBattery* bat = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_BATTERY,
                              reinterpret_cast<void**>(&bat)) != AEE_SUCCESS || !bat) {
        return Result::NotSupported;
    }

    AEEBatteryStatus st = 0;
    const int res = IBATTERY_GetBatteryStatus(bat, &st);
    uint32 level = 100u;
    IBATTERY_GetBatteryLevel(bat, &level);
    IBATTERY_Release(bat);

    if (res != SUCCESS) {
        return Result::Error;
    }

    outStatus.present      = true;
    outStatus.charging     = (st == AEEBATTERY_STATUS_POWERDOWN);
    outStatus.levelPercent = static_cast<uint8_t>(level);
    return Result::Ok;
}

Result SystemInfo::SetBacklight(void* platformContext, bool enabled, uint8_t levelPercent) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    IBacklight* bl = nullptr;
    if (ISHELL_CreateInstance(shell, AEECLSID_BACKLIGHT_DISPLAY1,
                              reinterpret_cast<void**>(&bl)) != AEE_SUCCESS || !bl) {
        return Result::NotSupported;
    }

    const int res = enabled
                        ? IBACKLIGHT_Enable(bl)
                        : IBACKLIGHT_Disable(bl);
    if (enabled && res == SUCCESS) {
        IBACKLIGHT_SetBrightnessLevel(bl, levelPercent);
    }
    IBACKLIGHT_Release(bl);
    return (res == SUCCESS) ? Result::Ok : Result::Error;
}

uint32_t SystemInfo::GetLanguageCode(void* platformContext) {
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return 0u;
    }
    AEEDeviceInfo info;
    MEMSET(&info, 0, sizeof(info));
    info.wStructSize = sizeof(info);
    ISHELL_GetDeviceInfo(shell, &info);
    return static_cast<uint32_t>(info.dwLang);
}

} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
