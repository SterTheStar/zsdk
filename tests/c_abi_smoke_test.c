/**
 * @file c_abi_smoke_test.c
 * @brief Pure C99 compile/link/runtime smoke test for the ZSDK C ABI.
 */

#include "zsdk/c/zsdk_c.h"

#include <stdio.h>

static int check_int(const char* name, int got, int expected) {
    if (got != expected) {
        fprintf(stderr, "FAIL %s: got %d, expected %d\n", name, got, expected);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    failures += check_int("ZSDK_C_OK", (int)ZSDK_C_OK, 0);
    failures += check_int("ZSDK_C_ERR_WOULD_BLOCK", (int)ZSDK_C_ERR_WOULD_BLOCK, -8);
    failures += check_int("ZSDK_C_ERR_NETWORK", (int)ZSDK_C_ERR_NETWORK, -14);

    ZSDK_C_LOG_INFO("TEST", "C99 ABI smoke test uptime=%u ms", (unsigned)zsdk_clock_uptime_ms());

    /* Verify software PcmMixer live pitch/sample-rate API in pure C */
    {
        zsdk_pcm_mixer_t* mixer = zsdk_pcm_mixer_create(22050u);
        int16_t samples[4] = { 0, 1000, 2000, 1000 };
        int16_t out[8] = { 0 };
        int8_t voice = -1;

        if (!mixer) {
            fprintf(stderr, "FAIL zsdk_pcm_mixer_create returned NULL\n");
            return 1;
        }
        voice = zsdk_pcm_mixer_play(mixer, samples, 4u, 22050u, 1u, 1, 100u, 0x10000u);
        failures += check_int("voice >= 0", voice >= 0 ? 1 : 0, 1);
        failures += check_int("set_voice_rate", (int)zsdk_pcm_mixer_set_voice_rate(mixer, voice, 44100u), (int)ZSDK_C_OK);
        failures += check_int("set_voice_pitch", (int)zsdk_pcm_mixer_set_voice_pitch(mixer, voice, 0x18000u), (int)ZSDK_C_OK);
        zsdk_pcm_mixer_mix(mixer, out, 4u);
        zsdk_pcm_mixer_stop_voice(mixer, voice);
        failures += check_int("is_voice_playing", zsdk_pcm_mixer_is_voice_playing(mixer, voice), 0);
        zsdk_pcm_mixer_destroy(mixer);
    }

    /* Verify opaque handle lifecycle for Input, Texture, MeshBuffer, FS, Net */
    {
        zsdk_input_t* inp = zsdk_input_create();
        zsdk_texture_t* tex = zsdk_texture_create();
        zsdk_mesh_buffer_t* mesh = zsdk_mesh_create();
        zsdk_fs_t* fs = zsdk_fs_create();
        zsdk_net_mgr_t* net = zsdk_net_create();

        failures += check_int("zsdk_input_create", inp != NULL ? 1 : 0, 1);
        failures += check_int("zsdk_texture_create", tex != NULL ? 1 : 0, 1);
        failures += check_int("zsdk_mesh_create", mesh != NULL ? 1 : 0, 1);
        failures += check_int("zsdk_fs_create", fs != NULL ? 1 : 0, 1);
        failures += check_int("zsdk_net_create", net != NULL ? 1 : 0, 1);

        failures += check_int("tex_update_uninit",
                              (int)zsdk_texture_update_sub_image(tex, 0, 0, 4, 4, "1234"),
                              (int)ZSDK_C_ERR_NOT_INITIALIZED);

        zsdk_net_destroy(net);
        zsdk_fs_destroy(fs);
        zsdk_mesh_destroy(mesh);
        zsdk_texture_destroy(tex);
        zsdk_input_destroy(inp);
    }

    if (failures != 0) {
        return 1;
    }
    printf("zsdk_c_abi_test: ALL PASSED\n");
    return 0;
}
