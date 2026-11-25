use ::core::ffi::{CStr, c_void};
use ::core::ptr;
use zsdk_sys::*;
use crate::{check, Error, Result};

pub use zsdk_sys::{
    ZsdkBlendMode as BlendMode,
    ZsdkPrimitiveType as PrimitiveType,
    ZsdkTexFilter as TexFilter,
    ZsdkTexFormat as TexFormat,
    ZsdkTexWrap as TexWrap,
    zsdk_gl_config_t as GLContextConfig,
};

pub struct GLContext { raw: *mut zsdk_gl_context_t }
impl GLContext {
    pub fn new(platform_ctx: *mut c_void, cfg: Option<&GLContextConfig>) -> Result<Self> {
        let raw = unsafe { zsdk_gl_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        let ptr = cfg.map_or(ptr::null(), |c| c as *const _);
        if let Err(e) = check(unsafe { zsdk_gl_init(raw, platform_ctx, ptr) }) {
            unsafe { zsdk_gl_destroy(raw); }
            return Err(e);
        }
        Ok(Self { raw })
    }
    pub fn make_current(&mut self) -> Result<()> { check(unsafe { zsdk_gl_make_current(self.raw) }) }
    pub fn swap_buffers(&mut self) -> Result<()> { check(unsafe { zsdk_gl_swap_buffers(self.raw) }) }
    pub fn has_extension(&self, name: &CStr) -> bool { unsafe { zsdk_gl_has_extension(self.raw, name.as_ptr()) != 0 } }
    pub fn set_swap_interval(&mut self, interval: i32) -> Result<()> { check(unsafe { zsdk_gl_set_swap_interval(self.raw, interval) }) }
    pub fn width(&self) -> i32 { unsafe { zsdk_gl_get_width(self.raw) } }
    pub fn height(&self) -> i32 { unsafe { zsdk_gl_get_height(self.raw) } }
    pub fn render_width(&self) -> i32 { unsafe { zsdk_gl_get_render_width(self.raw) } }
    pub fn render_height(&self) -> i32 { unsafe { zsdk_gl_get_render_height(self.raw) } }
    pub fn set_render_resolution(&mut self, w: i32, h: i32) -> Result<()> { check(unsafe { zsdk_gl_set_render_resolution(self.raw, w, h) }) }
    pub fn power_level(&self) -> Result<i32> {
        let mut lvl = 0i32;
        check(unsafe { zsdk_gl_get_power_level(self.raw, &mut lvl) })?;
        Ok(lvl)
    }
    pub fn enable_surface_scale(&mut self, enabled: bool) -> Result<()> { check(unsafe { zsdk_gl_enable_surface_scale(self.raw, enabled as i32) }) }
    pub fn set_surface_rotate(&mut self, rot90: bool, h_mirror: bool, v_mirror: bool, enabled: bool) -> Result<()> {
        check(unsafe { zsdk_gl_set_surface_rotate(self.raw, rot90 as i32, h_mirror as i32, v_mirror as i32, enabled as i32) })
    }
}
impl Drop for GLContext {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_gl_shutdown(self.raw); zsdk_gl_destroy(self.raw); } }
    }
}

