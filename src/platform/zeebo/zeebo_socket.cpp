/**
 * @file zeebo_socket.cpp
 * @brief Qualcomm BREW / Zeebo ISockPort TCP/UDP socket bridge.
 */

#include "zsdk/net/socket.hpp"
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEESockPort.h"

namespace zsdk {
namespace net {

struct Socket::Impl {
    ISockPort* sock;
};

Socket::Socket()
    : m_impl(nullptr) {
}

Socket::~Socket() {
    Close();
}

Result Socket::Open(void* platformContext, SocketProtocol protocol) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Network, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->sock = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_SOCKPORT,
                              reinterpret_cast<void**>(&m_impl->sock)) != AEE_SUCCESS || !m_impl->sock) {
        ZSDK_LOG_ERROR("NET", "Failed to create AEECLSID_SOCKPORT");
        Close();
        return Result::NetworkError;
    }

    const int sockType = (protocol == SocketProtocol::Udp) ? AEE_SOCK_DGRAM : AEE_SOCK_STREAM;
    if (ISOCKPORT_OpenEx(m_impl->sock, AEE_AF_INET, sockType, 0) != AEE_SUCCESS) {
        ZSDK_LOG_ERROR("NET", "ISOCKPORT_OpenEx failed");
        Close();
        return Result::NetworkError;
    }

    return Result::Ok;
}

void Socket::Close() {
    if (!m_impl) {
        return;
    }
    if (m_impl->sock) {
        ISOCKPORT_Close(m_impl->sock);
        ISOCKPORT_Release(m_impl->sock);
        m_impl->sock = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Network, m_impl);
    m_impl = nullptr;
}

Result Socket::ConnectIPv4(uint32_t ipv4HostOrder, uint16_t port) {
    if (!m_impl || !m_impl->sock) {
        return Result::NotInitialized;
    }

    AEESockAddrStorage addr;
    MEMSET(&addr, 0, sizeof(addr));
    addr.inet.wFamily = AEE_AF_INET;
    addr.inet.port    = HTONS(port);
    addr.inet.addr    = HTONL(ipv4HostOrder);

    const AEEResult res = ISOCKPORT_Connect(m_impl->sock, &addr);
    if (res == AEE_SUCCESS) {
        return Result::Ok;
    }
    if (res == AEE_NET_WOULDBLOCK) {
        return Result::WouldBlock;
    }
    return Result::NetworkError;
}

int32_t Socket::Send(const void* data, uint32_t sizeBytes, Result* outStatus) {
    if (!m_impl || !m_impl->sock || !data) {
        if (outStatus) *outStatus = Result::InvalidArg;
        return -1;
    }
    const int32 sent = ISOCKPORT_Write(m_impl->sock, static_cast<const char*>(data),
                                       static_cast<int32>(sizeBytes));
    if (sent >= 0) {
        if (outStatus) *outStatus = Result::Ok;
        return sent;
    }
    if (sent == AEE_NET_WOULDBLOCK) {
        if (outStatus) *outStatus = Result::WouldBlock;
        return 0;
    }
    if (outStatus) *outStatus = Result::NetworkError;
    return -1;
}

int32_t Socket::Recv(void* buffer, uint32_t maxBytes, Result* outStatus) {
    if (!m_impl || !m_impl->sock || !buffer) {
        if (outStatus) *outStatus = Result::InvalidArg;
        return -1;
    }
    const int32 recvd = ISOCKPORT_Read(m_impl->sock, static_cast<char*>(buffer),
                                       static_cast<int32>(maxBytes));
    if (recvd >= 0) {
        if (outStatus) *outStatus = Result::Ok;
        return recvd;
    }
    if (recvd == AEE_NET_WOULDBLOCK) {
        if (outStatus) *outStatus = Result::WouldBlock;
        return 0;
    }
    if (outStatus) *outStatus = Result::NetworkError;
    return -1;
}

bool Socket::IsOpen() const {
    return m_impl && (m_impl->sock != nullptr);
}

} // namespace net
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
