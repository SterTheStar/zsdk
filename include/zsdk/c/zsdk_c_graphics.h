/**
 * @file zsdk_c_graphics.h
 * @brief C99 ABI for ZSDK GLContext, Texture, MeshBuffer, and Renderer.
 */

#ifndef ZSDK_C_GRAPHICS_H
#define ZSDK_C_GRAPHICS_H

#include "zsdk/c/zsdk_c_types.h"

ZSDK_C_EXTERN_BEGIN

typedef struct zsdk_gl_context_t  zsdk_gl_context_t;
typedef struct zsdk_texture_t     zsdk_texture_t;
typedef struct zsdk_mesh_buffer_t zsdk_mesh_buffer_t;
typedef struct zsdk_sprite_batch_t zsdk_sprite_batch_t;
typedef struct zsdk_frustum_t      zsdk_frustum_t;

typedef enum zsdk_blend_mode_t {
    ZSDK_BLEND_OPAQUE   = 0,
    ZSDK_BLEND_ALPHA    = 1,
    ZSDK_BLEND_ADDITIVE = 2
} zsdk_blend_mode_t;

typedef enum zsdk_primitive_type_t {
    ZSDK_PRIM_TRIANGLES      = 0,
    ZSDK_PRIM_TRIANGLE_STRIP = 1,
    ZSDK_PRIM_LINES          = 2,
    ZSDK_PRIM_POINTS         = 3
} zsdk_primitive_type_t;

typedef enum zsdk_tex_format_t {
    ZSDK_TEX_ALPHA8   = 0,
    ZSDK_TEX_RGB565   = 1,
    ZSDK_TEX_RGBA4444 = 2,
    ZSDK_TEX_RGBA5551 = 3,
    ZSDK_TEX_RGB888   = 4,
    ZSDK_TEX_RGBA8888 = 5
} zsdk_tex_format_t;

typedef enum zsdk_tex_filter_t {
    ZSDK_FILTER_NEAREST = 0,
    ZSDK_FILTER_LINEAR  = 1
} zsdk_tex_filter_t;

typedef enum zsdk_tex_wrap_t {
    ZSDK_WRAP_CLAMP_TO_EDGE = 0,
    ZSDK_WRAP_REPEAT        = 1
} zsdk_tex_wrap_t;

typedef struct zsdk_gl_config_t {
    uint8_t red_bits;
    uint8_t green_bits;
    uint8_t blue_bits;
    uint8_t alpha_bits;
    uint8_t depth_bits;
    uint8_t stencil_bits;
    uint8_t swap_interval;
} zsdk_gl_config_t;

/* GLContext */
ZSDK_C_API zsdk_gl_context_t* zsdk_gl_create(void);
ZSDK_C_API void               zsdk_gl_destroy(zsdk_gl_context_t* ctx);
ZSDK_C_API zsdk_result_t      zsdk_gl_init(zsdk_gl_context_t* ctx, void* platform_ctx, const zsdk_gl_config_t* cfg);
ZSDK_C_API void               zsdk_gl_shutdown(zsdk_gl_context_t* ctx);
ZSDK_C_API zsdk_result_t      zsdk_gl_make_current(zsdk_gl_context_t* ctx);
ZSDK_C_API zsdk_result_t      zsdk_gl_swap_buffers(zsdk_gl_context_t* ctx);
ZSDK_C_API zsdk_result_t      zsdk_gl_set_swap_interval(zsdk_gl_context_t* ctx, int32_t interval);
ZSDK_C_API int32_t            zsdk_gl_get_width(const zsdk_gl_context_t* ctx);
ZSDK_C_API int32_t            zsdk_gl_get_height(const zsdk_gl_context_t* ctx);

/* Texture */
ZSDK_C_API zsdk_texture_t* zsdk_texture_create(void);
ZSDK_C_API void            zsdk_texture_destroy(zsdk_texture_t* tex);
ZSDK_C_API zsdk_result_t   zsdk_texture_init_2d(zsdk_texture_t* tex, uint16_t width, uint16_t height,
                                                zsdk_tex_format_t fmt, const void* pixels, zsdk_tex_filter_t filter);