pub struct Texture { raw: *mut zsdk_texture_t }
impl Texture {
    pub fn new() -> Result<Self> {
        let raw = unsafe { zsdk_texture_create() };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }
    pub fn init_2d(&mut self, w: u16, h: u16, fmt: TexFormat, pixels: *const c_void, filter: TexFilter) -> Result<()> {
        check(unsafe { zsdk_texture_init_2d(self.raw, w, h, fmt, pixels, filter) })
    }
    pub fn create_from_atitc(&mut self, bytes: &[u8], filter: TexFilter) -> Result<()> {
        check(unsafe { zsdk_texture_create_from_atitc(self.raw, bytes.as_ptr() as *const _, bytes.len() as u32, filter) })
    }
    pub fn update_sub_image(&mut self, x: u16, y: u16, w: u16, h: u16, pixels: *const c_void) -> Result<()> {
        check(unsafe { zsdk_texture_update_sub_image(self.raw, x, y, w, h, pixels) })
    }
    pub fn set_wrap(&mut self, s: TexWrap, t: TexWrap) -> Result<()> { check(unsafe { zsdk_texture_set_wrap(self.raw, s, t) }) }
    pub fn bind(&self, unit: u8) { unsafe { zsdk_texture_bind(self.raw, unit); } }
    pub fn is_valid(&self) -> bool { unsafe { zsdk_texture_is_valid(self.raw) != 0 } }
    pub fn width(&self) -> u16 { unsafe { zsdk_texture_get_width(self.raw) } }
    pub fn height(&self) -> u16 { unsafe { zsdk_texture_get_height(self.raw) } }
    pub fn gl_id(&self) -> u32 { unsafe { zsdk_texture_get_gl_id(self.raw) } }
    pub fn vram_bytes(&self) -> u32 { unsafe { zsdk_texture_get_vram_bytes(self.raw) } }
    pub fn as_raw(&self) -> *const zsdk_texture_t { self.raw }
}
impl Drop for Texture {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_texture_release(self.raw); zsdk_texture_destroy(self.raw); } }
    }
}

pub struct MeshBuffer { raw: *mut zsdk_mesh_buffer_t }
impl MeshBuffer {
    pub fn new() -> Result<Self> {
        let raw = unsafe { zsdk_mesh_create() };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }
    pub fn init(&mut self, is_index_buffer: bool, is_dynamic: bool, data: &[u8]) -> Result<()> {
        check(unsafe { zsdk_mesh_init(self.raw, is_index_buffer as i32, is_dynamic as i32, data.len() as u32, data.as_ptr() as *const _) })
    }
    pub fn update(&mut self, offset_bytes: u32, data: &[u8]) -> Result<()> {
        check(unsafe { zsdk_mesh_update(self.raw, offset_bytes, data.len() as u32, data.as_ptr() as *const _) })
    }
    pub fn release(&mut self) { unsafe { zsdk_mesh_release(self.raw); } }
    pub fn is_valid(&self) -> bool { unsafe { zsdk_mesh_is_valid(self.raw) != 0 } }
    pub fn size_bytes(&self) -> u32 { unsafe { zsdk_mesh_get_size_bytes(self.raw) } }
    pub fn as_raw(&self) -> *const zsdk_mesh_buffer_t { self.raw }
}
impl Drop for MeshBuffer {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_mesh_release(self.raw); zsdk_mesh_destroy(self.raw); } }
    }
}

