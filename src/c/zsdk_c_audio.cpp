/**
 * @file zsdk_c_audio.cpp
 * @brief C ABI bridge for zsdk::audio::AudioEngine and PcmMixer.
 */

#include "zsdk/c/zsdk_c_audio.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/audio/audio_engine.hpp"
#include "zsdk/audio/pcm_mixer.hpp"

struct zsdk_audio_engine_t {
    zsdk::audio::AudioEngine engine;
};

struct zsdk_pcm_mixer_t {
    explicit zsdk_pcm_mixer_t(uint32_t rate) : mixer(rate) {}
    zsdk::audio::PcmMixer mixer;
};

struct zsdk_media_stream_t {
    zsdk::audio::MediaStream stream;
};

struct zsdk_sfx_t {
    zsdk::audio::SoundEffect sfx;
};

extern "C" {

zsdk_audio_engine_t* zsdk_audio_create(void) {
    return zsdk::c_abi::Alloc<zsdk_audio_engine_t>(zsdk::debug::MemorySubsystem::Audio);
}

void zsdk_audio_destroy(zsdk_audio_engine_t* engine) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Audio, engine);
}

zsdk_result_t zsdk_audio_init(zsdk_audio_engine_t* engine, void* platform_ctx) {
    if (!engine) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(engine->engine.Initialize(platform_ctx));
}

void zsdk_audio_shutdown(zsdk_audio_engine_t* engine) {
    if (engine) engine->engine.Shutdown();
}

zsdk_result_t zsdk_audio_play_music(zsdk_audio_engine_t* engine, const char* file_path, int loop) {
    if (!engine) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(engine->engine.PlayMusic(file_path, loop != 0));
}

void zsdk_audio_stop_music(zsdk_audio_engine_t* engine) {
    if (engine) engine->engine.StopMusic();
}

void zsdk_audio_set_music_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent) {
    if (engine) engine->engine.SetMusicVolume(vol_percent);
}

zsdk_result_t zsdk_audio_play_sfx(zsdk_audio_engine_t* engine, const char* file_path) {
    if (!engine) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(engine->engine.PlaySfx(file_path));
}

void zsdk_audio_set_sfx_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent) {
    if (engine) engine->engine.SetSfxVolume(vol_percent);
}

void zsdk_audio_stop_all_sfx(zsdk_audio_engine_t* engine) {
    if (engine) engine->engine.StopAllSfx();
}

void zsdk_audio_set_master_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent) {
    if (engine) engine->engine.SetMasterVolume(vol_percent);
}

zsdk_pcm_mixer_t* zsdk_pcm_mixer_create(uint32_t output_sample_rate_hz) {
    return zsdk::c_abi::AllocArg<zsdk_pcm_mixer_t>(zsdk::debug::MemorySubsystem::Audio, output_sample_rate_hz);
}

void zsdk_pcm_mixer_destroy(zsdk_pcm_mixer_t* mixer) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Audio, mixer);
}

void zsdk_pcm_mixer_set_output_rate(zsdk_pcm_mixer_t* mixer, uint32_t output_rate_hz) {
    if (mixer) mixer->mixer.SetOutputSampleRate(output_rate_hz);
}

void zsdk_pcm_mixer_set_master_volume(zsdk_pcm_mixer_t* mixer, uint8_t vol_percent) {
    if (mixer) mixer->mixer.SetMasterVolume(vol_percent);
}

int8_t zsdk_pcm_mixer_play(zsdk_pcm_mixer_t* mixer, const int16_t* samples,
                           uint32_t frame_count, uint32_t sample_rate_hz,
                           uint8_t channels, int loop, uint8_t vol_percent,
                           uint32_t pitch_q16) {
    if (!mixer) return -1;
    return mixer->mixer.Play(samples, frame_count, sample_rate_hz, channels,
                             loop != 0, vol_percent, pitch_q16);
}

zsdk_result_t zsdk_pcm_mixer_set_voice_rate(zsdk_pcm_mixer_t* mixer, int8_t voice, uint32_t sample_rate_hz) {
    if (!mixer) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(mixer->mixer.SetVoiceSampleRate(voice, sample_rate_hz));
}

zsdk_result_t zsdk_pcm_mixer_set_voice_pitch(zsdk_pcm_mixer_t* mixer, int8_t voice, uint32_t pitch_q16) {
    if (!mixer) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(mixer->mixer.SetVoicePitch(voice, pitch_q16));
}

