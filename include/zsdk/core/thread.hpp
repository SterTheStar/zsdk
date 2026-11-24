/**
 * @file thread.hpp
 * @brief Cooperative/OS Threading & Signal abstraction over BREW IThread (AEEThread.h).
 */

#ifndef ZSDK_CORE_THREAD_HPP
#define ZSDK_CORE_THREAD_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {

typedef void (*ThreadEntryFn)(void* userData);

/**
 * @brief Lightweight thread wrapper over BREW IThread*.
 */
class Thread : public NonCopyable {
public:
    Thread();
    ~Thread();

    Result Start(void* platformContext, ThreadEntryFn entry, void* userData, uint32_t stackSizeBytes = 16384u);
    void   Join();
    void   SleepMs(uint32_t ms);
    bool   IsRunning() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace zsdk

#endif /* ZSDK_CORE_THREAD_HPP */
