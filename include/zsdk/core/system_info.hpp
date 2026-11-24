/**
 * @file system_info.hpp
 * @brief Hardware, RAM/FS free queries, Battery, Backlight, and Locale info.
 */

#ifndef ZSDK_CORE_SYSTEM_INFO_HPP
#define ZSDK_CORE_SYSTEM_INFO_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {

struct BatteryStatus {
    bool    present;
    bool    charging;
    uint8_t levelPercent;   /**< 0..100 */
};

struct SystemMemoryInfo {
    uint32_t freeRamBytes;
    uint32_t totalRamBytes;
    uint32_t largestFreeBlockBytes;
    uint32_t freeFsBytes;
    uint32_t totalFsBytes;
};

/**
 * @brief Queries BREW system/device capabilities (IBattery, IBacklight, GETRAMFREE, GETFSFREE).
 */
class SystemInfo {
public:
    static Result GetMemoryInfo(SystemMemoryInfo& outInfo);
    static Result GetBatteryStatus(void* platformContext, BatteryStatus& outStatus);
    static Result SetBacklight(void* platformContext, bool enabled, uint8_t levelPercent = 100u);
    static uint32_t GetLanguageCode(void* platformContext);
};

} // namespace zsdk

#endif /* ZSDK_CORE_SYSTEM_INFO_HPP */
