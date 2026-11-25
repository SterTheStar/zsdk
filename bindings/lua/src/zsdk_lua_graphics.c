/**
 * @file zsdk_lua_graphics.c
 * @brief Complete Lua bindings for zsdk_c_graphics (GLContext, Texture, MeshBuffer, SpriteBatch2D, Frustum, Renderer).
 */

#include "zsdk_lua.h"
#include "zsdk_lua_internal.h"
#include "zsdk/c/zsdk_c_graphics.h"

#define ZSDK_LUA_GL_MT      "zsdk.GLContext"
#define ZSDK_LUA_TEX_MT     "zsdk.Texture"
#define ZSDK_LUA_MESH_MT    "zsdk.MeshBuffer"
#define ZSDK_LUA_BATCH_MT   "zsdk.SpriteBatch2D"
#define ZSDK_LUA_FRUSTUM_MT "zsdk.Frustum"

typedef struct { zsdk_gl_context_t*  ptr; } zsdk_lua_gl_ud;
typedef struct { zsdk_texture_t*     ptr; } zsdk_lua_tex_ud;
typedef struct { zsdk_mesh_buffer_t* ptr; } zsdk_lua_mesh_ud;
typedef struct { zsdk_sprite_batch_t*ptr; } zsdk_lua_batch_ud;
typedef struct { zsdk_frustum_t*     ptr; } zsdk_lua_frustum_ud;

static zsdk_gl_context_t*  check_gl(lua_State* L, int i)      { return ((zsdk_lua_gl_ud*)luaL_checkudata(L, i, ZSDK_LUA_GL_MT))->ptr; }
static zsdk_texture_t*     check_tex(lua_State* L, int i)     { return ((zsdk_lua_tex_ud*)luaL_checkudata(L, i, ZSDK_LUA_TEX_MT))->ptr; }
static zsdk_mesh_buffer_t* check_mesh(lua_State* L, int i)    { return ((zsdk_lua_mesh_ud*)luaL_checkudata(L, i, ZSDK_LUA_MESH_MT))->ptr; }
static zsdk_sprite_batch_t*check_batch(lua_State* L, int i)   { return ((zsdk_lua_batch_ud*)luaL_checkudata(L, i, ZSDK_LUA_BATCH_MT))->ptr; }
static zsdk_frustum_t*     check_frustum(lua_State* L, int i) { return ((zsdk_lua_frustum_ud*)luaL_checkudata(L, i, ZSDK_LUA_FRUSTUM_MT))->ptr; }

