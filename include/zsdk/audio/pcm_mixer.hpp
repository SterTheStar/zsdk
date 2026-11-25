/**
 * @file pcm_mixer.hpp
 * @brief Fixed-point software PCM mixer with live per-voice sample-rate and pitch resampling.
 *
 * Allows real-time pitch and sample-rate modulation per voice on every Mix() call,
 * overcoming BREW IMedia/ISound's lack of live sample-rate control.
 */

#ifndef ZSDK_AUDIO_PCM_MIXER_HPP
#define ZSDK_AUDIO_PCM_MIXER_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace audio {

static const uint8_t kMaxPcmVoices = 16u;

/**
 * @brief Software 16-bit PCM mixer/resampler producing interleaved stereo output.
 *
 * Uses 16.16 fixed-point phase accumulation with linear interpolation so
 * SetVoiceSampleRate() and SetVoicePitch() take effect immediately on the next Mix().
 */
class PcmMixer : public NonCopyable {
public:
    explicit PcmMixer(uint32_t outputSampleRateHz = 22050u);

    void     SetOutputSampleRate(uint32_t outputSampleRateHz);
    uint32_t GetOutputSampleRate() const { return m_outputSampleRateHz; }

    void    SetMasterVolume(uint8_t volumePercent);
    uint8_t GetMasterVolume() const { return m_masterVolume; }

    /**
     * @brief Starts a 16-bit signed PCM voice.
     * @param samples Pointer to mono or interleaved stereo int16_t PCM data (must remain valid while playing).
     * @param frameCount Number of frames in @p samples.
     * @param sampleRateHz Native sample rate of @p samples in Hz.
     * @param channels 1 (mono) or 2 (stereo interleaved).
     * @param loop Whether to loop seamlessly at @p frameCount.
     * @param volumePercent Initial volume [0..100].
     * @param pitchQ16 Initial pitch multiplier in 16.16 fixed point (0x10000 = 1.0x).
     * @return Voice handle [0..kMaxPcmVoices-1], or -1 if no voice slot is free or args are invalid.
     */
    int8_t Play(const int16_t* samples,
                uint32_t frameCount,
                uint32_t sampleRateHz,
                uint8_t channels = 1u,
                bool loop = false,
                uint8_t volumePercent = 100u,
                uint32_t pitchQ16 = 0x10000u);

    /** @brief Updates a playing voice's base sample rate immediately (takes effect on next Mix). */
    Result SetVoiceSampleRate(int8_t voice, uint32_t sampleRateHz);

    /** @brief Updates a playing voice's 16.16 fixed-point pitch multiplier immediately (0x10000 = 1.0x). */
    Result SetVoicePitch(int8_t voice, uint32_t pitchQ16);

    /** @brief Updates a voice's stereo volume [0..100] per channel. */
    Result SetVoiceVolume(int8_t voice, uint8_t leftPercent, uint8_t rightPercent);

    /** @brief Updates a voice's mono/balanced volume [0..100]. */
    Result SetVoiceVolume(int8_t voice, uint8_t volumePercent);

    void StopVoice(int8_t voice);
    void StopAllVoices();
    bool IsVoicePlaying(int8_t voice) const;

    /**
     * @brief Mixes all active voices into @p outStereoInterleaved (2 * @p outFrames int16_t samples).
     */
    void Mix(int16_t* outStereoInterleaved, uint32_t outFrames);

private:
    struct Voice {
        const int16_t* samples;
        uint32_t       frameCount;
        uint32_t       baseSampleRateHz;
        uint32_t       pitchQ16;
        uint32_t       stepQ16;
        uint32_t       phaseFracQ16;
        uint32_t       phaseFrame;
        uint8_t        channels;
        uint8_t        volLeft;
        uint8_t        volRight;
        bool           loop;
        bool           active;
    };

    void RecomputeStep(Voice& v) const;

    Voice    m_voices[kMaxPcmVoices];
    uint32_t m_outputSampleRateHz;
    uint8_t  m_masterVolume;
};

} // namespace audio
} // namespace zsdk

#endif /* ZSDK_AUDIO_PCM_MIXER_HPP */
