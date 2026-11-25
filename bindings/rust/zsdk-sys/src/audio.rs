use core::ffi::{c_char, c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)] pub struct zsdk_audio_engine_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_pcm_mixer_t    { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_media_stream_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_sfx_t          { _private: [u8; 0] }

extern "C" {
    pub fn zsdk_audio_create() -> *mut zsdk_audio_engine_t;
    pub fn zsdk_audio_destroy(engine: *mut zsdk_audio_engine_t);
    pub fn zsdk_audio_init(engine: *mut zsdk_audio_engine_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_audio_shutdown(engine: *mut zsdk_audio_engine_t);
    pub fn zsdk_audio_play_music(engine: *mut zsdk_audio_engine_t, file_path: *const c_char, loop_: c_int) -> zsdk_result_t;
    pub fn zsdk_audio_stop_music(engine: *mut zsdk_audio_engine_t);
    pub fn zsdk_audio_set_music_volume(engine: *mut zsdk_audio_engine_t, vol_percent: u8);
    pub fn zsdk_audio_play_sfx(engine: *mut zsdk_audio_engine_t, file_path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_audio_set_sfx_volume(engine: *mut zsdk_audio_engine_t, vol_percent: u8);
    pub fn zsdk_audio_stop_all_sfx(engine: *mut zsdk_audio_engine_t);
    pub fn zsdk_audio_set_master_volume(engine: *mut zsdk_audio_engine_t, vol_percent: u8);

    pub fn zsdk_pcm_mixer_create(output_sample_rate_hz: u32) -> *mut zsdk_pcm_mixer_t;
    pub fn zsdk_pcm_mixer_destroy(mixer: *mut zsdk_pcm_mixer_t);
    pub fn zsdk_pcm_mixer_set_output_rate(mixer: *mut zsdk_pcm_mixer_t, output_rate_hz: u32);
    pub fn zsdk_pcm_mixer_set_master_volume(mixer: *mut zsdk_pcm_mixer_t, vol_percent: u8);
    pub fn zsdk_pcm_mixer_play(
        mixer: *mut zsdk_pcm_mixer_t,
        samples: *const i16,
        frame_count: u32,
        sample_rate_hz: u32,
        channels: u8,
        loop_: c_int,
        vol_percent: u8,
        pitch_q16: u32,
    ) -> i8;
    pub fn zsdk_pcm_mixer_set_voice_rate(mixer: *mut zsdk_pcm_mixer_t, voice: i8, sample_rate_hz: u32) -> zsdk_result_t;
    pub fn zsdk_pcm_mixer_set_voice_pitch(mixer: *mut zsdk_pcm_mixer_t, voice: i8, pitch_q16: u32) -> zsdk_result_t;
    pub fn zsdk_pcm_mixer_set_voice_volume(mixer: *mut zsdk_pcm_mixer_t, voice: i8, left_pct: u8, right_pct: u8) -> zsdk_result_t;
    pub fn zsdk_pcm_mixer_stop_voice(mixer: *mut zsdk_pcm_mixer_t, voice: i8);
    pub fn zsdk_pcm_mixer_stop_all(mixer: *mut zsdk_pcm_mixer_t);
    pub fn zsdk_pcm_mixer_is_voice_playing(mixer: *const zsdk_pcm_mixer_t, voice: i8) -> c_int;
    pub fn zsdk_pcm_mixer_mix(mixer: *mut zsdk_pcm_mixer_t, out_stereo: *mut i16, out_frames: u32);
    pub fn zsdk_pcm_mixer_get_output_rate(mixer: *const zsdk_pcm_mixer_t) -> u32;
    pub fn zsdk_pcm_mixer_get_master_volume(mixer: *const zsdk_pcm_mixer_t) -> u8;

    pub fn zsdk_media_create() -> *mut zsdk_media_stream_t;
    pub fn zsdk_media_destroy(stream: *mut zsdk_media_stream_t);
    pub fn zsdk_media_load_file(stream: *mut zsdk_media_stream_t, platform_ctx: *mut c_void, file_path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_media_unload(stream: *mut zsdk_media_stream_t);
    pub fn zsdk_media_play(stream: *mut zsdk_media_stream_t) -> zsdk_result_t;
    pub fn zsdk_media_stop(stream: *mut zsdk_media_stream_t) -> zsdk_result_t;
    pub fn zsdk_media_pause(stream: *mut zsdk_media_stream_t) -> zsdk_result_t;
    pub fn zsdk_media_resume(stream: *mut zsdk_media_stream_t) -> zsdk_result_t;
    pub fn zsdk_media_set_volume(stream: *mut zsdk_media_stream_t, vol_percent: u8) -> zsdk_result_t;
    pub fn zsdk_media_set_looping(stream: *mut zsdk_media_stream_t, loop_: c_int);
    pub fn zsdk_media_is_playing(stream: *const zsdk_media_stream_t) -> c_int;
    pub fn zsdk_media_is_loaded(stream: *const zsdk_media_stream_t) -> c_int;

    pub fn zsdk_sfx_create() -> *mut zsdk_sfx_t;
    pub fn zsdk_sfx_destroy(sfx: *mut zsdk_sfx_t);
    pub fn zsdk_sfx_init(sfx: *mut zsdk_sfx_t, platform_ctx: *mut c_void) -> zsdk_result_t;
    pub fn zsdk_sfx_shutdown(sfx: *mut zsdk_sfx_t);
    pub fn zsdk_sfx_play_file(sfx: *mut zsdk_sfx_t, file_path: *const c_char) -> zsdk_result_t;
    pub fn zsdk_sfx_play_memory(sfx: *mut zsdk_sfx_t, data: *const c_void, size_bytes: u32) -> zsdk_result_t;
    pub fn zsdk_sfx_stop(sfx: *mut zsdk_sfx_t) -> zsdk_result_t;
    pub fn zsdk_sfx_set_volume(sfx: *mut zsdk_sfx_t, vol_percent: u8) -> zsdk_result_t;
}
