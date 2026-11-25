-- ZSDK Example 03 (Lua): 1 Spinning Cube + Debug Panel
-- Requires `zsdk` module registered via `luaopen_zsdk(L)`.

local angle = 0.0

local CUBE_VERTS = {
    -- Front (+Z) - Cyan
    {-0.8, -0.8,  0.8,  60, 180, 255, 255}, { 0.8, -0.8,  0.8,  60, 180, 255, 255},
    { 0.8,  0.8,  0.8,  60, 180, 255, 255}, {-0.8,  0.8,  0.8,  60, 180, 255, 255},
    -- Back (-Z) - Orange
    { 0.8, -0.8, -0.8, 255, 140,  50, 255}, {-0.8, -0.8, -0.8, 255, 140,  50, 255},
    {-0.8,  0.8, -0.8, 255, 140,  50, 255}, { 0.8,  0.8, -0.8, 255, 140,  50, 255},
    -- Left (-X) - Green
    {-0.8, -0.8, -0.8,  70, 220, 120, 255}, {-0.8, -0.8,  0.8,  70, 220, 120, 255},
    {-0.8,  0.8,  0.8,  70, 220, 120, 255}, {-0.8,  0.8, -0.8,  70, 220, 120, 255},
    -- Right (+X) - Magenta
    { 0.8, -0.8,  0.8, 230,  80, 190, 255}, { 0.8, -0.8, -0.8, 230,  80, 190, 255},
    { 0.8,  0.8, -0.8, 230,  80, 190, 255}, { 0.8,  0.8,  0.8, 230,  80, 190, 255},
    -- Top (+Y) - Yellow
    {-0.8,  0.8,  0.8, 250, 220,  60, 255}, { 0.8,  0.8,  0.8, 250, 220,  60, 255},
    { 0.8,  0.8, -0.8, 250, 220,  60, 255}, {-0.8,  0.8, -0.8, 250, 220,  60, 255},
    -- Bottom (-Y) - Red
    {-0.8, -0.8, -0.8, 235,  65,  65, 255}, { 0.8, -0.8, -0.8, 235,  65,  65, 255},
    { 0.8, -0.8,  0.8, 235,  65,  65, 255}, {-0.8, -0.8,  0.8, 235,  65,  65, 255}
}

local CUBE_INDICES = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23
}

function zsdk_on_init(width, height)
    zsdk.graphics.reset_state_cache()
    zsdk.graphics.set_depth_test(true)
    zsdk.graphics.set_cull_face(true)
    zsdk.core.set_overlay_visible(true)
    zsdk.log.info("LUA_CUBE", string.format("Initialized (%dx%d)", width, height))
end

function zsdk_on_update(dt_ms)
    angle = (angle + (dt_ms * 0.001) * 45.0) % 360.0
end

function zsdk_on_render(width, height)
    local aspect = (height > 0) and (width / height) or 1.333

    zsdk.graphics.set_viewport(0, 0, width, height)
    zsdk.graphics.clear(20, 24, 34, 255, true, false)

    zsdk.graphics.set_perspective(45.0, aspect, 0.1, 50.0)
    zsdk.graphics.reset_modelview()
    zsdk.graphics.look_at(0.0, 1.2, 3.6, 0.0, 0.0, 0.0)
    zsdk.graphics.rotate(angle, 0.0, 1.0, 0.0)
    zsdk.graphics.rotate(angle * 0.5, 1.0, 0.0, 0.0)

    zsdk.graphics.draw_colored_mesh(CUBE_VERTS, CUBE_INDICES)
    zsdk.core.record_draw_call(24)
end
