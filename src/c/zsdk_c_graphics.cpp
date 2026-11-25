/**
 * @file zsdk_c_graphics.cpp
 * @brief C ABI bridge for GLContext, Texture, MeshBuffer, and Renderer.
 */

#include "zsdk/c/zsdk_c_graphics.h"
#include "zsdk_c_helpers.hpp"
#include "zsdk/graphics/gl_context.hpp"
#include "zsdk/graphics/texture.hpp"
#include "zsdk/graphics/mesh_buffer.hpp"
#include "zsdk/graphics/renderer.hpp"
#include "zsdk/graphics/sprite_batch.hpp"
#include "zsdk/graphics/frustum.hpp"

struct zsdk_gl_context_t {
    zsdk::graphics::GLContext gl;
};

struct zsdk_texture_t {
    zsdk::graphics::Texture tex;
};

struct zsdk_mesh_buffer_t {
    zsdk::graphics::MeshBuffer buf;
};

struct zsdk_sprite_batch_t {
    zsdk::graphics::SpriteBatch2D batch;
};

struct zsdk_frustum_t {
    zsdk::graphics::Frustum frustum;
};

extern "C" {

zsdk_gl_context_t* zsdk_gl_create(void) {
    return zsdk::c_abi::Alloc<zsdk_gl_context_t>(zsdk::debug::MemorySubsystem::Graphics);
}

void zsdk_gl_destroy(zsdk_gl_context_t* ctx) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Graphics, ctx);
}

zsdk_result_t zsdk_gl_init(zsdk_gl_context_t* ctx, void* platform_ctx, const zsdk_gl_config_t* cfg) {
    if (!ctx) return ZSDK_C_ERR_INVALID_ARG;
    zsdk::graphics::GLContextConfig cppCfg = { 5u, 6u, 5u, 0u, 16u, 0u, 1u };
    if (cfg) {
        cppCfg.redBits      = cfg->red_bits;
        cppCfg.greenBits    = cfg->green_bits;
        cppCfg.blueBits     = cfg->blue_bits;
        cppCfg.alphaBits    = cfg->alpha_bits;
        cppCfg.depthBits    = cfg->depth_bits;
        cppCfg.stencilBits  = cfg->stencil_bits;
        cppCfg.swapInterval = cfg->swap_interval;
    }
    return zsdk::c_abi::ToCResult(ctx->gl.Initialize(platform_ctx, cppCfg));
}

void zsdk_gl_shutdown(zsdk_gl_context_t* ctx) {
    if (ctx) ctx->gl.Shutdown();
}

zsdk_result_t zsdk_gl_make_current(zsdk_gl_context_t* ctx) {
    if (!ctx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(ctx->gl.MakeCurrent());
}

int zsdk_gl_has_extension(const zsdk_gl_context_t* ctx, const char* name) {
    if (!ctx || !name) return 0;
    return ctx->gl.HasExtension(name) ? 1 : 0;
}

zsdk_result_t zsdk_gl_swap_buffers(zsdk_gl_context_t* ctx) {
    if (!ctx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(ctx->gl.SwapBuffers());
}

zsdk_result_t zsdk_gl_set_swap_interval(zsdk_gl_context_t* ctx, int32_t interval) {
    if (!ctx) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(ctx->gl.SetSwapInterval(interval));
}

int32_t zsdk_gl_get_width(const zsdk_gl_context_t* ctx) {
    return ctx ? ctx->gl.GetWidth() : 0;
}

int32_t zsdk_gl_get_height(const zsdk_gl_context_t* ctx) {
    return ctx ? ctx->gl.GetHeight() : 0;
}

zsdk_texture_t* zsdk_texture_create(void) {
    return zsdk::c_abi::Alloc<zsdk_texture_t>(zsdk::debug::MemorySubsystem::Graphics);
}

void zsdk_texture_destroy(zsdk_texture_t* tex) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Graphics, tex);
}

zsdk_result_t zsdk_texture_init_2d(zsdk_texture_t* tex, uint16_t width, uint16_t height,
                                   zsdk_tex_format_t fmt, const void* pixels, zsdk_tex_filter_t filter) {
    if (!tex) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(tex->tex.Create(
        width, height,
        static_cast<zsdk::graphics::TextureFormat>(fmt),
        pixels,
        static_cast<zsdk::graphics::TextureFilter>(filter)));
}

zsdk_result_t zsdk_texture_create_from_atitc(zsdk_texture_t* tex, const void* file_bytes,
                                             uint32_t file_size_bytes, zsdk_tex_filter_t filter) {
    if (!tex) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(tex->tex.CreateFromAtitc(
        file_bytes, file_size_bytes,
        static_cast<zsdk::graphics::TextureFilter>(filter)));
}

