/**
 * @file pcm_mixer.cpp
 * @brief Fixed-point software PCM mixer with live per-voice sample-rate and pitch resampling.
 */

#include "zsdk/audio/pcm_mixer.hpp"

namespace zsdk {
namespace audio {

namespace {

int16_t ClampSample(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

uint8_t ClampPercent(uint8_t p) {
    return (p > 100u) ? 100u : p;
}

} // namespace

PcmMixer::PcmMixer(uint32_t outputSampleRateHz)
    : m_outputSampleRateHz(outputSampleRateHz > 0u ? outputSampleRateHz : 22050u)
    , m_masterVolume(100u) {
    StopAllVoices();
}

void PcmMixer::SetOutputSampleRate(uint32_t outputSampleRateHz) {
    if (outputSampleRateHz == 0u) {
        return;
    }
    m_outputSampleRateHz = outputSampleRateHz;
    for (uint8_t i = 0u; i < kMaxPcmVoices; ++i) {
        if (m_voices[i].active) {
            RecomputeStep(m_voices[i]);
        }
    }
}

void PcmMixer::SetMasterVolume(uint8_t volumePercent) {
    m_masterVolume = ClampPercent(volumePercent);
}

void PcmMixer::RecomputeStep(Voice& v) const {
    const uint64_t effectiveRate =
        (static_cast<uint64_t>(v.baseSampleRateHz) * static_cast<uint64_t>(v.pitchQ16)) >> 16;
    const uint64_t step = (effectiveRate << 16) / static_cast<uint64_t>(m_outputSampleRateHz);
    v.stepQ16 = static_cast<uint32_t>(step);
}

int8_t PcmMixer::Play(const int16_t* samples,
                      uint32_t frameCount,
                      uint32_t sampleRateHz,
                      uint8_t channels,
                      bool loop,
                      uint8_t volumePercent,
                      uint32_t pitchQ16) {
    if (!samples || frameCount == 0u || sampleRateHz == 0u || (channels != 1u && channels != 2u)) {
        return -1;
    }

    for (uint8_t i = 0u; i < kMaxPcmVoices; ++i) {
        if (!m_voices[i].active) {
            Voice& v = m_voices[i];
            v.samples          = samples;
            v.frameCount       = frameCount;
            v.baseSampleRateHz = sampleRateHz;
            v.pitchQ16         = pitchQ16;
            v.phaseFracQ16     = 0u;
            v.phaseFrame       = 0u;
            v.channels         = channels;
            const uint8_t vol  = ClampPercent(volumePercent);
            v.volLeft          = vol;
            v.volRight         = vol;
            v.loop             = loop;
            v.active           = true;
            RecomputeStep(v);
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

Result PcmMixer::SetVoiceSampleRate(int8_t voice, uint32_t sampleRateHz) {
    if (voice < 0 || voice >= static_cast<int8_t>(kMaxPcmVoices) || sampleRateHz == 0u) {
        return Result::InvalidArg;
    }
    Voice& v = m_voices[voice];
    if (!v.active) {
        return Result::NotInitialized;
    }
    v.baseSampleRateHz = sampleRateHz;
    RecomputeStep(v);
    return Result::Ok;
}

Result PcmMixer::SetVoicePitch(int8_t voice, uint32_t pitchQ16) {
    if (voice < 0 || voice >= static_cast<int8_t>(kMaxPcmVoices)) {
        return Result::InvalidArg;
    }
    Voice& v = m_voices[voice];
    if (!v.active) {
        return Result::NotInitialized;
    }
    v.pitchQ16 = pitchQ16;
    RecomputeStep(v);
    return Result::Ok;
}

Result PcmMixer::SetVoiceVolume(int8_t voice, uint8_t leftPercent, uint8_t rightPercent) {
    if (voice < 0 || voice >= static_cast<int8_t>(kMaxPcmVoices)) {
        return Result::InvalidArg;
    }
    Voice& v = m_voices[voice];
    if (!v.active) {
        return Result::NotInitialized;
    }
    v.volLeft  = ClampPercent(leftPercent);
    v.volRight = ClampPercent(rightPercent);
    return Result::Ok;
}

Result PcmMixer::SetVoiceVolume(int8_t voice, uint8_t volumePercent) {
    return SetVoiceVolume(voice, volumePercent, volumePercent);
}

void PcmMixer::StopVoice(int8_t voice) {
    if (voice < 0 || voice >= static_cast<int8_t>(kMaxPcmVoices)) {
        return;
    }
    m_voices[voice].active = false;
    m_voices[voice].samples = nullptr;
}

void PcmMixer::StopAllVoices() {
    for (uint8_t i = 0u; i < kMaxPcmVoices; ++i) {
        m_voices[i].samples          = nullptr;
        m_voices[i].frameCount       = 0u;
        m_voices[i].baseSampleRateHz = 0u;
        m_voices[i].pitchQ16         = 0x10000u;
        m_voices[i].stepQ16          = 0u;
        m_voices[i].phaseFracQ16     = 0u;
        m_voices[i].phaseFrame       = 0u;
        m_voices[i].channels         = 1u;
        m_voices[i].volLeft          = 0u;
        m_voices[i].volRight         = 0u;
        m_voices[i].loop             = false;
        m_voices[i].active           = false;
    }
}

bool PcmMixer::IsVoicePlaying(int8_t voice) const {
    if (voice < 0 || voice >= static_cast<int8_t>(kMaxPcmVoices)) {
        return false;
    }
    return m_voices[voice].active;
}

void PcmMixer::Mix(int16_t* outStereoInterleaved, uint32_t outFrames) {
    if (!outStereoInterleaved || outFrames == 0u) {
        return;
    }

    for (uint32_t f = 0u; f < outFrames; ++f) {
        int32_t mixL = 0;
        int32_t mixR = 0;

        for (uint8_t i = 0u; i < kMaxPcmVoices; ++i) {
            Voice& v = m_voices[i];
            if (!v.active || !v.samples || v.frameCount == 0u) {
                continue;
            }

            const uint32_t idx0 = v.phaseFrame;
            uint32_t idx1 = idx0 + 1u;
            if (idx1 >= v.frameCount) {
                idx1 = v.loop ? 0u : (v.frameCount - 1u);
            }

            const int32_t frac = static_cast<int32_t>(v.phaseFracQ16);
            int32_t sL = 0;
            int32_t sR = 0;

            if (v.channels == 1u) {
                const int32_t a = v.samples[idx0];
                const int32_t b = v.samples[idx1];
                const int32_t interp = a + (((b - a) * frac) >> 16);
                sL = interp;
                sR = interp;
            } else {
                const int32_t aL = v.samples[idx0 * 2u];
                const int32_t bL = v.samples[idx1 * 2u];
                const int32_t aR = v.samples[idx0 * 2u + 1u];
                const int32_t bR = v.samples[idx1 * 2u + 1u];
                sL = aL + (((bL - aL) * frac) >> 16);
                sR = aR + (((bR - aR) * frac) >> 16);
            }

            const int32_t gainL = static_cast<int32_t>(v.volLeft) * static_cast<int32_t>(m_masterVolume);
            const int32_t gainR = static_cast<int32_t>(v.volRight) * static_cast<int32_t>(m_masterVolume);
            mixL += (sL * gainL) / 10000;
            mixR += (sR * gainR) / 10000;

            const uint32_t nextFrac = v.phaseFracQ16 + v.stepQ16;
            v.phaseFrame  += (nextFrac >> 16);
            v.phaseFracQ16 = nextFrac & 0xFFFFu;

            if (v.phaseFrame >= v.frameCount) {
                if (v.loop) {
                    v.phaseFrame %= v.frameCount;
                } else {
                    v.active  = false;
                    v.samples = nullptr;
                }
            }
        }

        outStereoInterleaved[f * 2u]      = ClampSample(mixL);
        outStereoInterleaved[f * 2u + 1u] = ClampSample(mixR);
    }
}

} // namespace audio
} // namespace zsdk
