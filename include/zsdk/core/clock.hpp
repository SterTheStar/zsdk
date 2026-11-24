/**
 * @file clock.hpp
 * @brief High-precision monotonic clock and frame delta timer.
 *
 * Wraps BREW's GETUPTIMEMS() / GETTIMEMS() (AEEStdLib.h) with zero overhead.
 */

#ifndef ZSDK_CORE_CLOCK_HPP
#define ZSDK_CORE_CLOCK_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {

/**
 * @brief Platform monotonic & wall-clock time queries.
 */
class Clock {
public:
    /** @brief Monotonic milliseconds since device/process boot (GETUPTIMEMS). */
    static uint32_t GetUptimeMs();

    /** @brief Wall-clock milliseconds since BREW epoch (GETTIMEMS). */
    static uint32_t GetWallTimeMs();
};

/**
 * @brief Lightweight frame delta & FPS calculator with zero heap allocations.
 */
class FrameTimer {
public:
    FrameTimer()
        : m_lastMs(0)
        , m_deltaMs(16)
        , m_fpsAccumMs(0)
        , m_fpsFrames(0)
        , m_currentFps(60.0f) {}

    /** @brief Resets timer baseline to current uptime. */
    void Reset() {
        m_lastMs = Clock::GetUptimeMs();
        m_deltaMs = 16;
        m_fpsAccumMs = 0;
        m_fpsFrames = 0;
        m_currentFps = 60.0f;
    }

    /** @brief Advances one frame and returns elapsed milliseconds (clamped to [1..250]). */
    uint32_t Tick() {
        const uint32_t now = Clock::GetUptimeMs();
        const uint32_t rawDt = (now >= m_lastMs) ? (now - m_lastMs) : 0u;
        m_lastMs = now;

        m_fpsAccumMs += rawDt;
        ++m_fpsFrames;
        if (m_fpsAccumMs >= 500u) {
            m_currentFps = (static_cast<float>(m_fpsFrames) * 1000.0f) /
                           static_cast<float>(m_fpsAccumMs);
            m_fpsAccumMs = 0u;
            m_fpsFrames = 0u;
        }

        uint32_t dt = rawDt;
        if (dt == 0u) {
            dt = 1u;
        } else if (dt > 250u) {
            dt = 250u;
        }
        m_deltaMs = dt;
        return m_deltaMs;
    }

    ZSDK_FORCE_INLINE uint32_t GetDeltaMs() const { return m_deltaMs; }
    ZSDK_FORCE_INLINE float GetDeltaSeconds() const { return static_cast<float>(m_deltaMs) * 0.001f; }
    ZSDK_FORCE_INLINE float GetFps() const { return m_currentFps; }

private:
    uint32_t m_lastMs;
    uint32_t m_deltaMs;
    uint32_t m_fpsAccumMs;
    uint32_t m_fpsFrames;
    float    m_currentFps;
};

} // namespace zsdk

#endif /* ZSDK_CORE_CLOCK_HPP */
