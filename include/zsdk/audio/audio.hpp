/**
 * @file audio.hpp
 * @brief Audio, SFX & Media Streaming abstraction over BREW IMedia / ISoundPlayer.
 */

#ifndef ZSDK_AUDIO_AUDIO_HPP
#define ZSDK_AUDIO_AUDIO_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace audio {

enum class MediaFormat : uint8_t {
    Auto = 0,
    Wave,
    Mp3,
    Midi,
    Qcp,
    Aac
};

/**
 * @brief High-level media/music/SFX stream player wrapping BREW IMedia*.
 */
class MediaStream : public NonCopyable {
public:
    MediaStream();
    ~MediaStream();

    Result LoadFromFile(void* platformContext, const char* filePath);
    Result LoadFromMemory(void* platformContext, const void* data, uint32_t sizeBytes, MediaFormat fmt);
    void   Unload();

    Result Play();
    Result Stop();
    Result Pause();
    Result Resume();

    /** @brief Sets volume [0..100] (mapped to BREW 0..AEE_MAX_VOLUME). */
    Result SetVolume(uint8_t volumePercent);
    void   SetLooping(bool loop);

    bool IsPlaying() const;
    bool IsLoaded() const;

private:
    struct Impl;
    Impl* m_impl;
};

/**
 * @brief Low-latency sound effect player wrapping BREW ISoundPlayer*.
 */
class SoundEffect : public NonCopyable {
public:
    SoundEffect();
    ~SoundEffect();

    Result Initialize(void* platformContext);
    void   Shutdown();

    Result PlayFile(const char* filePath);
    Result PlayMemory(const void* data, uint32_t sizeBytes);
    Result Stop();
    Result SetVolume(uint8_t volumePercent);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace audio
} // namespace zsdk

#endif /* ZSDK_AUDIO_AUDIO_HPP */
