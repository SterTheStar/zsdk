/**
 * @file cube_c.c
 * @brief Minimal pure C99 example: 1 spinning colored cube + camera pan/zoom/spin + ZSDK Debug Panel.
 */

#include "cube_c.h"

typedef struct CubeVertex {
    float   x, y, z;
    uint8_t r, g, b, a;
} CubeVertex;

static const CubeVertex kCubeVertices[24] = {
    /* Front (+Z) - Cyan */
    {-0.8f, -0.8f,  0.8f,  60, 180, 255, 255},
    { 0.8f, -0.8f,  0.8f,  60, 180, 255, 255},
    { 0.8f,  0.8f,  0.8f,  60, 180, 255, 255},
    {-0.8f,  0.8f,  0.8f,  60, 180, 255, 255},
    /* Back (-Z) - Orange */
    { 0.8f, -0.8f, -0.8f, 255, 140,  50, 255},
    {-0.8f, -0.8f, -0.8f, 255, 140,  50, 255},
    {-0.8f,  0.8f, -0.8f, 255, 140,  50, 255},
    { 0.8f,  0.8f, -0.8f, 255, 140,  50, 255},
    /* Left (-X) - Green */
    {-0.8f, -0.8f, -0.8f,  70, 220, 120, 255},
    {-0.8f, -0.8f,  0.8f,  70, 220, 120, 255},
    {-0.8f,  0.8f,  0.8f,  70, 220, 120, 255},
    {-0.8f,  0.8f, -0.8f,  70, 220, 120, 255},
    /* Right (+X) - Magenta */
    { 0.8f, -0.8f,  0.8f, 230,  80, 190, 255},
    { 0.8f, -0.8f, -0.8f, 230,  80, 190, 255},
    { 0.8f,  0.8f, -0.8f, 230,  80, 190, 255},
    { 0.8f,  0.8f,  0.8f, 230,  80, 190, 255},
    /* Top (+Y) - Yellow */
    {-0.8f,  0.8f,  0.8f, 250, 220,  60, 255},
    { 0.8f,  0.8f,  0.8f, 250, 220,  60, 255},
    { 0.8f,  0.8f, -0.8f, 250, 220,  60, 255},
    {-0.8f,  0.8f, -0.8f, 250, 220,  60, 255},
    /* Bottom (-Y) - Red */
    {-0.8f, -0.8f, -0.8f, 235,  65,  65, 255},
    { 0.8f, -0.8f, -0.8f, 235,  65,  65, 255},
    { 0.8f, -0.8f,  0.8f, 235,  65,  65, 255},
    {-0.8f, -0.8f,  0.8f, 235,  65,  65, 255}
};

static const uint16_t kCubeIndices[36] = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23
};

static zsdk_gl_context_t* g_gl = NULL;
static float g_angleY   = 0.0f;
static float g_angleX   = 0.0f;
static float g_panX     = 0.0f;
static float g_panY     = 0.0f;
static float g_camDist  = 4.0f;
static int   g_autoSpin = 1;

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int cube_c_init(void* platform_ctx) {
    zsdk_gl_config_t cfg = { 5u, 6u, 5u, 0u, 16u, 0u, 1u };
    g_gl = zsdk_gl_create();
    if (!g_gl || zsdk_gl_init(g_gl, platform_ctx, &cfg) != ZSDK_C_OK) {
        ZSDK_C_LOG_ERROR("CUBE_C", "GLContext init failed");
        return -1;
    }

    zsdk_render_reset_state_cache();
    zsdk_render_set_depth_test(1);
    zsdk_render_set_cull_face(1);
    zsdk_render_set_lighting(0);
    zsdk_debug_overlay_set_visible(1);

    ZSDK_C_LOG_INFO("CUBE_C", "Ready %dx%d (RAM=%u B)",
                    (int)zsdk_gl_get_width(g_gl),
                    (int)zsdk_gl_get_height(g_gl),
                    (unsigned)zsdk_memory_ram_used());
    return 0;
}

