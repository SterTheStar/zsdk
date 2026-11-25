/**
 * @file zsdk_c_net.cpp
 * @brief C ABI bridge for NetworkManager and Socket.
 */

#include "zsdk/c/zsdk_c_net.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/net/network_mgr.hpp"
#include "zsdk/net/socket.hpp"
#include "zsdk/net/http_client.hpp"

struct zsdk_net_mgr_t {
    zsdk::net::NetworkManager mgr;
};

struct zsdk_socket_t {
    zsdk::net::Socket sock;
};

struct zsdk_http_t {
    zsdk::net::HttpClient http;
};

extern "C" {

zsdk_net_mgr_t* zsdk_net_create(void) {
    return zsdk::c_abi::Alloc<zsdk_net_mgr_t>(zsdk::debug::MemorySubsystem::Network);
}

void zsdk_net_destroy(zsdk_net_mgr_t* mgr) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Network, mgr);
}

zsdk_result_t zsdk_net_init(zsdk_net_mgr_t* mgr, void* platform_ctx) {
    if (!mgr) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(mgr->mgr.Initialize(platform_ctx));
}

void zsdk_net_shutdown(zsdk_net_mgr_t* mgr) {
    if (mgr) mgr->mgr.Shutdown();
}

zsdk_net_state_t zsdk_net_get_state(const zsdk_net_mgr_t* mgr) {
    if (!mgr) return ZSDK_NET_DISCONNECTED;
    return static_cast<zsdk_net_state_t>(mgr->mgr.GetState());
}

int zsdk_net_is_available(const zsdk_net_mgr_t* mgr) {
    return (mgr && mgr->mgr.IsWifiAvailable()) ? 1 : 0;
}

uint32_t zsdk_net_get_local_ip(const zsdk_net_mgr_t* mgr) {
    return mgr ? mgr->mgr.GetLocalIpAddress() : 0u;
}

zsdk_http_t* zsdk_http_create(void) {
    return zsdk::c_abi::Alloc<zsdk_http_t>(zsdk::debug::MemorySubsystem::Network);
}

void zsdk_http_destroy(zsdk_http_t* http) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Network, http);
}

zsdk_result_t zsdk_http_init(zsdk_http_t* http, void* platform_ctx) {
    if (!http) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(http->http.Initialize(platform_ctx));
}

void zsdk_http_shutdown(zsdk_http_t* http) {
    if (http) http->http.Shutdown();
}

zsdk_result_t zsdk_http_get(zsdk_http_t* http, const char* url, zsdk_http_cb_t cb, void* user_data) {
    if (!http) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(http->http.Get(url, cb, user_data));
}

zsdk_socket_t* zsdk_socket_create(void) {
    return zsdk::c_abi::Alloc<zsdk_socket_t>(zsdk::debug::MemorySubsystem::Network);
}

void zsdk_socket_destroy(zsdk_socket_t* sock) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Network, sock);
}

zsdk_result_t zsdk_socket_open(zsdk_socket_t* sock, void* platform_ctx, zsdk_socket_proto_t proto) {
    if (!sock) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sock->sock.Open(platform_ctx, static_cast<zsdk::net::SocketProtocol>(proto)));
}

void zsdk_socket_close(zsdk_socket_t* sock) {
    if (sock) sock->sock.Close();
}

zsdk_result_t zsdk_socket_connect_ipv4(zsdk_socket_t* sock, uint32_t ipv4_host_order, uint16_t port) {
    if (!sock) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sock->sock.ConnectIPv4(ipv4_host_order, port));
}

int32_t zsdk_socket_send(zsdk_socket_t* sock, const void* data, uint32_t size_bytes, zsdk_result_t* out_status) {
    if (!sock) {
        if (out_status) *out_status = ZSDK_C_ERR_INVALID_ARG;
        return -1;
    }
    zsdk::Result st = zsdk::Result::Ok;
    int32_t sent = sock->sock.Send(data, size_bytes, &st);
    if (out_status) *out_status = zsdk::c_abi::ToCResult(st);
    return sent;
}

int32_t zsdk_socket_recv(zsdk_socket_t* sock, void* buffer, uint32_t max_bytes, zsdk_result_t* out_status) {
    if (!sock) {
        if (out_status) *out_status = ZSDK_C_ERR_INVALID_ARG;
        return -1;
    }
    zsdk::Result st = zsdk::Result::Ok;
    int32_t recvd = sock->sock.Recv(buffer, max_bytes, &st);
    if (out_status) *out_status = zsdk::c_abi::ToCResult(st);
    return recvd;
}

int zsdk_socket_is_open(const zsdk_socket_t* sock) {
    return (sock && sock->sock.IsOpen()) ? 1 : 0;
}

void zsdk_net_cancel_dns(zsdk_net_mgr_t* mgr) {
    if (mgr) mgr->mgr.CancelDns();
}

void zsdk_http_set_max_response_bytes(zsdk_http_t* http, uint32_t max_bytes) {
    if (http) http->http.SetMaxResponseBytes(max_bytes);
}

zsdk_result_t zsdk_http_post(zsdk_http_t* http, const char* url, const char* content_type,
                             const void* body, uint32_t body_bytes, zsdk_http_cb_t cb, void* user_data) {
    if (!http) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(http->http.Post(url, content_type, body, body_bytes, cb, user_data));
}

void zsdk_http_cancel_pending(zsdk_http_t* http) {
    if (http) http->http.CancelPending();
}

namespace {
struct ZsdkCDnsBridge {
    zsdk_dns_cb_t cb;
    void*         userData;
};
void DnsThunk(zsdk::Result status, const uint32_t* ipv4Addrs, uint32_t addrCount, void* ud) {
    auto* b = static_cast<ZsdkCDnsBridge*>(ud);
    uint32_t firstIp = (ipv4Addrs && addrCount > 0u) ? ipv4Addrs[0] : 0u;
    if (b && b->cb) b->cb(zsdk::c_abi::ToCResult(status), firstIp, b->userData);
    delete b;
}
} // namespace

zsdk_result_t zsdk_net_resolve_host(zsdk_net_mgr_t* mgr, const char* hostname,
                                    zsdk_dns_cb_t cb, void* user_data) {
    if (!mgr || !hostname || !cb) return ZSDK_C_ERR_INVALID_ARG;
    auto* bridge = new ZsdkCDnsBridge{cb, user_data};
    zsdk::Result r = mgr->mgr.ResolveHost(hostname, DnsThunk, bridge);
    if (zsdk::Failed(r)) {
        delete bridge;
    }
    return zsdk::c_abi::ToCResult(r);
}

} /* extern "C" */