zsdk_result_t zsdk_texture_update_sub_image(zsdk_texture_t* tex, uint16_t x, uint16_t y,
                                            uint16_t width, uint16_t height, const void* pixels) {
    if (!tex) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(tex->tex.UpdateSubImage(x, y, width, height, pixels));
}

zsdk_result_t zsdk_texture_set_wrap(zsdk_texture_t* tex, zsdk_tex_wrap_t wrap_s, zsdk_tex_wrap_t wrap_t) {
    if (!tex) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(tex->tex.SetWrap(
        static_cast<zsdk::graphics::TextureWrap>(wrap_s),
        static_cast<zsdk::graphics::TextureWrap>(wrap_t)));
}

void zsdk_texture_bind(const zsdk_texture_t* tex, uint8_t unit) {
    if (tex) tex->tex.Bind(unit);
}

void zsdk_texture_release(zsdk_texture_t* tex) {
    if (tex) tex->tex.Destroy();
}

int zsdk_texture_is_valid(const zsdk_texture_t* tex) {
    return (tex && tex->tex.IsValid()) ? 1 : 0;
}

uint16_t zsdk_texture_get_width(const zsdk_texture_t* tex) {
    return tex ? tex->tex.GetWidth() : 0u;
}

uint16_t zsdk_texture_get_height(const zsdk_texture_t* tex) {
    return tex ? tex->tex.GetHeight() : 0u;
}

uint32_t zsdk_texture_get_gl_id(const zsdk_texture_t* tex) {
    return tex ? tex->tex.GetNativeId() : 0u;
}

zsdk_mesh_buffer_t* zsdk_mesh_create(void) {
    return zsdk::c_abi::Alloc<zsdk_mesh_buffer_t>(zsdk::debug::MemorySubsystem::Graphics);
}

void zsdk_mesh_destroy(zsdk_mesh_buffer_t* buf) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Graphics, buf);
}

zsdk_result_t zsdk_mesh_init(zsdk_mesh_buffer_t* buf, int is_index_buffer, int is_dynamic,
                             uint32_t size_bytes, const void* data) {
    if (!buf) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(buf->buf.Create(
        is_index_buffer ? zsdk::graphics::BufferType::Index : zsdk::graphics::BufferType::Vertex,
        is_dynamic ? zsdk::graphics::BufferUsage::Dynamic : zsdk::graphics::BufferUsage::Static,
        size_bytes, data));
}

zsdk_result_t zsdk_mesh_update(zsdk_mesh_buffer_t* buf, uint32_t offset_bytes,
                               uint32_t size_bytes, const void* data) {
    if (!buf) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(buf->buf.UpdateData(offset_bytes, size_bytes, data));
}

void zsdk_mesh_release(zsdk_mesh_buffer_t* buf) {
    if (buf) buf->buf.Destroy();
}

int zsdk_mesh_is_valid(const zsdk_mesh_buffer_t* buf) {
    return (buf && buf->buf.IsValid()) ? 1 : 0;
}

void zsdk_render_reset_state_cache(void) {
    zsdk::graphics::Renderer::ResetStateCache();
}

void zsdk_render_set_viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    zsdk::graphics::Renderer::SetViewport(x, y, width, height);
}

void zsdk_render_set_scissor(int enabled, int32_t x, int32_t y, int32_t width, int32_t height) {
    zsdk::graphics::Renderer::SetScissor(enabled != 0, x, y, width, height);
}

void zsdk_render_clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int clear_depth, int clear_stencil) {
    zsdk::graphics::Renderer::Clear(zsdk::Color::Rgba(r, g, b, a), clear_depth != 0, clear_stencil != 0);
}

void zsdk_render_set_blend_mode(zsdk_blend_mode_t mode) {
    zsdk::graphics::Renderer::SetBlendMode(static_cast<zsdk::graphics::BlendMode>(mode));
}

void zsdk_render_set_depth_test(int enabled) {
    zsdk::graphics::Renderer::SetDepthTest(enabled != 0);
}

void zsdk_render_set_cull_face(int enabled) {
    zsdk::graphics::Renderer::SetCullFace(enabled != 0);
}

void zsdk_render_set_ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
    zsdk::graphics::Renderer::SetOrtho(left, right, bottom, top, near_z, far_z);
}

void zsdk_render_set_perspective(float fov_y_deg, float aspect, float near_z, float far_z) {
    zsdk::graphics::Renderer::SetPerspective(fov_y_deg, aspect, near_z, far_z);
}

void zsdk_render_reset_modelview(void) {
    zsdk::graphics::Renderer::ResetModelView();
}

