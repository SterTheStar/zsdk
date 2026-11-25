/**
 * @file cube_c.c
 * @brief ZSDK Example 02 (Pure C99): 1:1 clone of 01_spinning_cube.
 *        1..1024 Spinning Lit 3D Cubes + Frustum/Face Culling + SpriteBatch2D Menu + udata Save.
 */

#include "zsdk/c/zsdk_c.h"

static const uint16_t kCubeCounts[11] = {
    1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u
};

typedef struct CubeVertex {
    float   x, y, z;
    float   nx, ny, nz;
    uint8_t r, g, b, a;
} CubeVertex;

static const CubeVertex kCubeVertices[24] = {
    /* Front (+Z) - Cyan */
    {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  60, 180, 255, 255},
    /* Back (-Z) - Orange */
    { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 255, 140,  50, 255},
    /* Left (-X) - Green */
    {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  70, 220, 120, 255},
    /* Right (+X) - Magenta */
    { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 230,  80, 190, 255},
    /* Top (+Y) - Yellow */
    {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 250, 220,  60, 255},
    /* Bottom (-Y) - Red */
    {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255},
    {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 235,  65,  65, 255}
};

static const uint16_t kCubeIndices[36] = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23
};

typedef struct CubeSettings {
    uint8_t lighting;
    uint8_t fpsMode;
    uint8_t vsync;
    uint8_t spinMode;
    uint8_t cubeCountIndex;
    uint8_t frustumCull;
    uint8_t faceCull;
} CubeSettings;

#define kMenuItemCount 10u
#define kSaveVersion   2u

static zsdk_gl_context_t*   g_gl      = NULL;
static zsdk_sprite_batch_t* g_batch   = NULL;
static zsdk_frustum_t*      g_frustum = NULL;
static zsdk_savegame_t*     g_saveMgr = NULL;

static float   g_angleX        = 25.0f;
static float   g_angleY        = 35.0f;
static float   g_camPanX       = 0.0f;
static float   g_camPanY       = 0.0f;
static float   g_camZoomOffset = 0.0f;
static int     g_menuOpen      = 0;
static uint8_t g_menuCursor    = 0u;
#if ZSDK_PLATFORM_ZEEBO
static CubeSettings g_settings = {1u, 1u, 1u, 2u, 0u, 1u, 1u};
#else
static CubeSettings g_settings = {1u, 0u, 1u, 2u, 0u, 1u, 1u};
#endif

static int AppendStr(char* dst, int pos, int maxLen, const char* src) {
    while (*src != '\0' && pos < maxLen - 1) {
        dst[pos++] = *src++;
    }
    dst[pos] = '\0';
    return pos;
}

static int AppendUInt(char* dst, int pos, int maxLen, uint32_t val) {
    char tmp[12];
    int n = 0;
    if (val == 0u) {
        tmp[n++] = '0';
    } else {
        while (val > 0u && n < 11) {
            tmp[n++] = (char)('0' + (val % 10u));
            val /= 10u;
        }
    }
    while (n > 0 && pos < maxLen - 1) {
        dst[pos++] = tmp[--n];
    }
    dst[pos] = '\0';
    return pos;
}

static void GetGridDims(uint8_t countIdx, uint16_t* cols, uint16_t* rows, float* camDist) {
    static const uint16_t kCols[11] = {1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 16};
    static const uint16_t kRows[11] = {1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8};
    static const float    kDist[11] = {4.5f, 6.5f, 8.0f, 10.5f, 13.0f, 16.0f, 19.0f, 23.0f, 28.0f, 33.0f, 38.0f};
    const uint8_t idx = (countIdx <= 10u) ? countIdx : 10u;
    *cols    = kCols[idx];
    *rows    = kRows[idx];
    *camDist = kDist[idx];
}

static void ApplyFps(void) {
    zsdk_app_set_fps_cap((g_settings.fpsMode & 1u) ? 30u : 60u);
}

static void ApplySettings(void) {
    ApplyFps();
    if (g_gl) {
        zsdk_gl_set_swap_interval(g_gl, g_settings.vsync ? 1 : 0);
    }
}

