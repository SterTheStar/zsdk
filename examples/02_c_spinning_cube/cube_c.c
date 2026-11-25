/**
 * @file cube_c.c
 * @brief Minimal pure C99 example: 1 spinning colored cube + ZSDK Debug Panel.
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
static float g_angle = 0.0f;

int cube_c_init(void* platform_ctx) {
    zsdk_gl_config_t cfg = { 5u, 6u, 5u, 0u, 16u, 0u, 1u };
    g_gl = zsdk_gl_create();
    if (!g_gl || zsdk_gl_init(g_gl, platform_ctx, &cfg) != ZSDK_C_OK) {
        ZSDK_C_LOG_ERROR("CUBE_C", "Failed to initialize GLContext");
        return -1;
    }

    zsdk_render_reset_state_cache();
    zsdk_render_set_depth_test(1);
    zsdk_render_set_cull_face(1);
    zsdk_debug_overlay_set_visible(1);

    ZSDK_C_LOG_INFO("CUBE_C", "C99 spinning cube ready (%dx%d)",
                    (int)zsdk_gl_get_width(g_gl), (int)zsdk_gl_get_height(g_gl));
    return 0;
}

void cube_c_update(float dt_sec) {
    g_angle += dt_sec * 45.0f;
    if (g_angle >= 360.0f) {
        g_angle -= 360.0f;
    }
}

void cube_c_render(int32_t* out_width, int32_t* out_height) {
    int32_t w = zsdk_gl_get_width(g_gl);
    int32_t h = zsdk_gl_get_height(g_gl);
    float aspect = (h > 0) ? ((float)w / (float)h) : 1.333f;

    zsdk_render_set_viewport(0, 0, w, h);
    zsdk_render_clear(20, 24, 34, 255, 1, 0);

    zsdk_render_set_perspective(45.0f, aspect, 0.1f, 50.0f);
    zsdk_render_reset_modelview();
    zsdk_render_look_at(0.0f, 1.2f, 3.6f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f);
    zsdk_render_rotate(g_angle, 0.0f, 1.0f, 0.0f);
    zsdk_render_rotate(g_angle * 0.5f, 1.0f, 0.0f, 0.0f);

    zsdk_render_bind_raw_vertex_arrays(&kCubeVertices[0].x, 3, NULL, &kCubeVertices[0].r,
                                       NULL, (int32_t)sizeof(CubeVertex));
    zsdk_render_draw_elements_u16(ZSDK_PRIM_TRIANGLES, 36, kCubeIndices);
    zsdk_render_unbind_vertex_attributes();

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