pub mod renderer {
    use super::*;
    #[inline] pub fn reset_state_cache() { unsafe { zsdk_render_reset_state_cache(); } }
    #[inline] pub fn set_viewport(x: i32, y: i32, w: i32, h: i32) { unsafe { zsdk_render_set_viewport(x, y, w, h); } }
    #[inline] pub fn set_scissor(enabled: bool, x: i32, y: i32, w: i32, h: i32) { unsafe { zsdk_render_set_scissor(enabled as i32, x, y, w, h); } }
    #[inline] pub fn clear(r: u8, g: u8, b: u8, a: u8, depth: bool, stencil: bool) { unsafe { zsdk_render_clear(r, g, b, a, depth as i32, stencil as i32); } }
    #[inline] pub fn set_blend_mode(m: BlendMode) { unsafe { zsdk_render_set_blend_mode(m); } }
    #[inline] pub fn set_depth_test(e: bool) { unsafe { zsdk_render_set_depth_test(e as i32); } }
    #[inline] pub fn set_cull_face(e: bool) { unsafe { zsdk_render_set_cull_face(e as i32); } }
    #[inline] pub fn set_ortho(left: f32, right: f32, bottom: f32, top: f32, near_z: f32, far_z: f32) { unsafe { zsdk_render_set_ortho(left, right, bottom, top, near_z, far_z); } }
    #[inline] pub fn set_perspective(fov_y: f32, aspect: f32, near_z: f32, far_z: f32) { unsafe { zsdk_render_set_perspective(fov_y, aspect, near_z, far_z); } }
    #[inline] pub fn reset_modelview() { unsafe { zsdk_render_reset_modelview(); } }
    #[inline] pub fn look_at(eye: [f32; 3], target: [f32; 3], up: [f32; 3]) { unsafe { zsdk_render_look_at(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]); } }
    #[inline] pub fn push_matrix() { unsafe { zsdk_render_push_matrix(); } }
    #[inline] pub fn pop_matrix() { unsafe { zsdk_render_pop_matrix(); } }
    #[inline] pub fn translate(x: f32, y: f32, z: f32) { unsafe { zsdk_render_translate(x, y, z); } }
    #[inline] pub fn rotate(deg: f32, x: f32, y: f32, z: f32) { unsafe { zsdk_render_rotate(deg, x, y, z); } }
    #[inline] pub fn scale(x: f32, y: f32, z: f32) { unsafe { zsdk_render_scale(x, y, z); } }
    #[inline] pub fn set_color(r: u8, g: u8, b: u8, a: u8) { unsafe { zsdk_render_set_color(r, g, b, a); } }
    #[inline] pub unsafe fn bind_raw_vertex_arrays(positions: *const c_void, pos_components: u8, normals: *const c_void, colors: *const c_void, tex_coords: *const c_void, stride_bytes: i32) {
        zsdk_render_bind_raw_vertex_arrays(positions, pos_components, normals, colors, tex_coords, stride_bytes);
    }
    #[inline] pub fn unbind_vertex_attributes() { unsafe { zsdk_render_unbind_vertex_attributes(); } }
    #[inline] pub fn draw_arrays(prim: PrimitiveType, first_vertex: i32, vertex_count: i32) { unsafe { zsdk_render_draw_arrays(prim, first_vertex, vertex_count); } }
    #[inline] pub fn draw_elements_u16(prim: PrimitiveType, indices: &[u16]) { unsafe { zsdk_render_draw_elements_u16(prim, indices.len() as i32, indices.as_ptr()); } }
    #[inline] pub fn begin_2d(viewport_w: i32, viewport_h: i32) { unsafe { zsdk_render_begin_2d(viewport_w, viewport_h); } }
    #[inline] pub fn begin_2d_virtual(vw: f32, vh: f32, rw: i32, rh: i32) { unsafe { zsdk_render_begin_2d_virtual(vw, vh, rw, rh); } }
    #[inline] pub fn end_2d() { unsafe { zsdk_render_end_2d(); } }
    #[inline] pub fn draw_rect_2d(x: f32, y: f32, w: f32, h: f32, rgba: [u8; 4]) { unsafe { zsdk_render_draw_rect_2d(x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]); } }
    #[inline] pub fn draw_text_2d(x: f32, y: f32, scale: f32, text: &CStr, rgba: [u8; 4]) { unsafe { zsdk_render_draw_text_2d(x, y, scale, text.as_ptr(), rgba[0], rgba[1], rgba[2], rgba[3]); } }
    #[inline] pub fn draw_textured_quad_2d(tex: &Texture, x: f32, y: f32, w: f32, h: f32, uv: [f32; 4], rgba: [u8; 4]) {
        unsafe { zsdk_render_draw_textured_quad_2d(tex.as_raw(), x, y, w, h, uv[0], uv[1], uv[2], uv[3], rgba[0], rgba[1], rgba[2], rgba[3]); }
    }
    #[inline] pub fn set_fog(enabled: bool) { unsafe { zsdk_render_set_fog(enabled as i32); } }
    #[inline] pub fn configure_fog(mode: u8, rgba: [u8; 4], start_z: f32, end_z: f32, density: f32) {
        unsafe { zsdk_render_configure_fog(mode, rgba[0], rgba[1], rgba[2], rgba[3], start_z, end_z, density); }
    }
    #[inline] pub fn set_lighting(enabled: bool) { unsafe { zsdk_render_set_lighting(enabled as i32); } }
    #[inline] pub fn configure_light(index: u8, pos: [f32; 3], ambient: [u8; 4], diffuse: [u8; 4], specular: [u8; 4]) {
        unsafe {
            zsdk_render_configure_light(
                index, pos[0], pos[1], pos[2],
                ambient[0], ambient[1], ambient[2], ambient[3],
                diffuse[0], diffuse[1], diffuse[2], diffuse[3],
                specular[0], specular[1], specular[2], specular[3],
            );
        }
    }
    #[inline] pub fn set_alpha_test(enabled: bool, compare_func: u8, ref_val: f32) { unsafe { zsdk_render_set_alpha_test(enabled as i32, compare_func, ref_val); } }
    #[inline] pub fn set_stencil_test(enabled: bool) { unsafe { zsdk_render_set_stencil_test(enabled as i32); } }
    #[inline] pub fn set_color_mask(r: bool, g: bool, b: bool, a: bool) { unsafe { zsdk_render_set_color_mask(r as i32, g as i32, b as i32, a as i32); } }
    #[inline] pub fn draw_blob_shadow(x: f32, y: f32, z: f32, radius: f32, rgba: [u8; 4]) {
        unsafe { zsdk_render_draw_blob_shadow(x, y, z, radius, rgba[0], rgba[1], rgba[2], rgba[3]); }
    }
    #[inline] pub fn set_front_face(clockwise: bool) { unsafe { zsdk_render_set_front_face(clockwise as i32); } }
    #[inline] pub fn set_depth_func(func: u8) { unsafe { zsdk_render_set_depth_func(func); } }
    #[inline] pub fn set_depth_mask(write_enabled: bool) { unsafe { zsdk_render_set_depth_mask(write_enabled as i32); } }
    #[inline] pub fn set_dither(enabled: bool) { unsafe { zsdk_render_set_dither(enabled as i32); } }
    #[inline] pub fn set_shade_model(model: u8) { unsafe { zsdk_render_set_shade_model(model); } }
    #[inline] pub fn set_line_width(width: f32) { unsafe { zsdk_render_set_line_width(width); } }
    #[inline] pub fn set_point_size(size: f32) { unsafe { zsdk_render_set_point_size(size); } }
    #[inline] pub fn set_light_enabled(index: u8, enabled: bool) { unsafe { zsdk_render_set_light_enabled(index, enabled as i32); } }
    #[inline] pub fn set_global_ambient(rgba: [u8; 4]) { unsafe { zsdk_render_set_global_ambient(rgba[0], rgba[1], rgba[2], rgba[3]); } }
    #[inline] pub fn set_material(ambient: [u8; 4], diffuse: [u8; 4], specular: [u8; 4], shininess: f32) {
        unsafe {
            zsdk_render_set_material(
                ambient[0], ambient[1], ambient[2], ambient[3],
                diffuse[0], diffuse[1], diffuse[2], diffuse[3],
                specular[0], specular[1], specular[2], specular[3],
                shininess,
            );
        }
    }
    #[inline] pub fn set_polygon_offset(enabled: bool, factor: f32, units: f32) { unsafe { zsdk_render_set_polygon_offset(enabled as i32, factor, units); } }
    #[inline] pub fn set_texture_2d(enabled: bool) { unsafe { zsdk_render_set_texture_2d(enabled as i32); } }
    #[inline] pub fn set_active_texture_unit(unit: u8) { unsafe { zsdk_render_set_active_texture_unit(unit); } }
    #[inline] pub fn set_normalize_normals(enabled: bool) { unsafe { zsdk_render_set_normalize_normals(enabled as i32); } }
    #[inline] pub fn load_matrix(mat16: &[f32; 16]) { unsafe { zsdk_render_load_matrix(mat16.as_ptr()); } }
    #[inline] pub fn mult_matrix(mat16: &[f32; 16]) { unsafe { zsdk_render_mult_matrix(mat16.as_ptr()); } }
    #[inline] pub fn set_frustum(left: f32, right: f32, bottom: f32, top: f32, near_z: f32, far_z: f32) {
        unsafe { zsdk_render_set_frustum(left, right, bottom, top, near_z, far_z); }
    }
}