static void AdjustMenuItem(int dir) {
    switch (g_menuCursor) {
        case 0:
            g_settings.cubeCountIndex = (uint8_t)((g_settings.cubeCountIndex + 11 + dir) % 11);
            g_camPanX = 0.0f;
            g_camPanY = 0.0f;
            g_camZoomOffset = 0.0f;
            break;
        case 1:
            g_settings.frustumCull = g_settings.frustumCull ? 0u : 1u;
            break;
        case 2:
            g_settings.faceCull = g_settings.faceCull ? 0u : 1u;
            break;
        case 3:
            g_settings.lighting = g_settings.lighting ? 0u : 1u;
            break;
        case 4:
            g_settings.fpsMode = g_settings.fpsMode ? 0u : 1u;
            ApplyFps();
            break;
        case 5:
            g_settings.vsync = g_settings.vsync ? 0u : 1u;
            if (g_gl) zsdk_gl_set_swap_interval(g_gl, g_settings.vsync ? 1 : 0);
            break;
        case 6:
            g_settings.spinMode = (uint8_t)((g_settings.spinMode + 4 + dir) % 4);
            break;
        case 7:
            zsdk_debug_overlay_set_visible(!zsdk_debug_overlay_is_visible());
            break;
        case 8:
            if (g_saveMgr) {
                zsdk_savegame_save_slot(g_saveMgr, 0u, &g_settings, (uint32_t)sizeof(g_settings), kSaveVersion);
                ZSDK_C_LOG_INFO("CUBE_C", "Saved settings to udata.");
            }
            break;
        case 9:
            zsdk_app_request_quit();
            break;
        default:
            break;
    }
}

static void UpdateMenu(const zsdk_input_t* input) {
    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_DPAD_UP)) {
        g_menuCursor = (g_menuCursor == 0u) ? (uint8_t)(kMenuItemCount - 1u) : (uint8_t)(g_menuCursor - 1u);
    }
    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_DPAD_DOWN)) {
        g_menuCursor = (uint8_t)((g_menuCursor + 1u) % kMenuItemCount);
    }
    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_DPAD_RIGHT) ||
        zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION1)) {
        AdjustMenuItem(1);
    }
    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_DPAD_LEFT)) {
        AdjustMenuItem(-1);
    }
    if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_BACK)) {
        g_menuOpen = 0;
    }
}

static void RenderHud(int32_t w, int32_t h, uint16_t totalCubes, uint16_t drawnCubes) {
    zsdk_batch_begin_virtual(g_batch, 640.0f, 480.0f, w, h);
    zsdk_batch_draw_rect(g_batch, 10.0f, 10.0f, 290.0f, 36.0f, 10, 14, 22, 180);
    zsdk_batch_flush(g_batch);

    char line1[48];
    int p = AppendStr(line1, 0, (int)sizeof(line1), "CUBES: ");
    p = AppendUInt(line1, p, (int)sizeof(line1), totalCubes);
    p = AppendStr(line1, p, (int)sizeof(line1), "  DRAWN: ");
    AppendUInt(line1, p, (int)sizeof(line1), drawnCubes);

    char line2[48];
    p = AppendStr(line2, 0, (int)sizeof(line2), "FRUSTUM:");
    p = AppendStr(line2, p, (int)sizeof(line2), g_settings.frustumCull ? "ON " : "OFF ");
    p = AppendStr(line2, p, (int)sizeof(line2), "FACE:");
    AppendStr(line2, p, (int)sizeof(line2), g_settings.faceCull ? "ON" : "OFF");

    zsdk_render_draw_text_2d(18.0f, 16.0f, 1.0f, line1, 235, 245, 255, 255);
    zsdk_render_draw_text_2d(18.0f, 30.0f, 1.0f, line2, 160, 215, 255, 255);
    zsdk_batch_end(g_batch);
}

