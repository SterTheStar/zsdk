/**
 * @file http_client.hpp
 * @brief Asynchronous HTTP/HTTPS client over BREW IWeb (AEEWeb.h).
 */

#ifndef ZSDK_NET_HTTP_CLIENT_HPP
#define ZSDK_NET_HTTP_CLIENT_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace net {

/**
 * @brief Callback invoked when an HTTP request completes (including body read) or fails.
 * @param httpStatus HTTP status code (e.g. 200, 404), or negative on transport/BREW error.
 * @param body       Pointer to response body bytes (may be nullptr if empty or error).
 * @param bodyBytes  Number of bytes in @p body.
 * @param userData   User-supplied context pointer.
 */
typedef void (*HttpResponseCallback)(int32_t httpStatus, const void* body, uint32_t bodyBytes, void* userData);

/**
 * @brief Lightweight HTTP/HTTPS request client wrapping BREW IWeb* and ISource*.
 */
class HttpClient : public NonCopyable {
public:
    HttpClient();
    ~HttpClient();

    /**
     * @brief Initializes the HTTP client with the platform context (IShell* on Zeebo).
     */
    Result Initialize(void* platformContext);

    /**
     * @brief Cancels any in-flight request and releases underlying BREW resources.
     */
    void   Shutdown();

    /**
     * @brief Sets the maximum response body size in bytes (default 256 KiB).
     */
    void   SetMaxResponseBytes(uint32_t maxBytes);

    /**
     * @brief Performs an asynchronous HTTP GET request and reads the response body.
     * @param url      Null-terminated URL string.
     * @param callback Completion callback receiving status and full response body.
     * @param userData User-supplied context pointer passed to @p callback.
     */
    Result Get(const char* url, HttpResponseCallback callback, void* userData);

    /**
     * @brief Performs an asynchronous HTTP POST request with an optional request body.
     * @param url         Null-terminated URL string.
     * @param contentType Optional MIME type (e.g. "application/json"); may be nullptr.
     * @param body        Pointer to request payload bytes (copied internally); may be nullptr.
     * @param bodyBytes   Size of @p body in bytes.
     * @param callback    Completion callback receiving status and full response body.
     * @param userData    User-supplied context pointer passed to @p callback.
     */
    Result Post(const char* url,
                const char* contentType,
                const void* body,
                uint32_t bodyBytes,
                HttpResponseCallback callback,
                void* userData);

    /**
     * @brief Cancels any pending HTTP request or response body read.
     */
    void   CancelPending();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace net
} // namespace zsdk

#endif /* ZSDK_NET_HTTP_CLIENT_HPP */