void zsdk_render_look_at(float eye_x, float eye_y, float eye_z,
                         float target_x, float target_y, float target_z,
                         float up_x, float up_y, float up_z) {
    zsdk::graphics::Renderer::LookAt(
        zsdk::Vec3{eye_x, eye_y, eye_z},
        zsdk::Vec3{target_x, target_y, target_z},
        zsdk::Vec3{up_x, up_y, up_z});
}

void zsdk_render_push_matrix(void) {
    zsdk::graphics::Renderer::PushMatrix();
}

void zsdk_render_pop_matrix(void) {
    zsdk::graphics::Renderer::PopMatrix();
}

void zsdk_render_translate(float x, float y, float z) {
    zsdk::graphics::Renderer::Translate(x, y, z);
}

void zsdk_render_rotate(float angle_deg, float x, float y, float z) {
    zsdk::graphics::Renderer::Rotate(angle_deg, x, y, z);
}

void zsdk_render_scale(float x, float y, float z) {
    zsdk::graphics::Renderer::Scale(x, y, z);
}

void zsdk_render_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    zsdk::graphics::Renderer::SetColor(zsdk::Color::Rgba(r, g, b, a));
}

void zsdk_render_bind_raw_vertex_arrays(const void* positions, uint8_t pos_components,
                                        const void* normals, const void* colors,
                                        const void* tex_coords, int32_t stride_bytes) {
    zsdk::graphics::Renderer::BindRawVertexArrays(
        positions, pos_components, normals, colors, tex_coords, stride_bytes);
}

void zsdk_render_unbind_vertex_attributes(void) {
    zsdk::graphics::Renderer::UnbindVertexAttributes();
}

void zsdk_render_draw_arrays(zsdk_primitive_type_t type, int32_t first_vertex, int32_t vertex_count) {
    zsdk::graphics::Renderer::DrawArrays(static_cast<zsdk::graphics::PrimitiveType>(type), first_vertex, vertex_count);
}

void zsdk_render_draw_elements_u16(zsdk_primitive_type_t type, int32_t index_count, const uint16_t* indices) {
    zsdk::graphics::Renderer::DrawElementsU16(static_cast<zsdk::graphics::PrimitiveType>(type), index_count, indices);
}

void zsdk_render_begin_2d(int32_t viewport_width, int32_t viewport_height) {
    zsdk::graphics::Renderer::Begin2D(viewport_width, viewport_height);
}

void zsdk_render_begin_2d_virtual(float virtual_width, float virtual_height, int32_t real_width, int32_t real_height) {
    zsdk::graphics::Renderer::Begin2DVirtual(virtual_width, virtual_height, real_width, real_height);
}

void zsdk_render_end_2d(void) {
    zsdk::graphics::Renderer::End2D();
}

void zsdk_render_draw_rect_2d(float x, float y, float width, float height,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    zsdk::graphics::Renderer::DrawRect2D(x, y, width, height, zsdk::Color::Rgba(r, g, b, a));
}

void zsdk_render_draw_text_2d(float x, float y, float scale, const char* text,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    zsdk::graphics::Renderer::DrawText2D(x, y, scale, text, zsdk::Color::Rgba(r, g, b, a));
}

void zsdk_render_draw_textured_quad_2d(const zsdk_texture_t* tex, float x, float y, float width, float height,
                                       float u0, float v0, float u1, float v1,
                                       uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!tex) return;
    zsdk::graphics::Renderer::DrawTexturedQuad2D(tex->tex, x, y, width, height, u0, v0, u1, v1,
                                                 zsdk::Color::Rgba(r, g, b, a));
}

void zsdk_render_set_fog(int enabled) {
    zsdk::graphics::Renderer::SetFog(enabled != 0);
}

void zsdk_render_configure_fog(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                               float start_z, float end_z, float density) {
    zsdk::graphics::Renderer::ConfigureFog(
        static_cast<zsdk::graphics::FogMode>(mode),
        zsdk::Color::Rgba(r, g, b, a), start_z, end_z, density);
}

void zsdk_render_set_lighting(int enabled) {
    zsdk::graphics::Renderer::SetLighting(enabled != 0);
}

void zsdk_render_configure_light(uint8_t light_index, float x, float y, float z,
                                 uint8_t ar, uint8_t ag, uint8_t ab, uint8_t aa,
                                 uint8_t dr, uint8_t dg, uint8_t db, uint8_t da,
                                 uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa) {
    zsdk::graphics::Renderer::ConfigureLight(
        light_index,
        zsdk::Vec3{x, y, z},
        zsdk::Color::Rgba(ar, ag, ab, aa),
        zsdk::Color::Rgba(dr, dg, db, da),
        zsdk::Color::Rgba(sr, sg, sb, sa));
}

