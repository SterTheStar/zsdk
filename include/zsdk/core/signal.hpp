/**
 * @file signal.hpp
 * @brief Asynchronous completion callbacks and 1:1 / fan-out signals over BREW AEECallback & ISignal.
 *
 * Isolates AEECallback.h, AEEISignal.h, AEEISignalCtl.h, AEEISignalCBFactory.h,
 * and AEEISignalBus.h from public user headers.
 */

#ifndef ZSDK_CORE_SIGNAL_HPP
#define ZSDK_CORE_SIGNAL_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {

/**
 * @brief Signature for asynchronous signal and callback notifications.
 * @param userData Opaque user pointer registered with the callback or signal.
 */
typedef void (*SignalCallbackFn)(void* userData);

/**
 * @brief RAII wrapper around BREW AEECallback for deferred main-loop execution and cancellation.
 *
 * Automatically cancels any pending callback on destruction to prevent dangling-pointer
 * invocations after the owning object is destroyed.
 */
class AsyncCallback : public NonCopyable {
public:
    AsyncCallback();
    ~AsyncCallback();

    /**
     * @brief Initializes the callback with a BREW IShell* context and target function.
     * @param platformContext Pointer to BREW IShell*.
     * @param callback        Function invoked when the callback fires.
     * @param userData        User pointer passed to @p callback.
     * @return Result::Ok on success, or an error code.
     */
    Result Initialize(void* platformContext, SignalCallbackFn callback, void* userData);

    /** @brief Cancels any queued callback and releases internal state. */
    void Shutdown();

    /**
     * @brief Schedules the callback for asynchronous execution via ISHELL_Resume().
     * @return Result::Ok if scheduled, Result::NotInitialized if not initialized.
     */
    Result Schedule();

    /** @brief Cancels the callback if currently queued. Safe to call when not pending. */
    void Cancel();

    /** @brief Returns true if the callback is currently queued for execution. */
    bool IsPending() const;

    /**
     * @brief Returns the underlying native AEECallback* for passing to BREW async APIs.
     * @return Opaque AEECallback* or nullptr if uninitialized.
     */
    void* GetNativeCallback() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief 1:1 asynchronous notification signal wrapping BREW ISignal / ISignalCtl.
 *
 * Created via ISignalCBFactory so that calling Set() dispatches the registered
 * callback asynchronously on the owning apartment.
 */
class Signal : public NonCopyable {
public:
    Signal();
    ~Signal();

    /**
     * @brief Creates a callback-backed signal via AEECLSID_SignalCBFactory.
     * @param platformContext Pointer to BREW IShell*.
     * @param callback        Function invoked when the signal is dispatched.
     * @param userData        User pointer passed to @p callback.
     * @return Result::Ok on success, Result::NotSupported if SignalCBFactory is unavailable.
     */
    Result Initialize(void* platformContext, SignalCallbackFn callback, void* userData);

    /** @brief Detaches and releases the underlying ISignal and ISignalCtl interfaces. */
    void Shutdown();

    /** @brief Marks the signal as ready, queueing dispatch if enabled. */
    Result Set();

    /** @brief Re-enables the signal after it has been auto-disabled on dispatch. */
    Result Enable();

    /** @brief Detaches the signal from its callback handler so future Set() calls are no-ops. */
    Result Detach();

    /** @brief Returns true if the signal was successfully initialized. */
    bool IsValid() const;

    /**
     * @brief Returns the underlying native ISignal* pointer.
     * @return Opaque ISignal* or nullptr if uninitialized.
     */
    void* GetNativeSignal() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Multi-listener signal bus wrapping BREW ISignalBus.
 */
class SignalBus : public NonCopyable {
public:
    SignalBus();
    ~SignalBus();

    /**
     * @brief Creates the underlying ISignalBus instance.
     * @param platformContext Pointer to BREW IShell*.
     * @return Result::Ok on success, Result::NotSupported if SignalBus is unavailable.
     */
    Result Initialize(void* platformContext);

    /** @brief Releases the underlying ISignalBus instance. */
    void Shutdown();

    /** @brief Adds a Signal to the bus. */
    Result Add(Signal& signal);

    /** @brief Removes a Signal from the bus. */
    Result Remove(Signal& signal);

    /** @brief Sets the bus state, signalling all enabled signals on the bus. */
    Result Set();

    /** @brief Clears the bus state. */
    Result Clear();

    /** @brief Atomically sets and clears the bus, signalling all currently enabled signals. */
    Result Strobe();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace zsdk

#endif /* ZSDK_CORE_SIGNAL_HPP */