pub struct SpriteBatch { raw: *mut zsdk_sprite_batch_t }
impl SpriteBatch {
    pub fn new(max_sprites: u16) -> Result<Self> {
        let raw = unsafe { zsdk_batch_create() };
        if raw.is_null() { return Err(Error::ErrOutOfMemory); }
        if let Err(e) = check(unsafe { zsdk_batch_init(raw, max_sprites) }) {
            unsafe { zsdk_batch_destroy(raw); }
            return Err(e);
        }
        Ok(Self { raw })
    }
    #[inline] pub fn begin(&mut self, viewport_w: i32, viewport_h: i32) { unsafe { zsdk_batch_begin(self.raw, viewport_w, viewport_h); } }
    #[inline] pub fn begin_virtual(&mut self, vw: f32, vh: f32, rw: i32, rh: i32) { unsafe { zsdk_batch_begin_virtual(self.raw, vw, vh, rw, rh); } }
    #[inline] pub fn end(&mut self) { unsafe { zsdk_batch_end(self.raw); } }
    #[inline] pub fn flush(&mut self) { unsafe { zsdk_batch_flush(self.raw); } }
    #[inline] pub fn draw_rect(&mut self, x: f32, y: f32, w: f32, h: f32, rgba: [u8; 4]) {
        unsafe { zsdk_batch_draw_rect(self.raw, x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]); }
    }
    #[inline] pub fn draw_quad(&mut self, tex: &Texture, x: f32, y: f32, w: f32, h: f32, uv: [f32; 4], rgba: [u8; 4]) {
        unsafe { zsdk_batch_draw_quad(self.raw, tex.as_raw(), x, y, w, h, uv[0], uv[1], uv[2], uv[3], rgba[0], rgba[1], rgba[2], rgba[3]); }
    }
    #[inline] pub fn draw_rotated(&mut self, tex: Option<&Texture>, cx: f32, cy: f32, w: f32, h: f32, angle_rad: f32, uv: [f32; 4], rgba: [u8; 4]) {
        let tp = tex.map_or(ptr::null(), |t| t.as_raw());
        unsafe { zsdk_batch_draw_rotated(self.raw, tp, cx, cy, w, h, angle_rad, uv[0], uv[1], uv[2], uv[3], rgba[0], rgba[1], rgba[2], rgba[3]); }
    }
}
impl Drop for SpriteBatch {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_batch_shutdown(self.raw); zsdk_batch_destroy(self.raw); } }
    }
}

