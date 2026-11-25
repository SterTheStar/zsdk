use core::ffi::{c_char, c_int, c_void};
use crate::types::zsdk_result_t;

#[repr(C)] pub struct zsdk_gl_context_t   { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_texture_t      { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_mesh_buffer_t  { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_sprite_batch_t { _private: [u8; 0] }
#[repr(C)] pub struct zsdk_frustum_t      { _private: [u8; 0] }

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkBlendMode {
    Opaque   = 0,
    Alpha    = 1,
    Additive = 2,
}
pub type zsdk_blend_mode_t = ZsdkBlendMode;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkPrimitiveType {
    Triangles     = 0,
    TriangleStrip = 1,
    Lines         = 2,
    Points        = 3,
}
pub type zsdk_primitive_type_t = ZsdkPrimitiveType;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkTexFormat {
    Alpha8   = 0,
    Rgb565   = 1,
    Rgba4444 = 2,
    Rgba5551 = 3,
    Rgb888   = 4,
    Rgba8888 = 5,
}
pub type zsdk_tex_format_t = ZsdkTexFormat;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkTexFilter {
    Nearest = 0,
    Linear  = 1,
}
pub type zsdk_tex_filter_t = ZsdkTexFilter;

#[repr(u32)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum ZsdkTexWrap {
    ClampToEdge = 0,
    Repeat      = 1,
}
pub type zsdk_tex_wrap_t = ZsdkTexWrap;

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct zsdk_gl_config_t {
    pub red_bits:      u8,
    pub green_bits:    u8,
    pub blue_bits:     u8,
    pub alpha_bits:    u8,
    pub depth_bits:    u8,
    pub stencil_bits:  u8,
    pub swap_interval: u8,
}

