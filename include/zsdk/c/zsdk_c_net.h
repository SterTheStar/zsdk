/**
 * @file zsdk_c_net.h
 * @brief C99 ABI for ZSDK NetworkManager and non-blocking Socket.
 */

#ifndef ZSDK_C_NET_H
#define ZSDK_C_NET_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_net_mgr_t zsdk_net_mgr_t;
typedef struct zsdk_socket_t  zsdk_socket_t;
typedef struct zsdk_http_t    zsdk_http_t;

typedef enum zsdk_net_state_t {
    ZSDK_NET_DISCONNECTED = 0,
    ZSDK_NET_CONNECTING   = 1,
    ZSDK_NET_CONNECTED    = 2
} zsdk_net_state_t;

typedef enum zsdk_socket_proto_t {
    ZSDK_SOCK_TCP = 0,
    ZSDK_SOCK_UDP = 1
} zsdk_socket_proto_t;

/* NetworkManager */
ZSDK_C_API zsdk_net_mgr_t*  zsdk_net_create(void);
ZSDK_C_API void             zsdk_net_destroy(zsdk_net_mgr_t* mgr);
ZSDK_C_API zsdk_result_t    zsdk_net_init(zsdk_net_mgr_t* mgr, void* platform_ctx);
ZSDK_C_API void             zsdk_net_shutdown(zsdk_net_mgr_t* mgr);
ZSDK_C_API zsdk_net_state_t zsdk_net_get_state(const zsdk_net_mgr_t* mgr);
ZSDK_C_API int              zsdk_net_is_available(const zsdk_net_mgr_t* mgr);
ZSDK_C_API uint32_t         zsdk_net_get_local_ip(const zsdk_net_mgr_t* mgr);

/* HttpClient */
typedef void (*zsdk_http_cb_t)(int32_t http_status, const void* body, uint32_t body_bytes, void* user_data);

ZSDK_C_API zsdk_http_t*  zsdk_http_create(void);
ZSDK_C_API void          zsdk_http_destroy(zsdk_http_t* http);
ZSDK_C_API zsdk_result_t zsdk_http_init(zsdk_http_t* http, void* platform_ctx);
ZSDK_C_API void          zsdk_http_shutdown(zsdk_http_t* http);
ZSDK_C_API zsdk_result_t zsdk_http_get(zsdk_http_t* http, const char* url, zsdk_http_cb_t cb, void* user_data);

/* Socket */
ZSDK_C_API zsdk_socket_t* zsdk_socket_create(void);
ZSDK_C_API void           zsdk_socket_destroy(zsdk_socket_t* sock);
ZSDK_C_API zsdk_result_t  zsdk_socket_open(zsdk_socket_t* sock, void* platform_ctx, zsdk_socket_proto_t proto);
ZSDK_C_API void           zsdk_socket_close(zsdk_socket_t* sock);
ZSDK_C_API zsdk_result_t  zsdk_socket_connect_ipv4(zsdk_socket_t* sock, uint32_t ipv4_host_order, uint16_t port);
ZSDK_C_API int32_t        zsdk_socket_send(zsdk_socket_t* sock, const void* data, uint32_t size_bytes, zsdk_result_t* out_status);
ZSDK_C_API int32_t        zsdk_socket_recv(zsdk_socket_t* sock, void* buffer, uint32_t max_bytes, zsdk_result_t* out_status);
ZSDK_C_API int            zsdk_socket_is_open(const zsdk_socket_t* sock);

ZSDK_C_API void           zsdk_net_cancel_dns(zsdk_net_mgr_t* mgr);
ZSDK_C_API void           zsdk_http_set_max_response_bytes(zsdk_http_t* http, uint32_t max_bytes);
ZSDK_C_API zsdk_result_t  zsdk_http_post(zsdk_http_t* http, const char* url, const char* content_type,
                                         const void* body, uint32_t body_bytes, zsdk_http_cb_t cb, void* user_data);
ZSDK_C_API void           zsdk_http_cancel_pending(zsdk_http_t* http);

typedef void (*zsdk_dns_cb_t)(zsdk_result_t status, uint32_t ipv4_host_order, void* user_data);
ZSDK_C_API zsdk_result_t  zsdk_net_resolve_host(zsdk_net_mgr_t* mgr, const char* hostname,
                                                zsdk_dns_cb_t cb, void* user_data);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_NET_H */
