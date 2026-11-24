/**
 * @file audio_engine.hpp
 * @brief Multi-channel Audio Engine (BGM + pooled SFX channels) built on ZSDK Audio.
 */

#ifndef ZSDK_AUDIO_AUDIO_ENGINE_HPP
#define ZSDK_AUDIO_AUDIO_ENGINE_HPP

#include "zsdk/audio/audio.hpp"

namespace zsdk {
namespace audio {

/* Zeebo Developer Guide 9.2: max 4 simultaneous ADPCM/PCM/QCP decoders + 1 MP3 decoder (#1). */
static const uint8_t kMaxSfxChannels = 4u;

/**
 * @brief Central audio mixer owning 1 BGM MediaStream + up to 8 SoundEffect channels.
 */
class AudioEngine : public NonCopyable {
public:
    AudioEngine();
    ~AudioEngine();

    Result Initialize(void* platformContext);
    void   Shutdown();

    Result PlayMusic(const char* filePath, bool loop = true);
    void   StopMusic();
    void   SetMusicVolume(uint8_t volumePercent);

    Result PlaySfx(const char* filePath);
    void   SetSfxVolume(uint8_t volumePercent);
    void   StopAllSfx();

    void   SetMasterVolume(uint8_t volumePercent);

private:
    void*       m_platformContext;
    MediaStream m_bgm;
    SoundEffect m_sfxPool[kMaxSfxChannels];
    uint8_t     m_nextChannel;
    uint8_t     m_masterVolume;
    uint8_t     m_bgmVolume;
    uint8_t     m_sfxVolume;
    bool        m_initialized;
    bool        m_bgmIsMp3;
};

} // namespace audio
} // namespace zsdk

#endif /* ZSDK_AUDIO_AUDIO_ENGINE_HPP */
