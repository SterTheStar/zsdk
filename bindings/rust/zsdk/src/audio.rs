use ::core::ffi::{CStr, c_void};
use zsdk_sys::*;
use crate::{check, Error, Result};

pub struct AudioEngine {
    raw: *mut zsdk_audio_engine_t,
}

impl AudioEngine {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_audio_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_audio_init(raw, platform_ctx) }) {
                unsafe { zsdk_audio_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }

    pub fn play_music(&mut self, path: &CStr, looping: bool) -> Result<()> { check(unsafe { zsdk_audio_play_music(self.raw, path.as_ptr(), looping as i32) }) }
    pub fn stop_music(&mut self) { unsafe { zsdk_audio_stop_music(self.raw); } }
    pub fn set_music_volume(&mut self, vol: u8) { unsafe { zsdk_audio_set_music_volume(self.raw, vol); } }
    pub fn play_sfx(&mut self, path: &CStr) -> Result<()> { check(unsafe { zsdk_audio_play_sfx(self.raw, path.as_ptr()) }) }
    pub fn set_sfx_volume(&mut self, vol: u8) { unsafe { zsdk_audio_set_sfx_volume(self.raw, vol); } }
    pub fn stop_all_sfx(&mut self) { unsafe { zsdk_audio_stop_all_sfx(self.raw); } }
    pub fn set_master_volume(&mut self, vol: u8) { unsafe { zsdk_audio_set_master_volume(self.raw, vol); } }
}

impl Drop for AudioEngine {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe { zsdk_audio_shutdown(self.raw); zsdk_audio_destroy(self.raw); }
        }
    }
}

pub struct PcmMixer {
    raw: *mut zsdk_pcm_mixer_t,
}

impl PcmMixer {
    pub fn new(output_sample_rate_hz: u32) -> Result<Self> {
        let raw = unsafe { zsdk_pcm_mixer_create(output_sample_rate_hz) };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }

    pub fn set_output_rate(&mut self, hz: u32) { unsafe { zsdk_pcm_mixer_set_output_rate(self.raw, hz); } }
    pub fn set_master_volume(&mut self, vol: u8) { unsafe { zsdk_pcm_mixer_set_master_volume(self.raw, vol); } }
    pub fn play(&mut self, samples: &[i16], sample_rate_hz: u32, channels: u8, looping: bool, vol_percent: u8, pitch_q16: u32) -> i8 {
        let ch = if channels == 0 { 1 } else { channels as usize };
        let frames = (samples.len() / ch) as u32;
        unsafe { zsdk_pcm_mixer_play(self.raw, samples.as_ptr(), frames, sample_rate_hz, channels, looping as i32, vol_percent, pitch_q16) }
    }
    pub fn set_voice_rate(&mut self, voice: i8, hz: u32) -> Result<()> { check(unsafe { zsdk_pcm_mixer_set_voice_rate(self.raw, voice, hz) }) }
    pub fn set_voice_pitch(&mut self, voice: i8, pitch_q16: u32) -> Result<()> { check(unsafe { zsdk_pcm_mixer_set_voice_pitch(self.raw, voice, pitch_q16) }) }
    pub fn set_voice_volume(&mut self, voice: i8, left: u8, right: u8) -> Result<()> { check(unsafe { zsdk_pcm_mixer_set_voice_volume(self.raw, voice, left, right) }) }
    pub fn stop_voice(&mut self, voice: i8) { unsafe { zsdk_pcm_mixer_stop_voice(self.raw, voice); } }
    pub fn stop_all(&mut self) { unsafe { zsdk_pcm_mixer_stop_all(self.raw); } }
    pub fn is_voice_playing(&self, voice: i8) -> bool { unsafe { zsdk_pcm_mixer_is_voice_playing(self.raw, voice) != 0 } }
    pub fn mix(&mut self, out_stereo: &mut [i16]) {
        let frames = (out_stereo.len() / 2) as u32;
        unsafe { zsdk_pcm_mixer_mix(self.raw, out_stereo.as_mut_ptr(), frames); }
    }
    pub fn output_rate(&self) -> u32 { unsafe { zsdk_pcm_mixer_get_output_rate(self.raw) } }
    pub fn master_volume(&self) -> u8 { unsafe { zsdk_pcm_mixer_get_master_volume(self.raw) } }
}

impl Drop for PcmMixer {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_pcm_mixer_destroy(self.raw); } }
    }
}

pub struct MediaStream {
    raw: *mut zsdk_media_stream_t,
}

impl MediaStream {
    pub fn new() -> Result<Self> {
        let raw = unsafe { zsdk_media_create() };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }

    pub fn load_file(&mut self, platform_ctx: *mut c_void, path: &CStr) -> Result<()> { check(unsafe { zsdk_media_load_file(self.raw, platform_ctx, path.as_ptr()) }) }
    pub fn load_memory(&mut self, platform_ctx: *mut c_void, data: &[u8], media_format: u8) -> Result<()> {
        check(unsafe { zsdk_media_load_memory(self.raw, platform_ctx, data.as_ptr() as *const _, data.len() as u32, media_format) })
    }
    pub fn unload(&mut self) { unsafe { zsdk_media_unload(self.raw); } }
    pub fn play(&mut self) -> Result<()> { check(unsafe { zsdk_media_play(self.raw) }) }
    pub fn stop(&mut self) -> Result<()> { check(unsafe { zsdk_media_stop(self.raw) }) }
    pub fn pause(&mut self) -> Result<()> { check(unsafe { zsdk_media_pause(self.raw) }) }
    pub fn resume(&mut self) -> Result<()> { check(unsafe { zsdk_media_resume(self.raw) }) }
    pub fn set_volume(&mut self, vol_percent: u8) -> Result<()> { check(unsafe { zsdk_media_set_volume(self.raw, vol_percent) }) }
    pub fn set_looping(&mut self, looping: bool) { unsafe { zsdk_media_set_looping(self.raw, looping as i32); } }
    pub fn is_playing(&self) -> bool { unsafe { zsdk_media_is_playing(self.raw) != 0 } }
    pub fn is_loaded(&self) -> bool { unsafe { zsdk_media_is_loaded(self.raw) != 0 } }
}

impl Drop for MediaStream {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_media_destroy(self.raw); } }
    }
}

pub struct Sfx {
    raw: *mut zsdk_sfx_t,
}

impl Sfx {
    pub fn new(platform_ctx: *mut c_void) -> Result<Self> {
        let raw = unsafe { zsdk_sfx_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if !platform_ctx.is_null() {
            if let Err(e) = check(unsafe { zsdk_sfx_init(raw, platform_ctx) }) {
                unsafe { zsdk_sfx_destroy(raw); }
                return Err(e);
            }
        }
        Ok(Self { raw })
    }

    pub fn play_file(&mut self, path: &CStr) -> Result<()> { check(unsafe { zsdk_sfx_play_file(self.raw, path.as_ptr()) }) }
    pub fn play_memory(&mut self, data: &[u8]) -> Result<()> { check(unsafe { zsdk_sfx_play_memory(self.raw, data.as_ptr() as *const _, data.len() as u32) }) }
    pub fn stop(&mut self) -> Result<()> { check(unsafe { zsdk_sfx_stop(self.raw) }) }
    pub fn set_volume(&mut self, vol_percent: u8) -> Result<()> { check(unsafe { zsdk_sfx_set_volume(self.raw, vol_percent) }) }
}

impl Drop for Sfx {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe { zsdk_sfx_shutdown(self.raw); zsdk_sfx_destroy(self.raw); }
        }
    }
}