ZSDK_C_API zsdk_result_t   zsdk_texture_create_from_atitc(zsdk_texture_t* tex, const void* file_bytes,
                                                          uint32_t file_size_bytes, zsdk_tex_filter_t filter);
ZSDK_C_API zsdk_result_t   zsdk_texture_update_sub_image(zsdk_texture_t* tex, uint16_t x, uint16_t y,
                                                         uint16_t width, uint16_t height, const void* pixels);
ZSDK_C_API zsdk_result_t   zsdk_texture_set_wrap(zsdk_texture_t* tex, zsdk_tex_wrap_t wrap_s, zsdk_tex_wrap_t wrap_t);
ZSDK_C_API void            zsdk_texture_bind(const zsdk_texture_t* tex, uint8_t unit);
ZSDK_C_API void            zsdk_texture_release(zsdk_texture_t* tex);
ZSDK_C_API int             zsdk_texture_is_valid(const zsdk_texture_t* tex);
ZSDK_C_API uint16_t        zsdk_texture_get_width(const zsdk_texture_t* tex);
ZSDK_C_API uint16_t        zsdk_texture_get_height(const zsdk_texture_t* tex);
ZSDK_C_API uint32_t        zsdk_texture_get_gl_id(const zsdk_texture_t* tex);

/* MeshBuffer */
ZSDK_C_API zsdk_mesh_buffer_t* zsdk_mesh_create(void);
ZSDK_C_API void                zsdk_mesh_destroy(zsdk_mesh_buffer_t* buf);
ZSDK_C_API zsdk_result_t       zsdk_mesh_init(zsdk_mesh_buffer_t* buf, int is_index_buffer, int is_dynamic,
                                              uint32_t size_bytes, const void* data);
ZSDK_C_API zsdk_result_t       zsdk_mesh_update(zsdk_mesh_buffer_t* buf, uint32_t offset_bytes,
                                                uint32_t size_bytes, const void* data);
ZSDK_C_API void                zsdk_mesh_release(zsdk_mesh_buffer_t* buf);
ZSDK_C_API int                 zsdk_mesh_is_valid(const zsdk_mesh_buffer_t* buf);

/* Renderer */
ZSDK_C_API void zsdk_render_reset_state_cache(void);
ZSDK_C_API void zsdk_render_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
ZSDK_C_API void zsdk_render_set_scissor(int enabled, int32_t x, int32_t y, int32_t width, int32_t height);
ZSDK_C_API void zsdk_render_clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int clear_depth, int clear_stencil);
ZSDK_C_API void zsdk_render_set_blend_mode(zsdk_blend_mode_t mode);
ZSDK_C_API void zsdk_render_set_depth_test(int enabled);
ZSDK_C_API void zsdk_render_set_cull_face(int enabled);
ZSDK_C_API void zsdk_render_set_ortho(float left, float right, float bottom, float top, float near_z, float far_z);
ZSDK_C_API void zsdk_render_set_perspective(float fov_y_deg, float aspect, float near_z, float far_z);
ZSDK_C_API void zsdk_render_reset_modelview(void);
ZSDK_C_API void zsdk_render_look_at(float eye_x, float eye_y, float eye_z,
                                    float target_x, float target_y, float target_z,
                                    float up_x, float up_y, float up_z);
ZSDK_C_API void zsdk_render_push_matrix(void);
ZSDK_C_API void zsdk_render_pop_matrix(void);
ZSDK_C_API void zsdk_render_translate(float x, float y, float z);
ZSDK_C_API void zsdk_render_rotate(float angle_deg, float x, float y, float z);
ZSDK_C_API void zsdk_render_scale(float x, float y, float z);
ZSDK_C_API void zsdk_render_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
ZSDK_C_API void zsdk_render_bind_raw_vertex_arrays(const void* positions, uint8_t pos_components,
                                                   const void* normals, const void* colors,
                                                   const void* tex_coords, int32_t stride_bytes);
