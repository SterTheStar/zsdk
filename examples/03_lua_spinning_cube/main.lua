-- ZSDK Example 03 (Lua): 1 Spinning Cube + Camera Controls + Debug Panel
-- Controls:
--   Left Stick / D-Pad : Move camera (pan)
--   Right Stick X      : Manual spin
--   Right Stick Y / ZL / ZR : Zoom in / out
--   Action1            : Toggle auto-spin
--   Action2            : Reset camera

local angle_y   = 0.0
local angle_x   = 0.0
local pan_x     = 0.0
local pan_y     = 0.0
local cam_dist  = 4.0
local auto_spin = true

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

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function zsdk_on_init(width, height)
    zsdk.graphics.reset_state_cache()
    zsdk.graphics.set_depth_test(true)
    zsdk.graphics.set_cull_face(true)
    zsdk.graphics.set_lighting(false)
    zsdk.core.set_overlay_visible(true)
    zsdk.log.info("LUA_CUBE", string.format("Ready %dx%d (RAM=%d B)", width, height, zsdk.core.ram_used()))
end

function zsdk_on_update(dt_ms, input)
    local dt = dt_ms * 0.001
    if auto_spin then
        angle_y = (angle_y + dt * 48.0) % 360.0
        angle_x = (angle_x + dt * 24.0) % 360.0
    end

    if input ~= nil then
        local lx, ly = input:left_stick(0)
        local rx, ry = input:right_stick(0)

        if input:is_down(0, zsdk.input.BTN_DPAD_LEFT)  then lx = lx - 1.0 end
        if input:is_down(0, zsdk.input.BTN_DPAD_RIGHT) then lx = lx + 1.0 end
        if input:is_down(0, zsdk.input.BTN_DPAD_UP)    then ly = ly + 1.0 end
        if input:is_down(0, zsdk.input.BTN_DPAD_DOWN)  then ly = ly - 1.0 end

        pan_x = clamp(pan_x + lx * 4.0 * dt, -6.0, 6.0)
        pan_y = clamp(pan_y + ly * 4.0 * dt, -4.0, 4.0)
        angle_y = (angle_y + rx * 120.0 * dt) % 360.0

        local zoom = ry * 6.0
        if input:is_down(0, zsdk.input.BTN_SHOULDER_LEFT)  then zoom = zoom - 6.0 end
        if input:is_down(0, zsdk.input.BTN_SHOULDER_RIGHT) then zoom = zoom + 6.0 end
        cam_dist = clamp(cam_dist + zoom * dt, 2.2, 20.0)

        if input:is_pressed(0, zsdk.input.BTN_ACTION1) then
            auto_spin = not auto_spin
            zsdk.log.info("LUA_CUBE", auto_spin and "Auto-spin: ON" or "Auto-spin: OFF")
        end
        if input:is_pressed(0, zsdk.input.BTN_ACTION2) then
            pan_x, pan_y, cam_dist = 0.0, 0.0, 4.0
            zsdk.log.info("LUA_CUBE", "Camera reset")
        end
    end
end

function zsdk_on_render(width, height)
    local aspect = (height > 0) and (width / height) or 1.333

    zsdk.graphics.set_viewport(0, 0, width, height)
    zsdk.graphics.clear(20, 24, 34, 255, true, false)

    zsdk.graphics.set_perspective(45.0, aspect, 0.1, 50.0)
    zsdk.graphics.reset_modelview()
    zsdk.graphics.look_at(pan_x, 1.2 + pan_y, cam_dist, pan_x, pan_y, 0.0)

    zsdk.graphics.draw_blob_shadow(0.0, -1.25, 0.0, 1.15, 0, 0, 0, 110)

    zsdk.graphics.push_matrix()
    zsdk.graphics.rotate(angle_y, 0.0, 1.0, 0.0)
    zsdk.graphics.rotate(angle_x, 1.0, 0.0, 0.0)
    zsdk.graphics.draw_colored_mesh(CUBE_VERTS, CUBE_INDICES)
    zsdk.graphics.pop_matrix()

    zsdk.core.record_draw_call(24)
end
