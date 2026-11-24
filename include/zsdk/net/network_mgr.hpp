/**
 * @file network_mgr.hpp
 * @brief Network, Wi-Fi, and DNS resolution over BREW INetMgr / IWIFI.
 */

#ifndef ZSDK_NET_NETWORK_MGR_HPP
#define ZSDK_NET_NETWORK_MGR_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace net {

/**
 * @brief High-level network connection state.
 */
enum class NetworkState : uint8_t {
    Disconnected = 0,
    Connecting,
    Connected
};

/**
 * @brief Callback invoked when an asynchronous DNS lookup completes.
 * @param result        Result::Ok on success, or Result::NetworkError on failure.
 * @param ipv4HostOrder Array of resolved IPv4 addresses in host byte order (up to 4).
 * @param addrCount     Number of valid addresses in @p ipv4HostOrder.
 * @param userData      User-supplied context pointer.
 */
typedef void (*DnsResolveCallback)(Result result,
                                   const uint32_t* ipv4HostOrder,
                                   uint32_t addrCount,
                                   void* userData);

/**
 * @brief Manages network availability, Wi-Fi state, and DNS resolution (INetMgr / IWIFI).
 */
class NetworkManager : public NonCopyable {
public:
    NetworkManager();
    ~NetworkManager();

    /**
     * @brief Initializes INetMgr and IWIFI instances from the platform context (IShell* on Zeebo).
     */
    Result Initialize(void* platformContext);

    /**
     * @brief Cancels any pending DNS lookup and releases BREW network interfaces.
     */
    void   Shutdown();

    /**
     * @brief Returns the current network connection state.
     */
    NetworkState GetState() const;

    /**
     * @brief Returns true if the Wi-Fi interface is available on this device.
     */
    bool         IsWifiAvailable() const;

    /**
     * @brief Returns the local device IPv4 address in host byte order, or 0 if unavailable.
     */
    uint32_t     GetLocalIpAddress() const;

    /**
     * @brief Asynchronously resolves a hostname to IPv4 addresses via INETMGR_GetHostByName.
     * @param hostname Null-terminated domain name (e.g. "example.com").
     * @param callback Completion callback invoked with resolved IPv4 addresses.
     * @param userData User-supplied context pointer passed to @p callback.
     */
    Result       ResolveHost(const char* hostname, DnsResolveCallback callback, void* userData);

    /**
     * @brief Cancels any in-flight DNS resolution request.
     */
    void         CancelDns();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace net
} // namespace zsdk

#endif /* ZSDK_NET_NETWORK_MGR_HPP */