zsdk_result_t zsdk_pcm_mixer_set_voice_volume(zsdk_pcm_mixer_t* mixer, int8_t voice, uint8_t left_pct, uint8_t right_pct) {
    if (!mixer) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(mixer->mixer.SetVoiceVolume(voice, left_pct, right_pct));
}

void zsdk_pcm_mixer_stop_voice(zsdk_pcm_mixer_t* mixer, int8_t voice) {
    if (mixer) mixer->mixer.StopVoice(voice);
}

void zsdk_pcm_mixer_stop_all(zsdk_pcm_mixer_t* mixer) {
    if (mixer) mixer->mixer.StopAllVoices();
}

int zsdk_pcm_mixer_is_voice_playing(const zsdk_pcm_mixer_t* mixer, int8_t voice) {
    return (mixer && mixer->mixer.IsVoicePlaying(voice)) ? 1 : 0;
}

void zsdk_pcm_mixer_mix(zsdk_pcm_mixer_t* mixer, int16_t* out_stereo, uint32_t out_frames) {
    if (mixer) mixer->mixer.Mix(out_stereo, out_frames);
}

zsdk_media_stream_t* zsdk_media_create(void) {
    return zsdk::c_abi::Alloc<zsdk_media_stream_t>(zsdk::debug::MemorySubsystem::Audio);
}

void zsdk_media_destroy(zsdk_media_stream_t* stream) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Audio, stream);
}

zsdk_result_t zsdk_media_load_file(zsdk_media_stream_t* stream, void* platform_ctx, const char* file_path) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.LoadFromFile(platform_ctx, file_path));
}

void zsdk_media_unload(zsdk_media_stream_t* stream) {
    if (stream) stream->stream.Unload();
}

zsdk_result_t zsdk_media_play(zsdk_media_stream_t* stream) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.Play());
}

zsdk_result_t zsdk_media_stop(zsdk_media_stream_t* stream) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.Stop());
}

zsdk_result_t zsdk_media_pause(zsdk_media_stream_t* stream) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.Pause());
}

zsdk_result_t zsdk_media_resume(zsdk_media_stream_t* stream) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.Resume());
}

zsdk_result_t zsdk_media_set_volume(zsdk_media_stream_t* stream, uint8_t vol_percent) {
    if (!stream) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(stream->stream.SetVolume(vol_percent));
}

void zsdk_media_set_looping(zsdk_media_stream_t* stream, int loop) {
    if (stream) stream->stream.SetLooping(loop != 0);
}

int zsdk_media_is_playing(const zsdk_media_stream_t* stream) {
    return (stream && stream->stream.IsPlaying()) ? 1 : 0;
}

zsdk_sfx_t* zsdk_sfx_create(void) {
    return zsdk::c_abi::Alloc<zsdk_sfx_t>(zsdk::debug::MemorySubsystem::Audio);
}

void zsdk_sfx_destroy(zsdk_sfx_t* sfx) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Audio, sfx);
}

zsdk_result_t zsdk_sfx_init(zsdk_sfx_t* sfx, void* platform_ctx) {
    if (!sfx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sfx->sfx.Initialize(platform_ctx));
}

void zsdk_sfx_shutdown(zsdk_sfx_t* sfx) {
    if (sfx) sfx->sfx.Shutdown();
}

zsdk_result_t zsdk_sfx_play_file(zsdk_sfx_t* sfx, const char* file_path) {
    if (!sfx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sfx->sfx.PlayFile(file_path));
}

zsdk_result_t zsdk_sfx_stop(zsdk_sfx_t* sfx) {
    if (!sfx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sfx->sfx.Stop());
}

zsdk_result_t zsdk_sfx_set_volume(zsdk_sfx_t* sfx, uint8_t vol_percent) {
    if (!sfx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sfx->sfx.SetVolume(vol_percent));
}

uint32_t zsdk_pcm_mixer_get_output_rate(const zsdk_pcm_mixer_t* mixer) {
    return mixer ? mixer->mixer.GetOutputSampleRate() : 0u;
}

uint8_t zsdk_pcm_mixer_get_master_volume(const zsdk_pcm_mixer_t* mixer) {
    return mixer ? mixer->mixer.GetMasterVolume() : 0u;
}

int zsdk_media_is_loaded(const zsdk_media_stream_t* stream) {
    return (stream && stream->stream.IsLoaded()) ? 1 : 0;
}

zsdk_result_t zsdk_sfx_play_memory(zsdk_sfx_t* sfx, const void* data, uint32_t size_bytes) {
    if (!sfx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(sfx->sfx.PlayMemory(data, size_bytes));
}

} /* extern "C" */
