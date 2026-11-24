/**
 * @file socket.hpp
 * @brief Non-blocking TCP/UDP socket abstraction over BREW ISockPort (AEESockPort.h).
 */

#ifndef ZSDK_NET_SOCKET_HPP
#define ZSDK_NET_SOCKET_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace net {

enum class SocketProtocol : uint8_t {
    Tcp = 0,    /**< AEE_SOCK_STREAM */
    Udp         /**< AEE_SOCK_DGRAM */
};

/**
 * @brief Non-blocking network socket wrapping BREW ISockPort*.
 */
class Socket : public NonCopyable {
public:
    Socket();
    ~Socket();

    Result Open(void* platformContext, SocketProtocol protocol);
    void   Close();

    /** @brief Connects to an IPv4 address in host byte order and port. */
    Result ConnectIPv4(uint32_t ipv4HostOrder, uint16_t port);

    /** @brief Sends bytes; returns bytes sent or negative Result on error. */
    int32_t Send(const void* data, uint32_t sizeBytes, Result* outStatus = nullptr);

    /** @brief Receives bytes; sets *outStatus = Result::WouldBlock if no data yet. */
    int32_t Recv(void* buffer, uint32_t maxBytes, Result* outStatus = nullptr);

    bool IsOpen() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace net
} // namespace zsdk

#endif /* ZSDK_NET_SOCKET_HPP */