extern "C" {
    pub fn zsdk_gl_create() -> *mut zsdk_gl_context_t;
    pub fn zsdk_gl_destroy(ctx: *mut zsdk_gl_context_t);
    pub fn zsdk_gl_init(ctx: *mut zsdk_gl_context_t, platform_ctx: *mut c_void, cfg: *const zsdk_gl_config_t) -> zsdk_result_t;
    pub fn zsdk_gl_shutdown(ctx: *mut zsdk_gl_context_t);
    pub fn zsdk_gl_make_current(ctx: *mut zsdk_gl_context_t) -> zsdk_result_t;
    pub fn zsdk_gl_swap_buffers(ctx: *mut zsdk_gl_context_t) -> zsdk_result_t;
    pub fn zsdk_gl_has_extension(ctx: *const zsdk_gl_context_t, name: *const c_char) -> c_int;
    pub fn zsdk_gl_set_swap_interval(ctx: *mut zsdk_gl_context_t, interval: i32) -> zsdk_result_t;
    pub fn zsdk_gl_get_width(ctx: *const zsdk_gl_context_t) -> i32;
    pub fn zsdk_gl_get_height(ctx: *const zsdk_gl_context_t) -> i32;

    pub fn zsdk_texture_create() -> *mut zsdk_texture_t;
    pub fn zsdk_texture_destroy(tex: *mut zsdk_texture_t);
    pub fn zsdk_texture_init_2d(tex: *mut zsdk_texture_t, width: u16, height: u16, fmt: zsdk_tex_format_t, pixels: *const c_void, filter: zsdk_tex_filter_t) -> zsdk_result_t;
    pub fn zsdk_texture_create_from_atitc(tex: *mut zsdk_texture_t, file_bytes: *const c_void, file_size_bytes: u32, filter: zsdk_tex_filter_t) -> zsdk_result_t;
    pub fn zsdk_texture_update_sub_image(tex: *mut zsdk_texture_t, x: u16, y: u16, width: u16, height: u16, pixels: *const c_void) -> zsdk_result_t;
    pub fn zsdk_texture_set_wrap(tex: *mut zsdk_texture_t, wrap_s: zsdk_tex_wrap_t, wrap_t: zsdk_tex_wrap_t) -> zsdk_result_t;
    pub fn zsdk_texture_bind(tex: *const zsdk_texture_t, unit: u8);
    pub fn zsdk_texture_release(tex: *mut zsdk_texture_t);
    pub fn zsdk_texture_is_valid(tex: *const zsdk_texture_t) -> c_int;
    pub fn zsdk_texture_get_width(tex: *const zsdk_texture_t) -> u16;
    pub fn zsdk_texture_get_height(tex: *const zsdk_texture_t) -> u16;
    pub fn zsdk_texture_get_gl_id(tex: *const zsdk_texture_t) -> u32;

    pub fn zsdk_mesh_create() -> *mut zsdk_mesh_buffer_t;
    pub fn zsdk_mesh_destroy(buf: *mut zsdk_mesh_buffer_t);
    pub fn zsdk_mesh_init(buf: *mut zsdk_mesh_buffer_t, is_index_buffer: c_int, is_dynamic: c_int, size_bytes: u32, data: *const c_void) -> zsdk_result_t;
    pub fn zsdk_mesh_update(buf: *mut zsdk_mesh_buffer_t, offset_bytes: u32, size_bytes: u32, data: *const c_void) -> zsdk_result_t;
    pub fn zsdk_mesh_release(buf: *mut zsdk_mesh_buffer_t);
    pub fn zsdk_mesh_is_valid(buf: *const zsdk_mesh_buffer_t) -> c_int;

    pub fn zsdk_render_reset_state_cache();
    pub fn zsdk_render_set_viewport(x: i32, y: i32, width: i32, height: i32);
    pub fn zsdk_render_set_scissor(enabled: c_int, x: i32, y: i32, width: i32, height: i32);
    pub fn zsdk_render_clear(r: u8, g: u8, b: u8, a: u8, clear_depth: c_int, clear_stencil: c_int);
    pub fn zsdk_render_set_blend_mode(mode: zsdk_blend_mode_t);
    pub fn zsdk_render_set_depth_test(enabled: c_int);
    pub fn zsdk_render_set_cull_face(enabled: c_int);
    pub fn zsdk_render_set_ortho(left: f32, right: f32, bottom: f32, top: f32, near_z: f32, far_z: f32);
    pub fn zsdk_render_set_perspective(fov_y_deg: f32, aspect: f32, near_z: f32, far_z: f32);
    pub fn zsdk_render_reset_modelview();
    pub fn zsdk_render_look_at(eye_x: f32, eye_y: f32, eye_z: f32, target_x: f32, target_y: f32, target_z: f32, up_x: f32, up_y: f32, up_z: f32);
    pub fn zsdk_render_push_matrix();
    pub fn zsdk_render_pop_matrix();
    pub fn zsdk_render_translate(x: f32, y: f32, z: f32);
    pub fn zsdk_render_rotate(angle_deg: f32, x: f32, y: f32, z: f32);
    pub fn zsdk_render_scale(x: f32, y: f32, z: f32);
    pub fn zsdk_render_set_color(r: u8, g: u8, b: u8, a: u8);
    pub fn zsdk_render_bind_raw_vertex_arrays(positions: *const c_void, pos_components: u8, normals: *const c_void, colors: *const c_void, tex_coords: *const c_void, stride_bytes: i32);
    pub fn zsdk_render_unbind_vertex_attributes();
    pub fn zsdk_render_draw_arrays(type_: zsdk_primitive_type_t, first_vertex: i32, vertex_count: i32);
    pub fn zsdk_render_draw_elements_u16(type_: zsdk_primitive_type_t, index_count: i32, indices: *const u16);
    pub fn zsdk_render_begin_2d(viewport_width: i32, viewport_height: i32);
    pub fn zsdk_render_begin_2d_virtual(virtual_width: f32, virtual_height: f32, real_width: i32, real_height: i32);
    pub fn zsdk_render_end_2d();
    pub fn zsdk_render_draw_rect_2d(x: f32, y: f32, width: f32, height: f32, r: u8, g: u8, b: u8, a: u8);
    pub fn zsdk_render_draw_text_2d(x: f32, y: f32, scale: f32, text: *const c_char, r: u8, g: u8, b: u8, a: u8);
    pub fn zsdk_render_draw_textured_quad_2d(tex: *const zsdk_texture_t, x: f32, y: f32, width: f32, height: f32, u0: f32, v0: f32, u1: f32, v1: f32, r: u8, g: u8, b: u8, a: u8);
    pub fn zsdk_render_set_fog(enabled: c_int);
    pub fn zsdk_render_configure_fog(mode: u8, r: u8, g: u8, b: u8, a: u8, start_z: f32, end_z: f32, density: f32);
    pub fn zsdk_render_set_lighting(enabled: c_int);
    pub fn zsdk_render_configure_light(light_index: u8, x: f32, y: f32, z: f32, ar: u8, ag: u8, ab: u8, aa: u8, dr: u8, dg: u8, db: u8, da: u8, sr: u8, sg: u8, sb: u8, sa: u8);
    pub fn zsdk_render_set_alpha_test(enabled: c_int, compare_func: u8, ref_val: f32);
    pub fn zsdk_render_set_stencil_test(enabled: c_int);
    pub fn zsdk_render_set_color_mask(r: c_int, g: c_int, b: c_int, a: c_int);
    pub fn zsdk_render_draw_blob_shadow(x: f32, y: f32, z: f32, radius: f32, r: u8, g: u8, b: u8, a: u8);

    pub fn zsdk_batch_create() -> *mut zsdk_sprite_batch_t;
    pub fn zsdk_batch_destroy(batch: *mut zsdk_sprite_batch_t);
    pub fn zsdk_batch_init(batch: *mut zsdk_sprite_batch_t, max_sprites: u16) -> zsdk_result_t;
    pub fn zsdk_batch_shutdown(batch: *mut zsdk_sprite_batch_t);
    pub fn zsdk_batch_begin(batch: *mut zsdk_sprite_batch_t, viewport_w: i32, viewport_h: i32);
    pub fn zsdk_batch_begin_virtual(batch: *mut zsdk_sprite_batch_t, vw: f32, vh: f32, rw: i32, rh: i32);
    pub fn zsdk_batch_end(batch: *mut zsdk_sprite_batch_t);
    pub fn zsdk_batch_flush(batch: *mut zsdk_sprite_batch_t);
    pub fn zsdk_batch_draw_rect(batch: *mut zsdk_sprite_batch_t, x: f32, y: f32, w: f32, h: f32, r: u8, g: u8, b: u8, a: u8);
    pub fn zsdk_batch_draw_quad(batch: *mut zsdk_sprite_batch_t, tex: *const zsdk_texture_t, x: f32, y: f32, w: f32, h: f32, u0: f32, v0: f32, u1: f32, v1: f32, r: u8, g: u8, b: u8, a: u8);

    pub fn zsdk_frustum_create() -> *mut zsdk_frustum_t;
    pub fn zsdk_frustum_destroy(frustum: *mut zsdk_frustum_t);
    pub fn zsdk_frustum_extract_camera(frustum: *mut zsdk_frustum_t, ex: f32, ey: f32, ez: f32, tx: f32, ty: f32, tz: f32, ux: f32, uy: f32, uz: f32, fov_y_deg: f32, aspect: f32, near_z: f32, far_z: f32);
    pub fn zsdk_frustum_test_sphere(frustum: *const zsdk_frustum_t, cx: f32, cy: f32, cz: f32, radius: f32) -> c_int;
    pub fn zsdk_frustum_test_aabb(frustum: *const zsdk_frustum_t, min_x: f32, min_y: f32, min_z: f32, max_x: f32, max_y: f32, max_z: f32) -> c_int;
}