static void RenderMenu(int32_t w, int32_t h) {
    zsdk_batch_begin_virtual(g_batch, 640.0f, 480.0f, w, h);

    const float menuW = 360.0f;
    const float menuH = 224.0f;
    const float mx = (640.0f - menuW) * 0.5f;
    const float my = (480.0f - menuH) * 0.5f;

    zsdk_batch_draw_rect(g_batch, mx, my, menuW, menuH, 12, 16, 26, 230);
    zsdk_batch_draw_rect(g_batch, mx, my, menuW, 24.0f, 35, 75, 130, 240);
    for (uint8_t i = 0u; i < kMenuItemCount; ++i) {
        if (i == g_menuCursor) {
            const float ry = my + 34.0f + (float)i * 18.0f;
            zsdk_batch_draw_rect(g_batch, mx + 8.0f, ry - 3.0f, menuW - 16.0f, 15.0f, 55, 110, 180, 200);
        }
    }
    zsdk_batch_flush(g_batch);

    zsdk_render_draw_text_2d(mx + 12.0f, my + 8.0f, 1.0f, "ZSDK SETTINGS (START)", 255, 255, 255, 255);

    static const char* const fpsNames[2]  = {"60 FPS", "30 FPS"};
    static const char* const spinNames[4] = {"Paused", "Slow", "Normal", "Fast"};

    char cubeBuf[40];
    int pos = AppendStr(cubeBuf, 0, (int)sizeof(cubeBuf), "Cube Count  : ");
    AppendUInt(cubeBuf, pos, (int)sizeof(cubeBuf), kCubeCounts[g_settings.cubeCountIndex]);

    char fpsBuf[40];
    pos = AppendStr(fpsBuf, 0, (int)sizeof(fpsBuf), "FPS Cap     : ");
    AppendStr(fpsBuf, pos, (int)sizeof(fpsBuf), fpsNames[g_settings.fpsMode & 1u]);

    char spinBuf[40];
    pos = AppendStr(spinBuf, 0, (int)sizeof(spinBuf), "Spin Speed  : ");
    AppendStr(spinBuf, pos, (int)sizeof(spinBuf), spinNames[g_settings.spinMode & 3u]);

    const char* labels[kMenuItemCount] = {
        cubeBuf,
        g_settings.frustumCull ? "Frustum Cull: ON"  : "Frustum Cull: OFF",
        g_settings.faceCull    ? "Face Cull   : ON"  : "Face Cull   : OFF",
        g_settings.lighting    ? "Lighting    : ON"  : "Lighting    : OFF",
        fpsBuf,
        g_settings.vsync       ? "VSync       : ON"  : "VSync       : OFF",
        spinBuf,
        zsdk_debug_overlay_is_visible() ? "Debug Panel : SHOW" : "Debug Panel : HIDE",
        "Save Config (udata)",
        "Exit App"
    };

    for (uint8_t i = 0u; i < kMenuItemCount; ++i) {
        const float ry = my + 34.0f + (float)i * 18.0f;
        if (i == g_menuCursor) {
            zsdk_render_draw_text_2d(mx + 16.0f, ry, 1.0f, labels[i], 255, 240, 120, 255);
        } else {
            zsdk_render_draw_text_2d(mx + 16.0f, ry, 1.0f, labels[i], 210, 220, 230, 255);
        }
    }

    zsdk_batch_end(g_batch);
}

int zsdk_c_app_init(void* platform_ctx) {
    zsdk_gl_config_t cfg = {5u, 6u, 5u, 0u, 16u, 0u, 1u};
    g_gl = zsdk_gl_create();
    if (!g_gl || zsdk_gl_init(g_gl, platform_ctx, &cfg) != ZSDK_C_OK) {
        ZSDK_C_LOG_ERROR("CUBE_C", "GLContext init failed");
        return -1;
    }

    g_batch = zsdk_batch_create();
    if (!g_batch || zsdk_batch_init(g_batch, 64u) != ZSDK_C_OK) {
        return -1;
    }

    g_frustum = zsdk_frustum_create();
    if (!g_frustum) {
        return -1;
    }

    zsdk_render_configure_light(0u, 6.0f, 8.0f, 10.0f,
                                64u, 64u, 80u, 255u,
                                242u, 242u, 230u, 255u,
                                204u, 204u, 204u, 255u);

    g_saveMgr = zsdk_savegame_create();
    if (g_saveMgr && zsdk_savegame_init(g_saveMgr, platform_ctx, 0) == ZSDK_C_OK) {
        CubeSettings loaded;
        uint32_t bytesRead = 0u;
        if (zsdk_savegame_load_slot(g_saveMgr, 0u, &loaded, (uint32_t)sizeof(loaded), &bytesRead, kSaveVersion) == ZSDK_C_OK &&
            bytesRead == (uint32_t)sizeof(loaded)) {
            if (loaded.cubeCountIndex > 10u) loaded.cubeCountIndex = 0u;
            loaded.spinMode    &= 3u;
            loaded.frustumCull = loaded.frustumCull ? 1u : 0u;
            loaded.faceCull    = loaded.faceCull ? 1u : 0u;
            g_settings = loaded;
            ApplySettings();
            ZSDK_C_LOG_INFO("CUBE_C", "Loaded settings from udata.");
        } else {
            ZSDK_C_LOG_INFO("CUBE_C", "Spinning Cube ready.");
        }
    }
    return 0;
}