void cube_c_update(float dt_sec, const zsdk_input_t* input) {
    if (g_autoSpin) {
        g_angleY += dt_sec * 48.0f;
        g_angleX += dt_sec * 24.0f;
    }

    if (input) {
        float lx = 0.0f, ly = 0.0f;
        float rx = 0.0f, ry = 0.0f;
        zsdk_input_left_stick(input, 0u, &lx, &ly);
        zsdk_input_right_stick(input, 0u, &rx, &ry);

        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_LEFT))  lx -= 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_RIGHT)) lx += 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_UP))    ly -= 1.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_DPAD_DOWN))  ly += 1.0f;

        g_panX = clampf(g_panX + lx * 4.0f * dt_sec, -6.0f, 6.0f);
        g_panY = clampf(g_panY - ly * 4.0f * dt_sec, -4.0f, 4.0f);

        g_angleY += rx * 120.0f * dt_sec;

        float zoom = -ry * 6.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_SHOULDER_LEFT))  zoom -= 6.0f;
        if (zsdk_input_is_down(input, 0u, ZSDK_BTN_SHOULDER_RIGHT)) zoom += 6.0f;
        g_camDist = clampf(g_camDist + zoom * dt_sec, 2.2f, 20.0f);

        if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION1)) {
            g_autoSpin = !g_autoSpin;
            ZSDK_C_LOG_INFO("CUBE_C", "Auto-spin: %s", g_autoSpin ? "ON" : "OFF");
        }
        if (zsdk_input_is_pressed(input, 0u, ZSDK_BTN_ACTION2)) {
            g_panX = 0.0f;
            g_panY = 0.0f;
            g_camDist = 4.0f;
            ZSDK_C_LOG_INFO("CUBE_C", "Camera reset");
        }
    }

    if (g_angleY >= 360.0f) g_angleY -= 360.0f;
    if (g_angleX >= 360.0f) g_angleX -= 360.0f;
}

void cube_c_render(int32_t* out_width, int32_t* out_height) {
    int32_t w = zsdk_gl_get_width(g_gl);
    int32_t h = zsdk_gl_get_height(g_gl);
    float aspect = (h > 0) ? ((float)w / (float)h) : 1.333f;

    zsdk_render_set_viewport(0, 0, w, h);
    zsdk_render_set_depth_test(1);
    zsdk_render_set_cull_face(1);
    zsdk_render_set_blend_mode(ZSDK_BLEND_OPAQUE);
    zsdk_render_clear(20, 24, 34, 255, 1, 0);

    zsdk_render_set_perspective(45.0f, aspect, 0.1f, 50.0f);
    zsdk_render_reset_modelview();
    zsdk_render_look_at(g_panX, 1.2f + g_panY, g_camDist,
                        g_panX, g_panY, 0.0f,
                        0.0f, 1.0f, 0.0f);

    zsdk_render_draw_blob_shadow(0.0f, -1.25f, 0.0f, 1.15f, 0, 0, 0, 110);

    zsdk_render_push_matrix();
    zsdk_render_rotate(g_angleY, 0.0f, 1.0f, 0.0f);
    zsdk_render_rotate(g_angleX, 1.0f, 0.0f, 0.0f);
    zsdk_render_bind_raw_vertex_arrays(&kCubeVertices[0].x, 3, NULL, &kCubeVertices[0].r,
                                       NULL, (int32_t)sizeof(CubeVertex));
    zsdk_render_draw_elements_u16(ZSDK_PRIM_TRIANGLES, 36, kCubeIndices);
    zsdk_render_unbind_vertex_attributes();
    zsdk_render_pop_matrix();

    zsdk_debug_overlay_record_draw_call(24u);

    if (out_width)  *out_width  = w;
    if (out_height) *out_height = h;
}

void cube_c_swap(void) {
    if (g_gl) {
        zsdk_gl_swap_buffers(g_gl);
    }
}

void cube_c_shutdown(void) {
    if (g_gl) {
        zsdk_gl_shutdown(g_gl);
        zsdk_gl_destroy(g_gl);
        g_gl = NULL;
    }
}
