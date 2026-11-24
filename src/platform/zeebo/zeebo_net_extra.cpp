/**
 * @file zeebo_net_extra.cpp
 * @brief Qualcomm BREW / Zeebo HttpClient (IWeb/ISource), NetworkManager (INetMgr/IWIFI), BluetoothManager (IBT/IBTSPPClient).
 */

#include "zsdk/net/http_client.hpp"
#include "zsdk/net/network_mgr.hpp"
#include "zsdk/net/bluetooth.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEWeb.h"
#include "AEESource.h"
#include "AEENet.h"
#include "AEEWIFI.h"
#include "AEEBT.h"
#include "AEEBTSPP.h"
#include "AEEBTSPPClient.h"
#include "AEEBTSPPClientGetObject.h"

namespace zsdk {
namespace net {

namespace {
static constexpr uint32_t kDefaultMaxResponseBytes = 256u * 1024u;
static constexpr uint32_t kInitialBodyCapacity     = 4096u;
static constexpr uint32_t kReadChunkBytes          = 2048u;
} // namespace

/* --------------------------------------------------------------------------
 * HttpClient (IWeb + ISource)
 * -------------------------------------------------------------------------- */
struct HttpClient::Impl {
    IShell*              shell;
    IWeb*                web;
    IWebResp*            resp;
    ISource*             bodySource;
    ISource*             postSource;
    uint8_t*             postBuf;
    char                 postHeaders[192];
    uint8_t*             bodyBuf;
    uint32_t             bodyLen;
    uint32_t             bodyCap;
    uint32_t             maxResponseBytes;
    int32_t              httpStatus;
    AEECallback          cb;
    AEECallback          readCb;
    HttpResponseCallback userCb;
    void*                userData;

    void ReleaseRequestBuffers() {
        if (bodySource) {
            ISOURCE_Release(bodySource);
            bodySource = nullptr;
        }
        if (resp) {
            IWEBRESP_Release(resp);
            resp = nullptr;
        }
        if (postSource) {
            ISOURCE_Release(postSource);
            postSource = nullptr;
        }
        if (postBuf) {
            debug::MemoryTracker::Free(debug::MemorySubsystem::Network, postBuf);
            postBuf = nullptr;
        }
        if (bodyBuf) {
            debug::MemoryTracker::Free(debug::MemorySubsystem::Network, bodyBuf);
            bodyBuf = nullptr;
        }
        bodyLen = 0u;
        bodyCap = 0u;
    }

    bool AppendBodyBytes(const uint8_t* data, uint32_t len) {
        if (len == 0u) {
            return true;
        }
        const uint32_t maxCap = (maxResponseBytes > 0u) ? maxResponseBytes : kDefaultMaxResponseBytes;
        if (bodyLen >= maxCap) {
            return false;
        }
        uint32_t copyLen = len;
        if (bodyLen + copyLen > maxCap) {
            copyLen = maxCap - bodyLen;
        }
        const uint32_t needed = bodyLen + copyLen;
        if (needed > bodyCap) {
            uint32_t newCap = (bodyCap == 0u) ? kInitialBodyCapacity : bodyCap;
            while (newCap < needed) {
                if (newCap >= maxCap) {
                    newCap = maxCap;
                    break;
                }
                newCap *= 2u;
                if (newCap > maxCap) {
                    newCap = maxCap;
                }
            }
            uint8_t* newBuf = static_cast<uint8_t*>(
                debug::MemoryTracker::Alloc(debug::MemorySubsystem::Network, newCap));
            if (!newBuf) {
                return false;
            }
            if (bodyBuf && bodyLen > 0u) {
                MEMCPY(newBuf, bodyBuf, bodyLen);
            }
            if (bodyBuf) {
                debug::MemoryTracker::Free(debug::MemorySubsystem::Network, bodyBuf);
            }
            bodyBuf = newBuf;
            bodyCap = newCap;
        }
        MEMCPY(bodyBuf + bodyLen, data, copyLen);
        bodyLen += copyLen;
        return true;
    }