void zsdk_c_app_update(uint32_t delta_ms, const zsdk_input_t* input) {
    if (!input) return;
    const float dt = (float)delta_ms * 0.001f;

    const int menuBtnPressed = zsdk_input_is_pressed(input, 0u, ZSDK_BTN_START) ||
                               zsdk_input_is_pressed(input, 0u, ZSDK_BTN_HOME);
    if (menuBtnPressed && !zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_DOWN)) {
        g_menuOpen = !g_menuOpen;
        ZSDK_C_LOG_INFO("MENU", "%s", g_menuOpen ? "Opened" : "Closed");
    }

    if (g_menuOpen) {
        UpdateMenu(input);
    } else {
        static const float spinTable[4] = {0.0f, 20.0f, 48.0f, 110.0f};
        const float baseSpin = spinTable[g_settings.spinMode & 3u];
        g_angleY += baseSpin * dt;
        g_angleX += (baseSpin * 0.65f) * dt;

        float panX = 0.0f, panY = 0.0f;
        zsdk_input_left_stick(input, 0u, &panX, &panY);
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_LEFT))  panX -= 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_RIGHT)) panX += 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_UP))    panY += 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_DOWN))  panY -= 1.0f;

        g_camPanX += panX * 16.0f * dt;
        g_camPanY -= panY * 16.0f * dt;
        if (g_camPanX >  40.0f) g_camPanX =  40.0f;
        if (g_camPanX < -40.0f) g_camPanX = -40.0f;
        if (g_camPanY >  30.0f) g_camPanY =  30.0f;
        if (g_camPanY < -30.0f) g_camPanY = -30.0f;

        float rx = 0.0f, ry = 0.0f;
        zsdk_input_right_stick(input, 0u, &rx, &ry);
        g_camZoomOffset += ry * 20.0f * dt;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_SHOULDER_LEFT))  g_camZoomOffset -= 14.0f * dt;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_SHOULDER_RIGHT)) g_camZoomOffset += 14.0f * dt;
        if (g_camZoomOffset < -30.0f) g_camZoomOffset = -30.0f;
        if (g_camZoomOffset >  50.0f) g_camZoomOffset =  50.0f;

        if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION1)) {
            g_settings.lighting = g_settings.lighting ? 0u : 1u;
            ZSDK_C_LOG_INFO("CUBE_C", "Lighting: %s", g_settings.lighting ? "ON" : "OFF");
        }
        if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION2)) {
            g_settings.fpsMode = (g_settings.fpsMode == 1u) ? 0u : 1u;
            ApplyFps();
        }
    }
}

