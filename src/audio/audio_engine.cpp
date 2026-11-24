/**
 * @file audio_engine.cpp
 * @brief Multi-channel AudioEngine built on ZSDK MediaStream & SoundEffect pool.
 */

#include "zsdk/audio/audio_engine.hpp"

namespace zsdk {
namespace audio {

AudioEngine::AudioEngine()
    : m_platformContext(nullptr)
    , m_nextChannel(0u)
    , m_masterVolume(100u)
    , m_bgmVolume(100u)
    , m_sfxVolume(100u)
    , m_initialized(false) {
}

AudioEngine::~AudioEngine() {
    Shutdown();
}

Result AudioEngine::Initialize(void* platformContext) {
    if (m_initialized) {
        return Result::AlreadyInitialized;
    }
    m_platformContext = platformContext;
    for (uint8_t i = 0u; i < kMaxSfxChannels; ++i) {
        m_sfxPool[i].Initialize(platformContext);
    }
    m_initialized = true;
    return Result::Ok;
}

void AudioEngine::Shutdown() {
    if (!m_initialized) {
        return;
    }
    m_bgm.Unload();
    for (uint8_t i = 0u; i < kMaxSfxChannels; ++i) {
        m_sfxPool[i].Shutdown();
    }
    m_platformContext = nullptr;
    m_initialized = false;
}

Result AudioEngine::PlayMusic(const char* filePath, bool loop) {
    if (!m_initialized) {
        return Result::NotInitialized;
    }
    Result r = m_bgm.LoadFromFile(m_platformContext, filePath);
    if (Failed(r)) {
        return r;
    }
    m_bgm.SetLooping(loop);
    m_bgm.SetVolume(static_cast<uint8_t>((static_cast<uint16_t>(m_bgmVolume) * m_masterVolume) / 100u));
    return m_bgm.Play();
}

void AudioEngine::StopMusic() {
    m_bgm.Stop();
}

void AudioEngine::SetMusicVolume(uint8_t volumePercent) {
    m_bgmVolume = (volumePercent > 100u) ? 100u : volumePercent;
    m_bgm.SetVolume(static_cast<uint8_t>((static_cast<uint16_t>(m_bgmVolume) * m_masterVolume) / 100u));
}

Result AudioEngine::PlaySfx(const char* filePath) {
    if (!m_initialized) {
        return Result::NotInitialized;
    }
    SoundEffect& ch = m_sfxPool[m_nextChannel];
    m_nextChannel = static_cast<uint8_t>((m_nextChannel + 1u) % kMaxSfxChannels);
    ch.SetVolume(static_cast<uint8_t>((static_cast<uint16_t>(m_sfxVolume) * m_masterVolume) / 100u));
    return ch.PlayFile(filePath);
}

void AudioEngine::SetSfxVolume(uint8_t volumePercent) {
    m_sfxVolume = (volumePercent > 100u) ? 100u : volumePercent;
    const uint8_t eff = static_cast<uint8_t>((static_cast<uint16_t>(m_sfxVolume) * m_masterVolume) / 100u);
    for (uint8_t i = 0u; i < kMaxSfxChannels; ++i) {
        m_sfxPool[i].SetVolume(eff);
    }
}

void AudioEngine::StopAllSfx() {
    for (uint8_t i = 0u; i < kMaxSfxChannels; ++i) {
        m_sfxPool[i].Stop();
    }
}

void AudioEngine::SetMasterVolume(uint8_t volumePercent) {
    m_masterVolume = (volumePercent > 100u) ? 100u : volumePercent;
    SetMusicVolume(m_bgmVolume);
    SetSfxVolume(m_sfxVolume);
}

} // namespace audio
} // namespace zsdk
