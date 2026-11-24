/**
 * @file bluetooth.hpp
 * @brief Bluetooth manager and Serial Port Profile (SPP) abstraction over BREW IBT / IBTSPPClient.
 */

#ifndef ZSDK_NET_BLUETOOTH_HPP
#define ZSDK_NET_BLUETOOTH_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace net {

/**
 * @brief Bluetooth Serial Port Profile (SPP) connection state.
 */
enum class SppState : uint8_t {
    Closed = 0,
    Opening,
    OpenError,
    Opened,
    Connected,
    Disconnecting,
    Disconnected
};

/**
 * @brief Bluetooth radio status and Serial Port Profile (SPP) wrapper over BREW IBT*.
 */
class BluetoothManager : public NonCopyable {
public:
    BluetoothManager();
    ~BluetoothManager();

    /**
     * @brief Initializes Bluetooth manager using the platform context (IShell* on Zeebo).
     */
    Result Initialize(void* platformContext);

    /**
     * @brief Closes any open SPP stream and releases BREW Bluetooth interfaces.
     */
    void   Shutdown();

    /**
     * @brief Returns true if the BREW IBT interface is supported and instantiated.
     */
    bool IsSupported() const;

    /**
     * @brief Queries IBT_GetRadioState and returns true if the Bluetooth radio is powered on.
     */
    bool IsRadioEnabled() const;

    /**
     * @brief Opens a Bluetooth SPP client port if IBTSPPClient is available on the device.
     * @param portName Null-terminated SPP port/service name (or nullptr for default).
     */
    Result   OpenSpp(const char* portName);

    /**
     * @brief Closes and releases the active SPP client stream.
     */
    void     CloseSpp();

    /**
     * @brief Queries the current SPP stream state via IBTSPPCLI_GetStatus.
     */
    SppState GetSppState() const;

    /**
     * @brief Reads up to @p maxBytes from the open SPP stream.
     * @return Number of bytes read (>= 0), or negative on wait/error.
     */
    int32_t  ReadSpp(void* outBuffer, uint32_t maxBytes);

    /**
     * @brief Writes up to @p sizeBytes to the open SPP stream.
     * @return Number of bytes written (>= 0), or negative on wait/error.
     */
    int32_t  WriteSpp(const void* data, uint32_t sizeBytes);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace net
} // namespace zsdk

#endif /* ZSDK_NET_BLUETOOTH_HPP */
