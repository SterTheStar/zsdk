/**
 * @file zeebo_signal.cpp
 * @brief Qualcomm BREW / Zeebo AEECallback, ISignal, and ISignalBus implementation.
 */

#include "zsdk/core/signal.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEECallback.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"
#include "AEEISignalCBFactory.h"
#include "AEEISignalBus.h"
#include "AEESignalCBFactory.bid"
#include "AEESignalBus.bid"

namespace zsdk {

// -----------------------------------------------------------------------------
// AsyncCallback
// -----------------------------------------------------------------------------

struct AsyncCallback::Impl {
    IShell*          shell;
    AEECallback      cb;
    SignalCallbackFn callback;
    void*            userData;

    static void Trampoline(void* pUser) {
        Impl* impl = static_cast<Impl*>(pUser);
        if (impl && impl->callback) {
            impl->callback(impl->userData);
        }
    }
};

AsyncCallback::AsyncCallback()
    : m_impl(nullptr) {
}

AsyncCallback::~AsyncCallback() {
    Shutdown();
}

Result AsyncCallback::Initialize(void* platformContext, SignalCallbackFn callback, void* userData) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !callback) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Core, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }

    MEMSET(m_impl, 0, sizeof(Impl));
    m_impl->shell    = shell;
    m_impl->callback = callback;
    m_impl->userData = userData;
    CALLBACK_Init(&m_impl->cb, AsyncCallback::Impl::Trampoline, m_impl);
    return Result::Ok;
}

void AsyncCallback::Shutdown() {
    if (!m_impl) {
        return;
    }
    CALLBACK_Cancel(&m_impl->cb);
    debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
    m_impl = nullptr;
}

Result AsyncCallback::Schedule() {
    if (!m_impl || !m_impl->shell) {
        return Result::NotInitialized;
    }
    CALLBACK_Cancel(&m_impl->cb);
    CALLBACK_Init(&m_impl->cb, AsyncCallback::Impl::Trampoline, m_impl);
    ISHELL_Resume(m_impl->shell, &m_impl->cb);
    return Result::Ok;
}

void AsyncCallback::Cancel() {
    if (m_impl) {
        CALLBACK_Cancel(&m_impl->cb);
    }
}

bool AsyncCallback::IsPending() const {
    if (!m_impl) {
        return false;
    }
    return CALLBACK_IsQueued(&m_impl->cb) != 0;
}

void* AsyncCallback::GetNativeCallback() const {
    return m_impl ? static_cast<void*>(&m_impl->cb) : nullptr;
}

// -----------------------------------------------------------------------------
// Signal
// -----------------------------------------------------------------------------

struct Signal::Impl {
    ISignalCBFactory* factory;
    ISignal*          signal;
    ISignalCtl*       signalCtl;
    SignalCallbackFn  callback;
    void*             userData;

    static void Trampoline(void* pCx) {
        Impl* impl = static_cast<Impl*>(pCx);
        if (impl && impl->callback) {
            impl->callback(impl->userData);
        }
    }
};

Signal::Signal()
    : m_impl(nullptr) {
}

Signal::~Signal() {
    Shutdown();
}

Result Signal::Initialize(void* platformContext, SignalCallbackFn callback, void* userData) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !callback) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Core, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    MEMSET(m_impl, 0, sizeof(Impl));
    m_impl->callback = callback;
    m_impl->userData = userData;

    if (ISHELL_CreateInstance(shell, AEECLSID_SignalCBFactory,
                              reinterpret_cast<void**>(&m_impl->factory)) != AEE_SUCCESS || !m_impl->factory) {
        Shutdown();
        return Result::NotSupported;
    }

    if (ISignalCBFactory_CreateSignal(m_impl->factory,
                                      Signal::Impl::Trampoline, m_impl,
                                      &m_impl->signal, &m_impl->signalCtl) != AEE_SUCCESS ||
        !m_impl->signal || !m_impl->signalCtl) {
        Shutdown();
        return Result::Error;
    }

    return Result::Ok;
}

void Signal::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->signalCtl) {
        ISignalCtl_Detach(m_impl->signalCtl);
        ISignalCtl_Release(m_impl->signalCtl);
        m_impl->signalCtl = nullptr;
    }
    if (m_impl->signal) {
        ISignal_Release(m_impl->signal);
        m_impl->signal = nullptr;
    }
    if (m_impl->factory) {
        ISignalCBFactory_Release(m_impl->factory);
        m_impl->factory = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
    m_impl = nullptr;
}

Result Signal::Set() {
    if (!m_impl || !m_impl->signal) {
        return Result::NotInitialized;
    }
    return (ISignal_Set(m_impl->signal) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result Signal::Enable() {
    if (!m_impl || !m_impl->signalCtl) {
        return Result::NotInitialized;
    }
    return (ISignalCtl_Enable(m_impl->signalCtl) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result Signal::Detach() {
    if (!m_impl || !m_impl->signalCtl) {
        return Result::NotInitialized;
    }
    return (ISignalCtl_Detach(m_impl->signalCtl) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

bool Signal::IsValid() const {
    return m_impl != nullptr && m_impl->signal != nullptr && m_impl->signalCtl != nullptr;
}

void* Signal::GetNativeSignal() const {
    return m_impl ? static_cast<void*>(m_impl->signal) : nullptr;
}

// -----------------------------------------------------------------------------
// SignalBus
// -----------------------------------------------------------------------------

struct SignalBus::Impl {
    ISignalBus* bus;
};

SignalBus::SignalBus()
    : m_impl(nullptr) {
}

SignalBus::~SignalBus() {
    Shutdown();
}

Result SignalBus::Initialize(void* platformContext) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Core, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->bus = nullptr;

    if (ISHELL_CreateInstance(shell, AEECLSID_SignalBus,
                              reinterpret_cast<void**>(&m_impl->bus)) != AEE_SUCCESS || !m_impl->bus) {
        Shutdown();
        return Result::NotSupported;
    }

    return Result::Ok;
}

void SignalBus::Shutdown() {
    if (!m_impl) {
        return;
    }
    if (m_impl->bus) {
        ISignalBus_Release(m_impl->bus);
        m_impl->bus = nullptr;
    }
    debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
    m_impl = nullptr;
}

Result SignalBus::Add(Signal& signal) {
    if (!m_impl || !m_impl->bus) {
        return Result::NotInitialized;
    }
    ISignal* native = static_cast<ISignal*>(signal.GetNativeSignal());
    if (!native) {
        return Result::InvalidArg;
    }
    return (ISignalBus_Add(m_impl->bus, native) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result SignalBus::Remove(Signal& signal) {
    if (!m_impl || !m_impl->bus) {
        return Result::NotInitialized;
    }
    ISignal* native = static_cast<ISignal*>(signal.GetNativeSignal());
    if (!native) {
        return Result::InvalidArg;
    }
    return (ISignalBus_Remove(m_impl->bus, native) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result SignalBus::Set() {
    if (!m_impl || !m_impl->bus) {
        return Result::NotInitialized;
    }
    return (ISignalBus_Set(m_impl->bus) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result SignalBus::Clear() {
    if (!m_impl || !m_impl->bus) {
        return Result::NotInitialized;
    }
    return (ISignalBus_Clear(m_impl->bus) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

Result SignalBus::Strobe() {
    if (!m_impl || !m_impl->bus) {
        return Result::NotInitialized;
    }
    return (ISignalBus_Strobe(m_impl->bus) == AEE_SUCCESS) ? Result::Ok : Result::Error;
}

} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