void zsdk_c_app_render(int32_t* out_width, int32_t* out_height) {
    const int32_t w = zsdk_gl_get_width(g_gl);
    const int32_t h = zsdk_gl_get_height(g_gl);

    zsdk_render_set_viewport(0, 0, w, h);
    zsdk_render_clear(16, 20, 32, 255, 1, 0);

    const float aspect = (h > 0) ? ((float)w / (float)h) : 1.333f;
    const float fovY   = 55.0f;
    const float nearZ  = 0.5f;
    const float farZ   = 150.0f;

    uint16_t cols = 1u, rows = 1u;
    float camDist = 4.5f;
    GetGridDims(g_settings.cubeCountIndex, &cols, &rows, &camDist);
    camDist += g_camZoomOffset;
    if (camDist < 2.5f)  camDist = 2.5f;
    if (camDist > 90.0f) camDist = 90.0f;

    zsdk_render_set_perspective(fovY, aspect, nearZ, farZ);
    zsdk_render_reset_modelview();
    zsdk_render_look_at(g_camPanX, g_camPanY, camDist,
                        g_camPanX, g_camPanY, 0.0f,
                        0.0f, 1.0f, 0.0f);

    if (g_settings.frustumCull != 0u) {
        zsdk_frustum_extract_camera(g_frustum,
                                    g_camPanX, g_camPanY, camDist,
                                    g_camPanX, g_camPanY, 0.0f,
                                    0.0f, 1.0f, 0.0f,
                                    fovY, aspect, nearZ, farZ);
    }

    zsdk_render_set_depth_test(1);
    zsdk_render_set_cull_face(g_settings.faceCull != 0u);
    zsdk_render_set_lighting(g_settings.lighting != 0u);

    zsdk_render_bind_raw_vertex_arrays(&kCubeVertices[0].x, 3, &kCubeVertices[0].nx,
                                       &kCubeVertices[0].r, NULL, (int32_t)sizeof(CubeVertex));

    const uint16_t totalCubes = kCubeCounts[g_settings.cubeCountIndex];
    const uint16_t perLayer   = (uint16_t)(cols * rows);
    const uint16_t layers     = (uint16_t)((totalCubes + perLayer - 1u) / perLayer);
    const float    spacing    = 3.0f;
    const float    halfW      = (float)(cols - 1u) * 0.5f * spacing;
    const float    halfH      = (float)(rows - 1u) * 0.5f * spacing;
    const float    halfD      = (float)(layers - 1u) * 0.5f * spacing;
    const float    cubeRadius = 1.75f;

    uint16_t drawn = 0u;
    for (uint16_t i = 0u; i < totalCubes; ++i) {
        const uint16_t l  = i / perLayer;
        const uint16_t r  = i % perLayer;
        const uint16_t cx = r % cols;
        const uint16_t cy = r / cols;

        const float px = (float)cx * spacing - halfW;
        const float py = (float)cy * spacing - halfH;
        const float pz = -(float)l * spacing + halfD;

        if (g_settings.frustumCull != 0u) {
            if (!zsdk_frustum_test_sphere(g_frustum, px, py, pz, cubeRadius)) {
                continue;
            }
        }

        zsdk_render_push_matrix();
        zsdk_render_translate(px, py, pz);
        zsdk_render_rotate(g_angleX + (float)(i & 7u) * 12.0f, 1.0f, 0.0f, 0.0f);
        zsdk_render_rotate(g_angleY + (float)((i >> 3u) & 7u) * 12.0f, 0.0f, 1.0f, 0.0f);
        zsdk_render_draw_elements_u16(ZSDK_PRIM_TRIANGLES, 36, kCubeIndices);
        zsdk_render_pop_matrix();
        zsdk_debug_overlay_record_draw_call(24u);
        ++drawn;
    }

    zsdk_render_unbind_vertex_attributes();
    zsdk_render_set_lighting(0);

    if (totalCubes == 1u) {
        zsdk_render_draw_blob_shadow(0.0f, -1.6f, 0.0f, 1.45f, 0, 0, 0, 120);
    }

    zsdk_render_set_depth_test(0);

    RenderHud(w, h, totalCubes, drawn);
    if (g_menuOpen) {
        RenderMenu(w, h);
    }

    if (out_width)  *out_width  = w;
    if (out_height) *out_height = h;
}

void zsdk_c_app_swap(void) {
    if (g_gl) {
        zsdk_gl_swap_buffers(g_gl);
    }
}

void zsdk_c_app_shutdown(void) {
    if (g_saveMgr) { zsdk_savegame_shutdown(g_saveMgr); zsdk_savegame_destroy(g_saveMgr); g_saveMgr = NULL; }
    if (g_frustum) { zsdk_frustum_destroy(g_frustum); g_frustum = NULL; }
    if (g_batch)   { zsdk_batch_shutdown(g_batch); zsdk_batch_destroy(g_batch); g_batch = NULL; }
    if (g_gl)      { zsdk_gl_shutdown(g_gl); zsdk_gl_destroy(g_gl); g_gl = NULL; }
}