    void FinishAndDeliver(int32_t status) {
        HttpResponseCallback cbFn = userCb;
        void* cbData              = userData;
        uint8_t* outBody          = bodyBuf;
        uint32_t outLen           = bodyLen;

        /* Detach user callback before invoking in case callback re-enters HttpClient */
        userCb   = nullptr;
        userData = nullptr;

        if (bodySource) {
            ISOURCE_Release(bodySource);
            bodySource = nullptr;
        }
        if (resp) {
            IWEBRESP_Release(resp);
            resp = nullptr;
        }
        if (postSource) {
            ISOURCE_Release(postSource);
            postSource = nullptr;
        }
        if (postBuf) {
            debug::MemoryTracker::Free(debug::MemorySubsystem::Network, postBuf);
            postBuf = nullptr;
        }

        if (cbFn) {
            cbFn(status, (outLen > 0u) ? outBody : nullptr, outLen, cbData);
        }

        if (outBody) {
            debug::MemoryTracker::Free(debug::MemorySubsystem::Network, outBody);
            /* Only clear if callback did not start a new request that allocated bodyBuf */
            if (bodyBuf == outBody) {
                bodyBuf = nullptr;
                bodyLen = 0u;
                bodyCap = 0u;
            }
        }
    }

    void PumpBodyRead() {
        if (!bodySource) {
            FinishAndDeliver(httpStatus);
            return;
        }
        char chunk[kReadChunkBytes];
        for (;;) {
            const int32 n = ISOURCE_Read(bodySource, chunk, static_cast<int32>(sizeof(chunk)));
            if (n > 0) {
                if (!AppendBodyBytes(reinterpret_cast<const uint8_t*>(chunk),
                                     static_cast<uint32_t>(n))) {
                    FinishAndDeliver(httpStatus);
                    return;
                }
                continue;
            }
            if (n == ISOURCE_WAIT) {
                CALLBACK_Init(&readCb, Impl::OnBodyReadable, this);
                ISOURCE_Readable(bodySource, &readCb);
                return;
            }
            /* ISOURCE_END (0) or ISOURCE_ERROR (-1) */
            FinishAndDeliver(httpStatus);
            return;
        }
    }

    static void OnBodyReadable(void* pNotifyData) {
        Impl* impl = static_cast<Impl*>(pNotifyData);
        if (impl) {
            impl->PumpBodyRead();
        }
    }

