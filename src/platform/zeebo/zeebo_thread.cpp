/**
 * @file zeebo_thread.cpp
 * @brief Qualcomm BREW / Zeebo cooperative IThread bridge (AEEThread.h).
 */

#include "zsdk/core/thread.hpp"
#include "zsdk/debug/memory_tracker.hpp"

#if ZSDK_PLATFORM_ZEEBO

#include "AEEShell.h"
#include "AEEStdLib.h"
#include "AEEThread.h"

namespace zsdk {

struct ThreadControlBlock {
    IThread*      thread;
    ThreadEntryFn entry;
    void*         userData;
    bool          running;
};

struct Thread::Impl {
    ThreadControlBlock cb;
};

static int BrewThreadTrampoline(IThread* piThread, void* pData) {
    ZSDK_UNUSED(piThread);
    ThreadControlBlock* cb = static_cast<ThreadControlBlock*>(pData);
    if (cb && cb->entry) {
        cb->entry(cb->userData);
    }
    if (cb) {
        cb->running = false;
    }
    return 0;
}

Thread::Thread()
    : m_impl(nullptr) {
}

Thread::~Thread() {
    Join();
}

Result Thread::Start(void* platformContext, ThreadEntryFn entry, void* userData, uint32_t stackSizeBytes) {
    if (m_impl) {
        return Result::AlreadyInitialized;
    }
    IShell* shell = static_cast<IShell*>(platformContext);
    if (!shell || !entry) {
        return Result::InvalidArg;
    }

    m_impl = static_cast<Impl*>(debug::MemoryTracker::Alloc(
        debug::MemorySubsystem::Core, sizeof(Impl)));
    if (!m_impl) {
        return Result::OutOfMemory;
    }
    m_impl->cb.thread   = nullptr;
    m_impl->cb.entry    = entry;
    m_impl->cb.userData = userData;
    m_impl->cb.running  = true;

    if (ISHELL_CreateInstance(shell, AEECLSID_THREAD,
                              reinterpret_cast<void**>(&m_impl->cb.thread)) != AEE_SUCCESS || !m_impl->cb.thread) {
        debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
        m_impl = nullptr;
        return Result::NotSupported;
    }

    ITHREAD_Start(m_impl->cb.thread, static_cast<int>(stackSizeBytes),
                  BrewThreadTrampoline, &m_impl->cb);
    return Result::Ok;
}

void Thread::Join() {
    if (!m_impl) {
        return;
    }
    if (m_impl->cb.thread) {
        int ret = 0;
        ITHREAD_Join(m_impl->cb.thread, nullptr, &ret);
        ITHREAD_Release(m_impl->cb.thread);
        m_impl->cb.thread = nullptr;
    }
    m_impl->cb.running = false;
    debug::MemoryTracker::Free(debug::MemorySubsystem::Core, m_impl);
    m_impl = nullptr;
}

void Thread::SleepMs(uint32_t ms) {
    ZSDK_UNUSED(ms);
    if (m_impl && m_impl->cb.thread) {
        ITHREAD_Suspend(m_impl->cb.thread);
    }
}

bool Thread::IsRunning() const {
    return m_impl && m_impl->cb.running;
}

} // namespace zsdk

#endif /* ZSDK_PLATFORM_ZEEBO */