pub struct Frustum { raw: *mut zsdk_frustum_t }
impl Frustum {
    pub fn new() -> Result<Self> {
        let raw = unsafe { zsdk_frustum_create() };
        if raw.is_null() { Err(Error::ErrOutOfMemory) } else { Ok(Self { raw }) }
    }
    #[inline]
    pub fn extract_camera(&mut self, eye: [f32; 3], target: [f32; 3], up: [f32; 3], fov_y_deg: f32, aspect: f32, near_z: f32, far_z: f32) {
        unsafe {
            zsdk_frustum_extract_camera(
                self.raw,
                eye[0], eye[1], eye[2],
                target[0], target[1], target[2],
                up[0], up[1], up[2],
                fov_y_deg, aspect, near_z, far_z,
            );
        }
    }
    #[inline]
    pub fn extract_view_proj(&mut self, vp16: &[f32; 16]) {
        unsafe { zsdk_frustum_extract_view_proj(self.raw, vp16.as_ptr()); }
    }
    #[inline] pub fn test_sphere(&self, center: [f32; 3], radius: f32) -> bool { unsafe { zsdk_frustum_test_sphere(self.raw, center[0], center[1], center[2], radius) != 0 } }
    #[inline] pub fn test_aabb(&self, min: [f32; 3], max: [f32; 3]) -> bool {
        unsafe { zsdk_frustum_test_aabb(self.raw, min[0], min[1], min[2], max[0], max[1], max[2]) != 0 }
    }
}
impl Drop for Frustum {
    fn drop(&mut self) {
        if !self.raw.is_null() { unsafe { zsdk_frustum_destroy(self.raw); } }
    }
}