    static void OnWebResp(void* pNotifyData) {
        Impl* impl = static_cast<Impl*>(pNotifyData);
        if (!impl || !impl->userCb) {
            return;
        }
        impl->httpStatus = -1;
        if (impl->resp) {
            WebRespInfo* wri = IWEBRESP_GetInfo(impl->resp);
            if (wri) {
                impl->httpStatus = static_cast<int32_t>(wri->nCode);
                if (wri->pisMessage) {
                    impl->bodySource = wri->pisMessage;
                    ISOURCE_AddRef(impl->bodySource);
                    impl->PumpBodyRead();
                    return;
                }
            }
        }
        impl->FinishAndDeliver(impl->httpStatus);
    }
};

HttpClient::HttpClient()
    : m_impl(nullptr) {
}

HttpClient::~HttpClient() {
    Shutdown();
}

Result HttpClient::Initialize(void* platformContext) {
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
    MEMSET(m_impl, 0, sizeof(Impl));
    m_impl->shell            = shell;
    m_impl->maxResponseBytes = kDefaultMaxResponseBytes;

    if (ISHELL_CreateInstance(shell, AEECLSID_WEB,
                              reinterpret_cast<void**>(&m_impl->web)) != AEE_SUCCESS || !m_impl->web) {
        Shutdown();
        return Result::NetworkError;
    }
    return Result::Ok;
}

void HttpClient::Shutdown() {
    if (!m_impl) {
        return;
    }
    CancelPending();
    if (m_impl->web) {
        IWEB_Release(m_impl->web);
        m_impl->web = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Network, m_impl);
    m_impl = nullptr;
}

void HttpClient::SetMaxResponseBytes(uint32_t maxBytes) {
    if (m_impl && maxBytes > 0u) {
        m_impl->maxResponseBytes = maxBytes;
    }
}

Result HttpClient::Get(const char* url, HttpResponseCallback callback, void* userData) {
    if (!m_impl || !m_impl->web || !url) {
        return Result::InvalidArg;
    }
    CancelPending();
    m_impl->userCb   = callback;
    m_impl->userData = userData;
    CALLBACK_Init(&m_impl->cb, Impl::OnWebResp, m_impl);
    IWEB_GetResponse(m_impl->web, (m_impl->web, &m_impl->resp, &m_impl->cb, url, WEBOPT_END));
    return Result::Ok;
}

Result HttpClient::Post(const char* url,
                        const char* contentType,
                        const void* body,
                        uint32_t bodyBytes,
                        HttpResponseCallback callback,
                        void* userData) {
    if (!m_impl || !m_impl->web || !m_impl->shell || !url) {
        return Result::InvalidArg;
    }
    CancelPending();

    if (contentType && contentType[0] != '\0') {
        SNPRINTF(m_impl->postHeaders, sizeof(m_impl->postHeaders),
                 "Content-Type: %s\r\n", contentType);
        m_impl->postHeaders[sizeof(m_impl->postHeaders) - 1u] = '\0';
    } else {
        m_impl->postHeaders[0] = '\0';
    }

    if (body && bodyBytes > 0u) {
        m_impl->postBuf = static_cast<uint8_t*>(
            debug::MemoryTracker::Alloc(debug::MemorySubsystem::Network, bodyBytes));
        if (!m_impl->postBuf) {
            return Result::OutOfMemory;
        }
        MEMCPY(m_impl->postBuf, body, bodyBytes);

        ISourceUtil* srcUtil = nullptr;
        if (ISHELL_CreateInstance(m_impl->shell, AEECLSID_SOURCEUTIL,
                                  reinterpret_cast<void**>(&srcUtil)) != AEE_SUCCESS || !srcUtil) {
            m_impl->ReleaseRequestBuffers();
            return Result::NetworkError;
        }
        const int32 srcRes = ISOURCEUTIL_SourceFromMemory(
            srcUtil, m_impl->postBuf, static_cast<int32>(bodyBytes),
            nullptr, nullptr, &m_impl->postSource);
        ISOURCEUTIL_Release(srcUtil);
        if (srcRes != AEE_SUCCESS || !m_impl->postSource) {
            m_impl->ReleaseRequestBuffers();
            return Result::NetworkError;
        }
    }

    m_impl->userCb   = callback;
    m_impl->userData = userData;
    CALLBACK_Init(&m_impl->cb, Impl::OnWebResp, m_impl);

    if (m_impl->postSource) {
        if (m_impl->postHeaders[0] != '\0') {
            IWEB_GetResponse(m_impl->web,
                             (m_impl->web, &m_impl->resp, &m_impl->cb, url,
                              WEBOPT_METHOD, "POST",
                              WEBOPT_HEADER, m_impl->postHeaders,
                              WEBOPT_CONTENTLENGTH, static_cast<long>(bodyBytes),
                              WEBOPT_BODY, m_impl->postSource,
                              WEBOPT_END));
        } else {
            IWEB_GetResponse(m_impl->web,
                             (m_impl->web, &m_impl->resp, &m_impl->cb, url,
                              WEBOPT_METHOD, "POST",
                              WEBOPT_CONTENTLENGTH, static_cast<long>(bodyBytes),
                              WEBOPT_BODY, m_impl->postSource,
                              WEBOPT_END));
        }
    } else {
        IWEB_GetResponse(m_impl->web,
                         (m_impl->web, &m_impl->resp, &m_impl->cb, url,
                          WEBOPT_METHOD, "POST",
                          WEBOPT_CONTENTLENGTH, 0L,
                          WEBOPT_END));
    }
    return Result::Ok;
}

void HttpClient::CancelPending() {
    if (!m_impl) {
        return;
    }
    CALLBACK_Cancel(&m_impl->cb);
    CALLBACK_Cancel(&m_impl->readCb);
    m_impl->userCb   = nullptr;
    m_impl->userData = nullptr;
    m_impl->ReleaseRequestBuffers();
}

/* --------------------------------------------------------------------------
 * NetworkManager (INetMgr / IWIFI)
 * -------------------------------------------------------------------------- */
struct NetworkManager::Impl {
    INetMgr*           net;
    IWIFI*             wifi;
    AEEDNSResult       dnsResult;
    AEECallback        dnsCb;
    DnsResolveCallback dnsUserCb;
    void*              dnsUserData;
    bool               dnsPending;

