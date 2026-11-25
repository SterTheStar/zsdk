/**
 * @file zsdk_c_audio.h
 * @brief C99 ABI for ZSDK AudioEngine and PcmMixer (with live pitch/sample-rate control).
 */

#ifndef ZSDK_C_AUDIO_H
#define ZSDK_C_AUDIO_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_audio_engine_t zsdk_audio_engine_t;
typedef struct zsdk_pcm_mixer_t    zsdk_pcm_mixer_t;
typedef struct zsdk_media_stream_t zsdk_media_stream_t;
typedef struct zsdk_sfx_t          zsdk_sfx_t;

/* AudioEngine (BGM + pooled SFX) */
ZSDK_C_API zsdk_audio_engine_t* zsdk_audio_create(void);
ZSDK_C_API void                 zsdk_audio_destroy(zsdk_audio_engine_t* engine);
ZSDK_C_API zsdk_result_t        zsdk_audio_init(zsdk_audio_engine_t* engine, void* platform_ctx);
ZSDK_C_API void                 zsdk_audio_shutdown(zsdk_audio_engine_t* engine);
ZSDK_C_API zsdk_result_t        zsdk_audio_play_music(zsdk_audio_engine_t* engine, const char* file_path, int loop);
ZSDK_C_API void                 zsdk_audio_stop_music(zsdk_audio_engine_t* engine);
ZSDK_C_API void                 zsdk_audio_set_music_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent);
ZSDK_C_API zsdk_result_t        zsdk_audio_play_sfx(zsdk_audio_engine_t* engine, const char* file_path);
ZSDK_C_API void                 zsdk_audio_set_sfx_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent);
ZSDK_C_API void                 zsdk_audio_stop_all_sfx(zsdk_audio_engine_t* engine);
ZSDK_C_API void                 zsdk_audio_set_master_volume(zsdk_audio_engine_t* engine, uint8_t vol_percent);

/* Software PcmMixer (live sample-rate & pitch modulation) */
ZSDK_C_API zsdk_pcm_mixer_t* zsdk_pcm_mixer_create(uint32_t output_sample_rate_hz);
ZSDK_C_API void              zsdk_pcm_mixer_destroy(zsdk_pcm_mixer_t* mixer);
ZSDK_C_API void              zsdk_pcm_mixer_set_output_rate(zsdk_pcm_mixer_t* mixer, uint32_t output_rate_hz);
ZSDK_C_API void              zsdk_pcm_mixer_set_master_volume(zsdk_pcm_mixer_t* mixer, uint8_t vol_percent);
ZSDK_C_API int8_t            zsdk_pcm_mixer_play(zsdk_pcm_mixer_t* mixer, const int16_t* samples,
                                                 uint32_t frame_count, uint32_t sample_rate_hz,
                                                 uint8_t channels, int loop, uint8_t vol_percent,
                                                 uint32_t pitch_q16);
ZSDK_C_API zsdk_result_t     zsdk_pcm_mixer_set_voice_rate(zsdk_pcm_mixer_t* mixer, int8_t voice, uint32_t sample_rate_hz);
ZSDK_C_API zsdk_result_t     zsdk_pcm_mixer_set_voice_pitch(zsdk_pcm_mixer_t* mixer, int8_t voice, uint32_t pitch_q16);
ZSDK_C_API zsdk_result_t     zsdk_pcm_mixer_set_voice_volume(zsdk_pcm_mixer_t* mixer, int8_t voice, uint8_t left_pct, uint8_t right_pct);
ZSDK_C_API void              zsdk_pcm_mixer_stop_voice(zsdk_pcm_mixer_t* mixer, int8_t voice);
ZSDK_C_API void              zsdk_pcm_mixer_stop_all(zsdk_pcm_mixer_t* mixer);
ZSDK_C_API int               zsdk_pcm_mixer_is_voice_playing(const zsdk_pcm_mixer_t* mixer, int8_t voice);
ZSDK_C_API void              zsdk_pcm_mixer_mix(zsdk_pcm_mixer_t* mixer, int16_t* out_stereo, uint32_t out_frames);

/* MediaStream */
ZSDK_C_API zsdk_media_stream_t* zsdk_media_create(void);
ZSDK_C_API void                 zsdk_media_destroy(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_load_file(zsdk_media_stream_t* stream, void* platform_ctx, const char* file_path);
ZSDK_C_API void                 zsdk_media_unload(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_play(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_stop(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_pause(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_resume(zsdk_media_stream_t* stream);
ZSDK_C_API zsdk_result_t        zsdk_media_set_volume(zsdk_media_stream_t* stream, uint8_t vol_percent);
ZSDK_C_API void                 zsdk_media_set_looping(zsdk_media_stream_t* stream, int loop);
ZSDK_C_API int                  zsdk_media_is_playing(const zsdk_media_stream_t* stream);

/* SoundEffect */
ZSDK_C_API zsdk_sfx_t*   zsdk_sfx_create(void);
ZSDK_C_API void          zsdk_sfx_destroy(zsdk_sfx_t* sfx);
ZSDK_C_API zsdk_result_t zsdk_sfx_init(zsdk_sfx_t* sfx, void* platform_ctx);
ZSDK_C_API void          zsdk_sfx_shutdown(zsdk_sfx_t* sfx);
ZSDK_C_API zsdk_result_t zsdk_sfx_play_file(zsdk_sfx_t* sfx, const char* file_path);
ZSDK_C_API zsdk_result_t zsdk_sfx_stop(zsdk_sfx_t* sfx);
ZSDK_C_API zsdk_result_t zsdk_sfx_set_volume(zsdk_sfx_t* sfx, uint8_t vol_percent);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_AUDIO_H */
