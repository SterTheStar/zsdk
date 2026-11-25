/**
 * @file zsdk_lua_audio.c
 * @brief Complete Lua bindings for zsdk_c_audio (AudioEngine, PcmMixer, MediaStream, SoundEffect).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_audio.h"

#define ZSDK_LUA_AUDIO_MT "zsdk.AudioEngine"
#define ZSDK_LUA_MIXER_MT "zsdk.PcmMixer"
#define ZSDK_LUA_MEDIA_MT "zsdk.MediaStream"
#define ZSDK_LUA_SFX_MT   "zsdk.SoundEffect"

typedef struct { zsdk_audio_engine_t* ptr; } zsdk_lua_audio_ud;
typedef struct { zsdk_pcm_mixer_t*    ptr; } zsdk_lua_mixer_ud;
typedef struct { zsdk_media_stream_t* ptr; } zsdk_lua_media_ud;
typedef struct { zsdk_sfx_t*          ptr; } zsdk_lua_sfx_ud;

static zsdk_audio_engine_t* check_audio(lua_State* L, int idx) {
    return ((zsdk_lua_audio_ud*)luaL_checkudata(L, idx, ZSDK_LUA_AUDIO_MT))->ptr;
}

static zsdk_pcm_mixer_t* check_mixer(lua_State* L, int idx) {
    return ((zsdk_lua_mixer_ud*)luaL_checkudata(L, idx, ZSDK_LUA_MIXER_MT))->ptr;
}

static zsdk_media_stream_t* check_media(lua_State* L, int idx) {
    return ((zsdk_lua_media_ud*)luaL_checkudata(L, idx, ZSDK_LUA_MEDIA_MT))->ptr;
}

static zsdk_sfx_t* check_sfx(lua_State* L, int idx) {
    return ((zsdk_lua_sfx_ud*)luaL_checkudata(L, idx, ZSDK_LUA_SFX_MT))->ptr;
}

/* AudioEngine */
static int l_audio_gc(lua_State* L) {
    zsdk_lua_audio_ud* ud = (zsdk_lua_audio_ud*)luaL_checkudata(L, 1, ZSDK_LUA_AUDIO_MT);
    if (ud->ptr) { zsdk_audio_shutdown(ud->ptr); zsdk_audio_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_audio_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_audio_engine_t* eng = zsdk_audio_create();
    if (!eng) return luaL_error(L, "zsdk.audio.new: out of memory");
    if (ctx && zsdk_audio_init(eng, ctx) != ZSDK_C_OK) { zsdk_audio_destroy(eng); return luaL_error(L, "init failed"); }
    zsdk_lua_audio_ud* ud = (zsdk_lua_audio_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = eng;
    luaL_getmetatable(L, ZSDK_LUA_AUDIO_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_audio_play_music(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_audio_play_music(
        check_audio(L, 1), luaL_checkstring(L, 2), lua_isnone(L, 3) ? 1 : lua_toboolean(L, 3)));
    return 1;
}
static int l_audio_stop_music(lua_State* L)        { zsdk_audio_stop_music(check_audio(L, 1)); return 0; }
static int l_audio_set_music_volume(lua_State* L)  { zsdk_audio_set_music_volume(check_audio(L, 1), (uint8_t)luaL_checkinteger(L, 2)); return 0; }
static int l_audio_play_sfx(lua_State* L)          { lua_pushinteger(L, (lua_Integer)zsdk_audio_play_sfx(check_audio(L, 1), luaL_checkstring(L, 2))); return 1; }
static int l_audio_set_sfx_volume(lua_State* L)    { zsdk_audio_set_sfx_volume(check_audio(L, 1), (uint8_t)luaL_checkinteger(L, 2)); return 0; }
static int l_audio_stop_all_sfx(lua_State* L)      { zsdk_audio_stop_all_sfx(check_audio(L, 1)); return 0; }
static int l_audio_set_master_volume(lua_State* L) { zsdk_audio_set_master_volume(check_audio(L, 1), (uint8_t)luaL_checkinteger(L, 2)); return 0; }

/* PcmMixer */
static int l_mixer_gc(lua_State* L) {
    zsdk_lua_mixer_ud* ud = (zsdk_lua_mixer_ud*)luaL_checkudata(L, 1, ZSDK_LUA_MIXER_MT);
    if (ud->ptr) { zsdk_pcm_mixer_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_mixer_new(lua_State* L) {
    zsdk_pcm_mixer_t* mx = zsdk_pcm_mixer_create((uint32_t)luaL_optinteger(L, 1, 22050));
    if (!mx) return luaL_error(L, "zsdk.audio.new_pcm_mixer: out of memory");
    zsdk_lua_mixer_ud* ud = (zsdk_lua_mixer_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = mx;
    luaL_getmetatable(L, ZSDK_LUA_MIXER_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_mixer_set_output_rate(lua_State* L) {
    zsdk_pcm_mixer_set_output_rate(check_mixer(L, 1), (uint32_t)luaL_checkinteger(L, 2));
    return 0;
}

static int l_mixer_set_master_volume(lua_State* L) {
    zsdk_pcm_mixer_set_master_volume(check_mixer(L, 1), (uint8_t)luaL_checkinteger(L, 2));
    return 0;
}

static int l_mixer_play_bytes(lua_State* L) {
    size_t len = 0;
    const char* bytes = luaL_checklstring(L, 2, &len);
    uint32_t rate     = (uint32_t)luaL_checkinteger(L, 3);
    uint8_t  ch       = (uint8_t)luaL_optinteger(L, 4, 1);
    int      loop     = lua_toboolean(L, 5);
    uint8_t  vol      = (uint8_t)luaL_optinteger(L, 6, 100);
    uint32_t pitch    = (uint32_t)luaL_optinteger(L, 7, 0x10000);
    uint32_t frames   = (uint32_t)(len / (sizeof(int16_t) * (ch == 2 ? 2 : 1)));
    lua_pushinteger(L, (lua_Integer)zsdk_pcm_mixer_play(
        check_mixer(L, 1), (const int16_t*)bytes, frames, rate, ch, loop, vol, pitch));
    return 1;
}

static int l_mixer_set_voice_rate(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_pcm_mixer_set_voice_rate(
        check_mixer(L, 1), (int8_t)luaL_checkinteger(L, 2), (uint32_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_mixer_set_voice_pitch(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)zsdk_pcm_mixer_set_voice_pitch(
        check_mixer(L, 1), (int8_t)luaL_checkinteger(L, 2), (uint32_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_mixer_set_voice_volume(lua_State* L) {
    uint8_t lvol = (uint8_t)luaL_checkinteger(L, 3);
    uint8_t rvol = (uint8_t)luaL_optinteger(L, 4, lvol);
    lua_pushinteger(L, (lua_Integer)zsdk_pcm_mixer_set_voice_volume(
        check_mixer(L, 1), (int8_t)luaL_checkinteger(L, 2), lvol, rvol));
    return 1;
}

static int l_mixer_stop_voice(lua_State* L)  { zsdk_pcm_mixer_stop_voice(check_mixer(L, 1), (int8_t)luaL_checkinteger(L, 2)); return 0; }
static int l_mixer_stop_all(lua_State* L)    { zsdk_pcm_mixer_stop_all(check_mixer(L, 1)); return 0; }
static int l_mixer_is_playing(lua_State* L)  { lua_pushboolean(L, zsdk_pcm_mixer_is_voice_playing(check_mixer(L, 1), (int8_t)luaL_checkinteger(L, 2))); return 1; }

static int l_mixer_mix_bytes(lua_State* L) {
    uint32_t frames = (uint32_t)luaL_optinteger(L, 2, 256);
    int16_t buf[512 * 2];
    if (frames > 512u) frames = 512u;
    zsdk_pcm_mixer_mix(check_mixer(L, 1), buf, frames);
    lua_pushlstring(L, (const char*)buf, (size_t)(frames * 2u * sizeof(int16_t)));
    return 1;
}

/* MediaStream */
static int l_media_gc(lua_State* L) {
    zsdk_lua_media_ud* ud = (zsdk_lua_media_ud*)luaL_checkudata(L, 1, ZSDK_LUA_MEDIA_MT);
    if (ud->ptr) { zsdk_media_unload(ud->ptr); zsdk_media_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_media_new(lua_State* L) {
    zsdk_media_stream_t* m = zsdk_media_create();
    if (!m) return luaL_error(L, "zsdk.audio.new_media: out of memory");
    zsdk_lua_media_ud* ud = (zsdk_lua_media_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = m;
    luaL_getmetatable(L, ZSDK_LUA_MEDIA_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_media_load_file(lua_State* L)   { lua_pushinteger(L, (lua_Integer)zsdk_media_load_file(check_media(L, 1), lua_touserdata(L, 2), luaL_checkstring(L, 3))); return 1; }
static int l_media_unload(lua_State* L)      { zsdk_media_unload(check_media(L, 1)); return 0; }
static int l_media_play(lua_State* L)        { lua_pushinteger(L, (lua_Integer)zsdk_media_play(check_media(L, 1))); return 1; }
static int l_media_stop(lua_State* L)        { lua_pushinteger(L, (lua_Integer)zsdk_media_stop(check_media(L, 1))); return 1; }
static int l_media_pause(lua_State* L)       { lua_pushinteger(L, (lua_Integer)zsdk_media_pause(check_media(L, 1))); return 1; }
static int l_media_resume(lua_State* L)      { lua_pushinteger(L, (lua_Integer)zsdk_media_resume(check_media(L, 1))); return 1; }
static int l_media_set_volume(lua_State* L)  { lua_pushinteger(L, (lua_Integer)zsdk_media_set_volume(check_media(L, 1), (uint8_t)luaL_checkinteger(L, 2))); return 1; }
static int l_media_set_looping(lua_State* L) { zsdk_media_set_looping(check_media(L, 1), lua_toboolean(L, 2)); return 0; }
static int l_media_is_playing(lua_State* L)  { lua_pushboolean(L, zsdk_media_is_playing(check_media(L, 1))); return 1; }

/* SoundEffect */
static int l_sfx_gc(lua_State* L) {
    zsdk_lua_sfx_ud* ud = (zsdk_lua_sfx_ud*)luaL_checkudata(L, 1, ZSDK_LUA_SFX_MT);
    if (ud->ptr) { zsdk_sfx_shutdown(ud->ptr); zsdk_sfx_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}

static int l_sfx_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_sfx_t* s = zsdk_sfx_create();
    if (!s) return luaL_error(L, "zsdk.audio.new_sfx: out of memory");
    if (ctx && zsdk_sfx_init(s, ctx) != ZSDK_C_OK) { zsdk_sfx_destroy(s); return luaL_error(L, "init failed"); }
    zsdk_lua_sfx_ud* ud = (zsdk_lua_sfx_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = s;
    luaL_getmetatable(L, ZSDK_LUA_SFX_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_sfx_play_file(lua_State* L)  { lua_pushinteger(L, (lua_Integer)zsdk_sfx_play_file(check_sfx(L, 1), luaL_checkstring(L, 2))); return 1; }
static int l_sfx_stop(lua_State* L)       { lua_pushinteger(L, (lua_Integer)zsdk_sfx_stop(check_sfx(L, 1))); return 1; }
static int l_sfx_set_volume(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_sfx_set_volume(check_sfx(L, 1), (uint8_t)luaL_checkinteger(L, 2))); return 1; }

static const luaL_Reg kAudioMethods[] = {
    { "play_music",        l_audio_play_music        },
    { "stop_music",        l_audio_stop_music        },
    { "set_music_volume",  l_audio_set_music_volume  },
    { "play_sfx",          l_audio_play_sfx          },
    { "set_sfx_volume",    l_audio_set_sfx_volume    },
    { "stop_all_sfx",      l_audio_stop_all_sfx      },
    { "set_master_volume", l_audio_set_master_volume },
    { "__gc",              l_audio_gc                },
    { NULL,                NULL                      }
};

static const luaL_Reg kMixerMethods[] = {
    { "set_output_rate",   l_mixer_set_output_rate   },
    { "set_master_volume", l_mixer_set_master_volume },
    { "play_bytes",        l_mixer_play_bytes        },
    { "set_voice_rate",    l_mixer_set_voice_rate    },
    { "set_voice_pitch",   l_mixer_set_voice_pitch   },
    { "set_voice_volume",  l_mixer_set_voice_volume  },
    { "stop_voice",        l_mixer_stop_voice        },
    { "stop_all",          l_mixer_stop_all          },
    { "is_voice_playing",  l_mixer_is_playing        },
    { "mix_bytes",         l_mixer_mix_bytes         },
    { "__gc",              l_mixer_gc                },
    { NULL,                NULL                      }
};

static const luaL_Reg kMediaMethods[] = {
    { "load_file",   l_media_load_file   },
    { "unload",      l_media_unload      },
    { "play",        l_media_play        },
    { "stop",        l_media_stop        },
    { "pause",       l_media_pause       },
    { "resume",      l_media_resume      },
    { "set_volume",  l_media_set_volume  },
    { "set_looping", l_media_set_looping },
    { "is_playing",  l_media_is_playing  },
    { "__gc",        l_media_gc          },
    { NULL,          NULL                }
};

static const luaL_Reg kSfxMethods[] = {
    { "play_file",  l_sfx_play_file  },
    { "stop",       l_sfx_stop       },
    { "set_volume", l_sfx_set_volume },
    { "__gc",       l_sfx_gc         },
    { NULL,         NULL             }
};

static const luaL_Reg kAudioModule[] = {
    { "new",           l_audio_new },
    { "new_pcm_mixer", l_mixer_new },
    { "new_media",     l_media_new },
    { "new_sfx",       l_sfx_new   },
    { NULL,            NULL        }
};

int luaopen_zsdk_audio(lua_State* L) {
    if (luaL_newmetatable(L, ZSDK_LUA_AUDIO_MT)) { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kAudioMethods); } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_MIXER_MT)) { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kMixerMethods); } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_MEDIA_MT)) { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kMediaMethods); } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_SFX_MT))   { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kSfxMethods);   } lua_pop(L, 1);

    lua_newtable(L);
    zsdk_lua_register_funcs(L, kAudioModule);
    return 1;
}