void zsdk_render_set_alpha_test(int enabled, uint8_t compare_func, float ref_val) {
    zsdk::graphics::Renderer::SetAlphaTest(
        enabled != 0, static_cast<zsdk::graphics::CompareFunc>(compare_func), ref_val);
}

void zsdk_render_set_stencil_test(int enabled) {
    zsdk::graphics::Renderer::SetStencilTest(enabled != 0);
}

void zsdk_render_set_color_mask(int r, int g, int b, int a) {
    zsdk::graphics::Renderer::SetColorMask(r != 0, g != 0, b != 0, a != 0);
}

void zsdk_render_draw_blob_shadow(float x, float y, float z, float radius,
                                  uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    zsdk::graphics::Renderer::DrawBlobShadow(
        zsdk::Vec3{x, y, z}, radius, zsdk::Color::Rgba(r, g, b, a));
}

zsdk_sprite_batch_t* zsdk_batch_create(void) {
    return zsdk::c_abi::Alloc<zsdk_sprite_batch_t>(zsdk::debug::MemorySubsystem::Graphics);
}

void zsdk_batch_destroy(zsdk_sprite_batch_t* batch) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Graphics, batch);
}

zsdk_result_t zsdk_batch_init(zsdk_sprite_batch_t* batch, uint16_t max_sprites) {
    if (!batch) return ZSDK_C_ERR_INVALID_ARG;
    return zsdk::c_abi::ToCResult(batch->batch.Initialize(max_sprites));
}

void zsdk_batch_shutdown(zsdk_sprite_batch_t* batch) {
    if (batch) batch->batch.Shutdown();
}

void zsdk_batch_begin(zsdk_sprite_batch_t* batch, int32_t viewport_w, int32_t viewport_h) {
    if (batch) batch->batch.Begin(viewport_w, viewport_h);
}

void zsdk_batch_begin_virtual(zsdk_sprite_batch_t* batch, float vw, float vh, int32_t rw, int32_t rh) {
    if (batch) batch->batch.BeginVirtual(vw, vh, rw, rh);
}

void zsdk_batch_end(zsdk_sprite_batch_t* batch) {
    if (batch) batch->batch.End();
}

void zsdk_batch_flush(zsdk_sprite_batch_t* batch) {
    if (batch) batch->batch.Flush();
}

void zsdk_batch_draw_rect(zsdk_sprite_batch_t* batch, float x, float y, float w, float h,
                          uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (batch) batch->batch.DrawRect(x, y, w, h, zsdk::Color::Rgba(r, g, b, a));
}

void zsdk_batch_draw_quad(zsdk_sprite_batch_t* batch, const zsdk_texture_t* tex,
                          float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (batch) {
        batch->batch.DrawQuad(tex ? &tex->tex : nullptr, x, y, w, h, u0, v0, u1, v1,
                              zsdk::Color::Rgba(r, g, b, a));
    }
}

zsdk_frustum_t* zsdk_frustum_create(void) {
    return zsdk::c_abi::Alloc<zsdk_frustum_t>(zsdk::debug::MemorySubsystem::Graphics);
}

void zsdk_frustum_destroy(zsdk_frustum_t* frustum) {
    zsdk::c_abi::Free(zsdk::debug::MemorySubsystem::Graphics, frustum);
}

void zsdk_frustum_extract_camera(zsdk_frustum_t* frustum,
                                 float ex, float ey, float ez,
                                 float tx, float ty, float tz,
                                 float ux, float uy, float uz,
                                 float fov_y_deg, float aspect, float near_z, float far_z) {
    if (!frustum) return;
    frustum->frustum.ExtractFromCamera(
        zsdk::Vec3{ex, ey, ez}, zsdk::Vec3{tx, ty, tz}, zsdk::Vec3{ux, uy, uz},
        fov_y_deg, aspect, near_z, far_z);
}

int zsdk_frustum_test_sphere(const zsdk_frustum_t* frustum, float cx, float cy, float cz, float radius) {
    return (frustum && frustum->frustum.TestSphere(zsdk::Vec3{cx, cy, cz}, radius)) ? 1 : 0;
}

int zsdk_frustum_test_aabb(const zsdk_frustum_t* frustum,
                           float min_x, float min_y, float min_z,
                           float max_x, float max_y, float max_z) {
    return (frustum && frustum->frustum.TestAabb(zsdk::Vec3{min_x, min_y, min_z}, zsdk::Vec3{max_x, max_y, max_z})) ? 1 : 0;
}

} /* extern "C" */