ZSDK_C_API void zsdk_render_unbind_vertex_attributes(void);
ZSDK_C_API void zsdk_render_draw_arrays(zsdk_primitive_type_t type, int32_t first_vertex, int32_t vertex_count);
ZSDK_C_API void zsdk_render_draw_elements_u16(zsdk_primitive_type_t type, int32_t index_count, const uint16_t* indices);
ZSDK_C_API void zsdk_render_begin_2d(int32_t viewport_width, int32_t viewport_height);
ZSDK_C_API void zsdk_render_begin_2d_virtual(float virtual_width, float virtual_height, int32_t real_width, int32_t real_height);
ZSDK_C_API void zsdk_render_end_2d(void);
ZSDK_C_API void zsdk_render_draw_rect_2d(float x, float y, float width, float height,
                                         uint8_t r, uint8_t g, uint8_t b, uint8_t a);
ZSDK_C_API void zsdk_render_draw_text_2d(float x, float y, float scale, const char* text,
                                         uint8_t r, uint8_t g, uint8_t b, uint8_t a);
ZSDK_C_API void zsdk_render_draw_textured_quad_2d(const zsdk_texture_t* tex, float x, float y, float width, float height,
                                                  float u0, float v0, float u1, float v1,
                                                  uint8_t r, uint8_t g, uint8_t b, uint8_t a);
ZSDK_C_API void zsdk_render_set_fog(int enabled);
ZSDK_C_API void zsdk_render_configure_fog(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                                          float start_z, float end_z, float density);
ZSDK_C_API void zsdk_render_set_lighting(int enabled);
ZSDK_C_API void zsdk_render_set_alpha_test(int enabled, uint8_t compare_func, float ref_val);
ZSDK_C_API void zsdk_render_set_stencil_test(int enabled);
ZSDK_C_API void zsdk_render_set_color_mask(int r, int g, int b, int a);
ZSDK_C_API void zsdk_render_draw_blob_shadow(float x, float y, float z, float radius,
                                             uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* SpriteBatch2D */
ZSDK_C_API zsdk_sprite_batch_t* zsdk_batch_create(void);
ZSDK_C_API void                 zsdk_batch_destroy(zsdk_sprite_batch_t* batch);
ZSDK_C_API zsdk_result_t        zsdk_batch_init(zsdk_sprite_batch_t* batch, uint16_t max_sprites);
ZSDK_C_API void                 zsdk_batch_shutdown(zsdk_sprite_batch_t* batch);
ZSDK_C_API void                 zsdk_batch_begin(zsdk_sprite_batch_t* batch, int32_t viewport_w, int32_t viewport_h);
ZSDK_C_API void                 zsdk_batch_begin_virtual(zsdk_sprite_batch_t* batch, float vw, float vh, int32_t rw, int32_t rh);
ZSDK_C_API void                 zsdk_batch_end(zsdk_sprite_batch_t* batch);
ZSDK_C_API void                 zsdk_batch_flush(zsdk_sprite_batch_t* batch);
ZSDK_C_API void                 zsdk_batch_draw_rect(zsdk_sprite_batch_t* batch, float x, float y, float w, float h,
                                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a);
ZSDK_C_API void                 zsdk_batch_draw_quad(zsdk_sprite_batch_t* batch, const zsdk_texture_t* tex,
                                                     float x, float y, float w, float h,
                                                     float u0, float v0, float u1, float v1,
                                                     uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* Frustum */
ZSDK_C_API zsdk_frustum_t* zsdk_frustum_create(void);
ZSDK_C_API void            zsdk_frustum_destroy(zsdk_frustum_t* frustum);
ZSDK_C_API void            zsdk_frustum_extract_camera(zsdk_frustum_t* frustum,
                                                       float ex, float ey, float ez,
                                                       float tx, float ty, float tz,
                                                       float ux, float uy, float uz,
                                                       float fov_y_deg, float aspect, float near_z, float far_z);
ZSDK_C_API int             zsdk_frustum_test_sphere(const zsdk_frustum_t* frustum, float cx, float cy, float cz, float radius);
ZSDK_C_API int             zsdk_frustum_test_aabb(const zsdk_frustum_t* frustum,
                                                  float min_x, float min_y, float min_z,
                                                  float max_x, float max_y, float max_z);

ZSDK_C_EXTERN_END

#endif /* ZSDK_C_GRAPHICS_H */