    static void OnDnsComplete(void* pNotifyData) {
        Impl* impl = static_cast<Impl*>(pNotifyData);
        if (!impl) {
            return;
        }
        impl->dnsPending        = false;
        DnsResolveCallback cbFn = impl->dnsUserCb;
        void* cbData            = impl->dnsUserData;
        impl->dnsUserCb         = nullptr;
        impl->dnsUserData       = nullptr;

        if (!cbFn) {
            return;
        }

        if (impl->dnsResult.nResult > 0 && impl->dnsResult.nResult <= AEEDNSMAXADDRS) {
            uint32_t hostAddrs[AEEDNSMAXADDRS];
            const uint32_t count = static_cast<uint32_t>(impl->dnsResult.nResult);
            for (uint32_t i = 0u; i < count; ++i) {
                hostAddrs[i] = NTOHL(impl->dnsResult.addrs[i]);
            }
            cbFn(Result::Ok, hostAddrs, count, cbData);
        } else {
            cbFn(Result::NetworkError, nullptr, 0u, cbData);
        }
    }
};

NetworkManager::NetworkManager()
    : m_impl(nullptr) {
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

Result NetworkManager::Initialize(void* platformContext) {
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
    MEMSET(m_impl, 0, sizeof(Impl));

    ISHELL_CreateInstance(shell, AEECLSID_NET, reinterpret_cast<void**>(&m_impl->net));
    ISHELL_CreateInstance(shell, AEECLSID_WIFI, reinterpret_cast<void**>(&m_impl->wifi));
    return Result::Ok;
}

void NetworkManager::Shutdown() {
    if (!m_impl) {
        return;
    }
    CancelDns();
    if (m_impl->wifi) {
        IWIFI_Release(m_impl->wifi);
        m_impl->wifi = nullptr;
    }
    if (m_impl->net) {
        INETMGR_Release(m_impl->net);
        m_impl->net = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Network, m_impl);
    m_impl = nullptr;
}

NetworkState NetworkManager::GetState() const {
    if (!m_impl || !m_impl->net) {
        return NetworkState::Disconnected;
    }
    return NetworkState::Connected;
}

bool NetworkManager::IsWifiAvailable() const {
    return m_impl && (m_impl->wifi != nullptr);
}

uint32_t NetworkManager::GetLocalIpAddress() const {
    if (!m_impl || !m_impl->net) {
        return 0u;
    }
    return NTOHL(INETMGR_GetMyIPAddr(m_impl->net));
}

Result NetworkManager::ResolveHost(const char* hostname, DnsResolveCallback callback, void* userData) {
    if (!m_impl || !hostname || hostname[0] == '\0' || !callback) {
        return Result::InvalidArg;
    }
    if (!m_impl->net) {
        return Result::NotSupported;
    }
    CancelDns();

    MEMSET(&m_impl->dnsResult, 0, sizeof(m_impl->dnsResult));
    m_impl->dnsUserCb   = callback;
    m_impl->dnsUserData = userData;
    m_impl->dnsPending  = true;
    CALLBACK_Init(&m_impl->dnsCb, Impl::OnDnsComplete, m_impl);
    INETMGR_GetHostByName(m_impl->net, &m_impl->dnsResult, hostname, &m_impl->dnsCb);
    return Result::Ok;
}

void NetworkManager::CancelDns() {
    if (!m_impl) {
        return;
    }
    if (m_impl->dnsPending) {
        CALLBACK_Cancel(&m_impl->dnsCb);
        m_impl->dnsPending = false;
    }
    m_impl->dnsUserCb   = nullptr;
    m_impl->dnsUserData = nullptr;
}

/* --------------------------------------------------------------------------
 * BluetoothManager (IBT / IBTSPPClient)
 * -------------------------------------------------------------------------- */
struct BluetoothManager::Impl {
    IBT*          bt;
    IBTSPPClient* sppClient;
};

BluetoothManager::BluetoothManager()
    : m_impl(nullptr) {
}

BluetoothManager::~BluetoothManager() {
    Shutdown();
}

Result BluetoothManager::Initialize(void* platformContext) {
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
    MEMSET(m_impl, 0, sizeof(Impl));
#ifdef AEECLSID_BT
    ISHELL_CreateInstance(shell, AEECLSID_BT, reinterpret_cast<void**>(&m_impl->bt));
#endif
    return Result::Ok;
}

void BluetoothManager::Shutdown() {
    if (!m_impl) {
        return;
    }
    CloseSpp();
    if (m_impl->bt) {
        IBT_Release(m_impl->bt);
        m_impl->bt = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Network, m_impl);
    m_impl = nullptr;
}

bool BluetoothManager::IsSupported() const {
    return m_impl && (m_impl->bt != nullptr);
}

bool BluetoothManager::IsRadioEnabled() const {
    if (!m_impl || !m_impl->bt) {
        return false;
    }
#ifdef AEECLSID_BT
    BTRadioState state = AEEBT_RADIO_OFF;
    if (IBT_GetRadioState(m_impl->bt, &state) == SUCCESS) {
        return (state & AEEBT_RADIO_OFF) == 0u;
    }
#endif
    return false;
}

Result BluetoothManager::OpenSpp(const char* portName) {
    if (!m_impl) {
        return Result::NotInitialized;
    }
    if (!m_impl->bt) {
        return Result::NotSupported;
    }
#ifdef AEEIID_BT_SPP_CLIENT
    if (!m_impl->sppClient) {
        if (IBT_GetSPPClient(m_impl->bt, reinterpret_cast<void**>(&m_impl->sppClient)) != SUCCESS ||
            !m_impl->sppClient) {
            return Result::NotSupported;
        }
    }
    const int res = IBTSPPCLI_Open(m_impl->sppClient, portName);
    return (res == SUCCESS) ? Result::Ok : Result::IoError;
#else
    (void)portName;
    return Result::NotSupported;
#endif
}

void BluetoothManager::CloseSpp() {
    if (!m_impl || !m_impl->sppClient) {
        return;
    }
#ifdef AEEIID_BT_SPP_CLIENT
    IBTSPPCLI_Close(m_impl->sppClient);
    IBTSPPCLI_Release(m_impl->sppClient);
#endif
    m_impl->sppClient = nullptr;
}

SppState BluetoothManager::GetSppState() const {
    if (!m_impl || !m_impl->sppClient) {
        return SppState::Closed;
    }
#ifdef AEEIID_BT_SPP_CLIENT
    BTSPPStatus status;
    MEMSET(&status, 0, sizeof(status));
    if (IBTSPPCLI_GetStatus(m_impl->sppClient, &status) != SUCCESS) {
        return SppState::Closed;
    }
    switch (status.state) {
        case AEEBTSPP_ST_OPENING:       return SppState::Opening;
        case AEEBTSPP_ST_OPEN_ERROR:    return SppState::OpenError;
        case AEEBTSPP_ST_OPENED:        return SppState::Opened;
        case AEEBTSPP_ST_CONNECTED:     return SppState::Connected;
        case AEEBTSPP_ST_DISCONNECTING: return SppState::Disconnecting;
        case AEEBTSPP_ST_DISCONNECTED:  return SppState::Disconnected;
        case AEEBTSPP_ST_CLOSED:
        default:                        return SppState::Closed;
    }
#else
    return SppState::Closed;
#endif
}

int32_t BluetoothManager::ReadSpp(void* outBuffer, uint32_t maxBytes) {
    if (!m_impl || !m_impl->sppClient || !outBuffer || maxBytes == 0u) {
        return -1;
    }
#ifdef AEEIID_BT_SPP_CLIENT
    return static_cast<int32_t>(
        IBTSPPCLI_Read(m_impl->sppClient, static_cast<char*>(outBuffer), static_cast<int32>(maxBytes)));
#else
    return -1;
#endif
}

int32_t BluetoothManager::WriteSpp(const void* data, uint32_t sizeBytes) {
    if (!m_impl || !m_impl->sppClient || !data || sizeBytes == 0u) {
        return -1;
    }
#ifdef AEEIID_BT_SPP_CLIENT
    return static_cast<int32_t>(
        IBTSPPCLI_Write(m_impl->sppClient, static_cast<const char*>(data), static_cast<int32>(sizeBytes)));
#else
    return -1;
#endif
}

} // namespace net
} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