/* GLContext */
static int l_gl_gc(lua_State* L) {
    zsdk_lua_gl_ud* ud = (zsdk_lua_gl_ud*)luaL_checkudata(L, 1, ZSDK_LUA_GL_MT);
    if (ud->ptr) { zsdk_gl_shutdown(ud->ptr); zsdk_gl_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_gl_new(lua_State* L) {
    void* ctx = lua_touserdata(L, 1);
    zsdk_gl_context_t* gl = zsdk_gl_create();
    if (!gl) return luaL_error(L, "zsdk.graphics.new_gl: out of memory");
    if (ctx && zsdk_gl_init(gl, ctx, NULL) != ZSDK_C_OK) { zsdk_gl_destroy(gl); return luaL_error(L, "init failed"); }
    zsdk_lua_gl_ud* ud = (zsdk_lua_gl_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = gl;
    luaL_getmetatable(L, ZSDK_LUA_GL_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_gl_make_current(lua_State* L)      { lua_pushinteger(L, (lua_Integer)zsdk_gl_make_current(check_gl(L, 1))); return 1; }
static int l_gl_has_extension(lua_State* L)    { lua_pushboolean(L, zsdk_gl_has_extension(check_gl(L, 1), luaL_checkstring(L, 2))); return 1; }
static int l_gl_swap_buffers(lua_State* L)      { lua_pushinteger(L, (lua_Integer)zsdk_gl_swap_buffers(check_gl(L, 1))); return 1; }
static int l_gl_set_swap_interval(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_gl_set_swap_interval(check_gl(L, 1), (int32_t)luaL_checkinteger(L, 2))); return 1; }
static int l_gl_size(lua_State* L)              { zsdk_gl_context_t* g = check_gl(L, 1); lua_pushinteger(L, zsdk_gl_get_width(g)); lua_pushinteger(L, zsdk_gl_get_height(g)); return 2; }

/* Texture */
static int l_tex_gc(lua_State* L) {
    zsdk_lua_tex_ud* ud = (zsdk_lua_tex_ud*)luaL_checkudata(L, 1, ZSDK_LUA_TEX_MT);
    if (ud->ptr) { zsdk_texture_release(ud->ptr); zsdk_texture_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_tex_new(lua_State* L) {
    zsdk_texture_t* tex = zsdk_texture_create();
    if (!tex) return luaL_error(L, "zsdk.graphics.new_texture: out of memory");
    zsdk_lua_tex_ud* ud = (zsdk_lua_tex_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = tex;
    luaL_getmetatable(L, ZSDK_LUA_TEX_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_tex_init_2d(lua_State* L) {
    size_t len = 0;
    uint16_t w = (uint16_t)luaL_checkinteger(L, 2);
    uint16_t h = (uint16_t)luaL_checkinteger(L, 3);
    zsdk_tex_format_t fmt = (zsdk_tex_format_t)luaL_checkinteger(L, 4);
    const char* px = lua_isnil(L, 5) ? NULL : luaL_checklstring(L, 5, &len);
    zsdk_tex_filter_t flt = (zsdk_tex_filter_t)luaL_optinteger(L, 6, ZSDK_FILTER_LINEAR);
    lua_pushinteger(L, (lua_Integer)zsdk_texture_init_2d(check_tex(L, 1), w, h, fmt, px, flt));
    return 1;
}
static int l_tex_from_atitc(lua_State* L) {
    size_t len = 0;
    const char* bytes = luaL_checklstring(L, 2, &len);
    zsdk_tex_filter_t flt = (zsdk_tex_filter_t)luaL_optinteger(L, 3, ZSDK_FILTER_LINEAR);
    lua_pushinteger(L, (lua_Integer)zsdk_texture_create_from_atitc(check_tex(L, 1), bytes, (uint32_t)len, flt));
    return 1;
}
static int l_tex_update_sub(lua_State* L) {
    size_t len = 0;
    const char* px = luaL_checklstring(L, 6, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_texture_update_sub_image(
        check_tex(L, 1), (uint16_t)luaL_checkinteger(L, 2), (uint16_t)luaL_checkinteger(L, 3),
        (uint16_t)luaL_checkinteger(L, 4), (uint16_t)luaL_checkinteger(L, 5), px));
    return 1;
}
static int l_tex_bind(lua_State* L)     { zsdk_texture_bind(check_tex(L, 1), (uint8_t)luaL_optinteger(L, 2, 0)); return 0; }
static int l_tex_set_wrap(lua_State* L) { lua_pushinteger(L, (lua_Integer)zsdk_texture_set_wrap(check_tex(L, 1), (zsdk_tex_wrap_t)luaL_checkinteger(L, 2), (zsdk_tex_wrap_t)luaL_checkinteger(L, 3))); return 1; }
static int l_tex_is_valid(lua_State* L) { lua_pushboolean(L, zsdk_texture_is_valid(check_tex(L, 1))); return 1; }
static int l_tex_gl_id(lua_State* L)    { lua_pushinteger(L, (lua_Integer)zsdk_texture_get_gl_id(check_tex(L, 1))); return 1; }
static int l_tex_size(lua_State* L)     { zsdk_texture_t* t = check_tex(L, 1); lua_pushinteger(L, zsdk_texture_get_width(t)); lua_pushinteger(L, zsdk_texture_get_height(t)); return 2; }

/* MeshBuffer */
static int l_mesh_gc(lua_State* L) {
    zsdk_lua_mesh_ud* ud = (zsdk_lua_mesh_ud*)luaL_checkudata(L, 1, ZSDK_LUA_MESH_MT);
    if (ud->ptr) { zsdk_mesh_release(ud->ptr); zsdk_mesh_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_mesh_new(lua_State* L) {
    zsdk_mesh_buffer_t* m = zsdk_mesh_create();
    if (!m) return luaL_error(L, "zsdk.graphics.new_mesh: out of memory");
    zsdk_lua_mesh_ud* ud = (zsdk_lua_mesh_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = m;
    luaL_getmetatable(L, ZSDK_LUA_MESH_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_mesh_init(lua_State* L) {
    size_t len = 0;
    int is_idx = lua_toboolean(L, 2);
    int is_dyn = lua_toboolean(L, 3);
    const char* data = lua_isnil(L, 4) ? NULL : luaL_checklstring(L, 4, &len);
    uint32_t sz = (uint32_t)luaL_optinteger(L, 5, (lua_Integer)len);
    lua_pushinteger(L, (lua_Integer)zsdk_mesh_init(check_mesh(L, 1), is_idx, is_dyn, sz, data));
    return 1;
}
static int l_mesh_update(lua_State* L) {
    size_t len = 0;
    uint32_t off = (uint32_t)luaL_checkinteger(L, 2);
    const char* data = luaL_checklstring(L, 3, &len);
    lua_pushinteger(L, (lua_Integer)zsdk_mesh_update(check_mesh(L, 1), off, (uint32_t)len, data));
    return 1;
}
static int l_mesh_release(lua_State* L)  { zsdk_mesh_release(check_mesh(L, 1)); return 0; }
static int l_mesh_is_valid(lua_State* L) { lua_pushboolean(L, zsdk_mesh_is_valid(check_mesh(L, 1))); return 1; }

/* SpriteBatch2D */
static int l_batch_gc(lua_State* L) {
    zsdk_lua_batch_ud* ud = (zsdk_lua_batch_ud*)luaL_checkudata(L, 1, ZSDK_LUA_BATCH_MT);
    if (ud->ptr) { zsdk_batch_shutdown(ud->ptr); zsdk_batch_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_batch_new(lua_State* L) {
    uint16_t max_sprites = (uint16_t)luaL_optinteger(L, 1, 128);
    zsdk_sprite_batch_t* b = zsdk_batch_create();
    if (!b || zsdk_batch_init(b, max_sprites) != ZSDK_C_OK) { zsdk_batch_destroy(b); return luaL_error(L, "init failed"); }
    zsdk_lua_batch_ud* ud = (zsdk_lua_batch_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = b;
    luaL_getmetatable(L, ZSDK_LUA_BATCH_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_batch_begin(lua_State* L)         { zsdk_batch_begin(check_batch(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3)); return 0; }
static int l_batch_begin_virtual(lua_State* L) { zsdk_batch_begin_virtual(check_batch(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (int32_t)luaL_checkinteger(L, 4), (int32_t)luaL_checkinteger(L, 5)); return 0; }
static int l_batch_end(lua_State* L)           { zsdk_batch_end(check_batch(L, 1)); return 0; }
static int l_batch_flush(lua_State* L)         { zsdk_batch_flush(check_batch(L, 1)); return 0; }
static int l_batch_draw_rect(lua_State* L) {
    zsdk_batch_draw_rect(check_batch(L, 1),
        (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3),
        (float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5),
        (uint8_t)luaL_optinteger(L, 6, 255), (uint8_t)luaL_optinteger(L, 7, 255),
        (uint8_t)luaL_optinteger(L, 8, 255), (uint8_t)luaL_optinteger(L, 9, 255));
    return 0;
}
static int l_batch_draw_quad(lua_State* L) {
    zsdk_texture_t* tex = lua_isnil(L, 2) ? NULL : check_tex(L, 2);
    zsdk_batch_draw_quad(check_batch(L, 1), tex,
        (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4),
        (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6),
        (float)luaL_optnumber(L, 7, 0.0), (float)luaL_optnumber(L, 8, 0.0),
        (float)luaL_optnumber(L, 9, 1.0), (float)luaL_optnumber(L, 10, 1.0),
        (uint8_t)luaL_optinteger(L, 11, 255), (uint8_t)luaL_optinteger(L, 12, 255),
        (uint8_t)luaL_optinteger(L, 13, 255), (uint8_t)luaL_optinteger(L, 14, 255));
    return 0;
}

/* Frustum */
static int l_frustum_gc(lua_State* L) {
    zsdk_lua_frustum_ud* ud = (zsdk_lua_frustum_ud*)luaL_checkudata(L, 1, ZSDK_LUA_FRUSTUM_MT);
    if (ud->ptr) { zsdk_frustum_destroy(ud->ptr); ud->ptr = NULL; }
    return 0;
}
static int l_frustum_new(lua_State* L) {
    zsdk_frustum_t* f = zsdk_frustum_create();
    if (!f) return luaL_error(L, "zsdk.graphics.new_frustum: out of memory");
    zsdk_lua_frustum_ud* ud = (zsdk_lua_frustum_ud*)lua_newuserdata(L, sizeof(*ud));
    ud->ptr = f;
    luaL_getmetatable(L, ZSDK_LUA_FRUSTUM_MT);
    lua_setmetatable(L, -2);
    return 1;
}
static int l_frustum_extract_camera(lua_State* L) {
    zsdk_frustum_extract_camera(check_frustum(L, 1),
        (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4),
        (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6), (float)luaL_checknumber(L, 7),
        (float)luaL_checknumber(L, 8), (float)luaL_checknumber(L, 9), (float)luaL_checknumber(L, 10),
        (float)luaL_checknumber(L, 11), (float)luaL_checknumber(L, 12),
        (float)luaL_checknumber(L, 13), (float)luaL_checknumber(L, 14));
    return 0;
}
static int l_frustum_test_sphere(lua_State* L) {
    lua_pushboolean(L, zsdk_frustum_test_sphere(check_frustum(L, 1),
        (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3),
        (float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5)));
    return 1;
}
static int l_frustum_test_aabb(lua_State* L) {
    lua_pushboolean(L, zsdk_frustum_test_aabb(check_frustum(L, 1),
        (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4),
        (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6), (float)luaL_checknumber(L, 7)));
    return 1;
}

/* Renderer */
static int l_gfx_reset_state(lua_State* L) { (void)L; zsdk_render_reset_state_cache(); return 0; }
static int l_gfx_set_viewport(lua_State* L) { zsdk_render_set_viewport((int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3), (int32_t)luaL_checkinteger(L, 4)); return 0; }
static int l_gfx_set_scissor(lua_State* L) { zsdk_render_set_scissor(lua_toboolean(L, 1), (int32_t)luaL_optinteger(L, 2, 0), (int32_t)luaL_optinteger(L, 3, 0), (int32_t)luaL_optinteger(L, 4, 0), (int32_t)luaL_optinteger(L, 5, 0)); return 0; }
static int l_gfx_clear(lua_State* L) { zsdk_render_clear((uint8_t)luaL_optinteger(L, 1, 0), (uint8_t)luaL_optinteger(L, 2, 0), (uint8_t)luaL_optinteger(L, 3, 0), (uint8_t)luaL_optinteger(L, 4, 255), lua_isnone(L, 5) ? 1 : lua_toboolean(L, 5), lua_toboolean(L, 6)); return 0; }
static int l_gfx_set_blend_mode(lua_State* L) { zsdk_render_set_blend_mode((zsdk_blend_mode_t)luaL_checkinteger(L, 1)); return 0; }
static int l_gfx_set_depth_test(lua_State* L) { zsdk_render_set_depth_test(lua_toboolean(L, 1)); return 0; }
static int l_gfx_set_cull_face(lua_State* L) { zsdk_render_set_cull_face(lua_toboolean(L, 1)); return 0; }
static int l_gfx_set_ortho(lua_State* L) { zsdk_render_set_ortho((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), (float)luaL_optnumber(L, 5, -1.0), (float)luaL_optnumber(L, 6, 1.0)); return 0; }
static int l_gfx_set_perspective(lua_State* L) { zsdk_render_set_perspective((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4)); return 0; }
static int l_gfx_reset_modelview(lua_State* L) { (void)L; zsdk_render_reset_modelview(); return 0; }
static int l_gfx_push_matrix(lua_State* L) { (void)L; zsdk_render_push_matrix(); return 0; }
static int l_gfx_pop_matrix(lua_State* L) { (void)L; zsdk_render_pop_matrix(); return 0; }
static int l_gfx_look_at(lua_State* L) { zsdk_render_look_at((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6), (float)luaL_optnumber(L, 7, 0.0), (float)luaL_optnumber(L, 8, 1.0), (float)luaL_optnumber(L, 9, 0.0)); return 0; }
static int l_gfx_translate(lua_State* L) { zsdk_render_translate((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)); return 0; }
static int l_gfx_rotate(lua_State* L) { zsdk_render_rotate((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4)); return 0; }
static int l_gfx_scale(lua_State* L) { zsdk_render_scale((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)); return 0; }
static int l_gfx_set_color(lua_State* L) { zsdk_render_set_color((uint8_t)luaL_checkinteger(L, 1), (uint8_t)luaL_checkinteger(L, 2), (uint8_t)luaL_checkinteger(L, 3), (uint8_t)luaL_optinteger(L, 4, 255)); return 0; }
static int l_gfx_draw_arrays(lua_State* L) { zsdk_render_draw_arrays((zsdk_primitive_type_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3)); return 0; }
static int l_gfx_begin_2d(lua_State* L) { zsdk_render_begin_2d((int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2)); return 0; }
static int l_gfx_begin_2d_virtual(lua_State* L) { zsdk_render_begin_2d_virtual((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (int32_t)luaL_checkinteger(L, 3), (int32_t)luaL_checkinteger(L, 4)); return 0; }
static int l_gfx_end_2d(lua_State* L) { (void)L; zsdk_render_end_2d(); return 0; }
static int l_gfx_draw_rect_2d(lua_State* L) { zsdk_render_draw_rect_2d((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), (uint8_t)luaL_optinteger(L, 5, 255), (uint8_t)luaL_optinteger(L, 6, 255), (uint8_t)luaL_optinteger(L, 7, 255), (uint8_t)luaL_optinteger(L, 8, 255)); return 0; }
static int l_gfx_draw_text_2d(lua_State* L) { zsdk_render_draw_text_2d((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), luaL_checkstring(L, 4), (uint8_t)luaL_optinteger(L, 5, 255), (uint8_t)luaL_optinteger(L, 6, 255), (uint8_t)luaL_optinteger(L, 7, 255), (uint8_t)luaL_optinteger(L, 8, 255)); return 0; }
static int l_gfx_draw_textured_quad_2d(lua_State* L) { zsdk_render_draw_textured_quad_2d(check_tex(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), (float)luaL_optnumber(L, 6, 0.0), (float)luaL_optnumber(L, 7, 0.0), (float)luaL_optnumber(L, 8, 1.0), (float)luaL_optnumber(L, 9, 1.0), (uint8_t)luaL_optinteger(L, 10, 255), (uint8_t)luaL_optinteger(L, 11, 255), (uint8_t)luaL_optinteger(L, 12, 255), (uint8_t)luaL_optinteger(L, 13, 255)); return 0; }
static int l_gfx_set_fog(lua_State* L) { zsdk_render_set_fog(lua_toboolean(L, 1)); return 0; }
static int l_gfx_configure_fog(lua_State* L) { zsdk_render_configure_fog((uint8_t)luaL_checkinteger(L, 1), (uint8_t)luaL_checkinteger(L, 2), (uint8_t)luaL_checkinteger(L, 3), (uint8_t)luaL_checkinteger(L, 4), (uint8_t)luaL_optinteger(L, 5, 255), (float)luaL_checknumber(L, 6), (float)luaL_checknumber(L, 7), (float)luaL_optnumber(L, 8, 1.0)); return 0; }
static int l_gfx_set_lighting(lua_State* L) { zsdk_render_set_lighting(lua_toboolean(L, 1)); return 0; }
static int l_gfx_set_alpha_test(lua_State* L) { zsdk_render_set_alpha_test(lua_toboolean(L, 1), (uint8_t)luaL_optinteger(L, 2, 4), (float)luaL_optnumber(L, 3, 0.5)); return 0; }
static int l_gfx_set_stencil_test(lua_State* L) { zsdk_render_set_stencil_test(lua_toboolean(L, 1)); return 0; }
static int l_gfx_set_color_mask(lua_State* L) { zsdk_render_set_color_mask(lua_toboolean(L, 1), lua_toboolean(L, 2), lua_toboolean(L, 3), lua_toboolean(L, 4)); return 0; }
static int l_gfx_draw_blob_shadow(lua_State* L) { zsdk_render_draw_blob_shadow((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4), (uint8_t)luaL_optinteger(L, 5, 0), (uint8_t)luaL_optinteger(L, 6, 0), (uint8_t)luaL_optinteger(L, 7, 0), (uint8_t)luaL_optinteger(L, 8, 110)); return 0; }

static int l_gfx_draw_colored_mesh(lua_State* L) {
    typedef struct { float x, y, z; uint8_t r, g, b, a; } ColorVtx;
    enum { kMaxVerts = 256, kMaxIndices = 1024 };
    ColorVtx verts[kMaxVerts];
    uint16_t indices[kMaxIndices];
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);
    int vcount = (int)zsdk_lua_rawlen(L, 1);
    int icount = (int)zsdk_lua_rawlen(L, 2);
    if (vcount <= 0 || vcount > kMaxVerts || icount <= 0 || icount > kMaxIndices) {
        return luaL_error(L, "draw_colored_mesh: vertex/index count out of range");
    }
    for (int i = 0; i < vcount; ++i) {
        lua_rawgeti(L, 1, i + 1);
        lua_rawgeti(L, -1, 1); verts[i].x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); verts[i].y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); verts[i].z = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 4); verts[i].r = (uint8_t)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, -1, 5); verts[i].g = (uint8_t)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, -1, 6); verts[i].b = (uint8_t)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_rawgeti(L, -1, 7); verts[i].a = (uint8_t)luaL_optinteger(L, -1, 255); lua_pop(L, 1);
        lua_pop(L, 1);
    }
    for (int i = 0; i < icount; ++i) {
        lua_rawgeti(L, 2, i + 1);
        indices[i] = (uint16_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    zsdk_render_bind_raw_vertex_arrays(&verts[0].x, 3, NULL, &verts[0].r, NULL, (int32_t)sizeof(ColorVtx));
    zsdk_render_draw_elements_u16(ZSDK_PRIM_TRIANGLES, (int32_t)icount, indices);
    zsdk_render_unbind_vertex_attributes();
    return 0;
}

static const luaL_Reg kGlMethods[]      = { { "make_current", l_gl_make_current }, { "swap_buffers", l_gl_swap_buffers }, { "has_extension", l_gl_has_extension }, { "set_swap_interval", l_gl_set_swap_interval }, { "size", l_gl_size }, { "__gc", l_gl_gc }, { NULL, NULL } };
static const luaL_Reg kTexMethods[]     = { { "init_2d", l_tex_init_2d }, { "from_atitc", l_tex_from_atitc }, { "update_sub_image", l_tex_update_sub }, { "bind", l_tex_bind }, { "set_wrap", l_tex_set_wrap }, { "is_valid", l_tex_is_valid }, { "gl_id", l_tex_gl_id }, { "size", l_tex_size }, { "__gc", l_tex_gc }, { NULL, NULL } };
static const luaL_Reg kMeshMethods[]    = { { "init", l_mesh_init }, { "update", l_mesh_update }, { "release", l_mesh_release }, { "is_valid", l_mesh_is_valid }, { "__gc", l_mesh_gc }, { NULL, NULL } };
static const luaL_Reg kBatchMethods[]   = { { "begin", l_batch_begin }, { "begin_virtual", l_batch_begin_virtual }, { "finish", l_batch_end }, { "flush", l_batch_flush }, { "draw_rect", l_batch_draw_rect }, { "draw_quad", l_batch_draw_quad }, { "__gc", l_batch_gc }, { NULL, NULL } };
static const luaL_Reg kFrustumMethods[] = { { "extract_camera", l_frustum_extract_camera }, { "test_sphere", l_frustum_test_sphere }, { "test_aabb", l_frustum_test_aabb }, { "__gc", l_frustum_gc }, { NULL, NULL } };

static const luaL_Reg kGfxFuncs[] = {
    { "new_gl",                l_gl_new                   },
    { "new_texture",           l_tex_new                  },
    { "new_mesh",              l_mesh_new                 },
    { "new_sprite_batch",      l_batch_new                },
    { "new_frustum",           l_frustum_new              },
    { "reset_state_cache",     l_gfx_reset_state          },
    { "set_viewport",          l_gfx_set_viewport         },
    { "set_scissor",           l_gfx_set_scissor          },
    { "clear",                 l_gfx_clear                },
    { "set_blend_mode",        l_gfx_set_blend_mode       },
    { "set_depth_test",        l_gfx_set_depth_test       },
    { "set_cull_face",         l_gfx_set_cull_face        },
    { "set_ortho",             l_gfx_set_ortho            },
    { "set_perspective",       l_gfx_set_perspective      },
    { "reset_modelview",       l_gfx_reset_modelview      },
    { "push_matrix",           l_gfx_push_matrix          },
    { "pop_matrix",            l_gfx_pop_matrix           },
    { "look_at",               l_gfx_look_at              },
    { "translate",             l_gfx_translate            },
    { "rotate",                l_gfx_rotate               },
    { "scale",                 l_gfx_scale                },
    { "set_color",             l_gfx_set_color            },
    { "draw_arrays",           l_gfx_draw_arrays          },
    { "begin_2d",              l_gfx_begin_2d             },
    { "begin_2d_virtual",      l_gfx_begin_2d_virtual     },
    { "end_2d",                l_gfx_end_2d               },
    { "draw_rect_2d",          l_gfx_draw_rect_2d         },
    { "draw_text_2d",          l_gfx_draw_text_2d         },
    { "draw_textured_quad_2d", l_gfx_draw_textured_quad_2d},
    { "set_fog",               l_gfx_set_fog              },
    { "configure_fog",         l_gfx_configure_fog        },
    { "set_lighting",          l_gfx_set_lighting         },
    { "set_alpha_test",        l_gfx_set_alpha_test       },
    { "set_stencil_test",      l_gfx_set_stencil_test     },
    { "set_color_mask",        l_gfx_set_color_mask       },
    { "draw_blob_shadow",      l_gfx_draw_blob_shadow     },
    { "draw_colored_mesh",     l_gfx_draw_colored_mesh    },
    { NULL,                    NULL                       }
};

int luaopen_zsdk_graphics(lua_State* L) {
    if (luaL_newmetatable(L, ZSDK_LUA_GL_MT))      { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kGlMethods);      } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_TEX_MT))     { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kTexMethods);     } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_MESH_MT))    { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kMeshMethods);    } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_BATCH_MT))   { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kBatchMethods);   } lua_pop(L, 1);
    if (luaL_newmetatable(L, ZSDK_LUA_FRUSTUM_MT)) { lua_pushvalue(L, -1); lua_setfield(L, -2, "__index"); zsdk_lua_register_funcs(L, kFrustumMethods); } lua_pop(L, 1);

    lua_newtable(L);
    zsdk_lua_register_funcs(L, kGfxFuncs);
    lua_pushinteger(L, ZSDK_BLEND_OPAQUE);   lua_setfield(L, -2, "BLEND_OPAQUE");
    lua_pushinteger(L, ZSDK_BLEND_ALPHA);    lua_setfield(L, -2, "BLEND_ALPHA");
    lua_pushinteger(L, ZSDK_BLEND_ADDITIVE); lua_setfield(L, -2, "BLEND_ADDITIVE");
    return 1;
}
