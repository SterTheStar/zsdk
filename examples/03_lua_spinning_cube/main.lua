-- ZSDK Example 03 (Pure Lua): 1:1 clone of 01_spinning_cube.
-- 1..1024 Spinning Lit 3D Cubes + Frustum/Face Culling + SpriteBatch2D Menu + udata Save.

local log  = zsdk.log
local core = zsdk.core
local inp  = zsdk.input
local gfx  = zsdk.graphics
local fs   = zsdk.fs

local CUBE_COUNTS = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024}
local GRID_COLS   = {1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 16}
local GRID_ROWS   = {1, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8}
local GRID_DIST   = {4.5, 6.5, 8.0, 10.5, 13.0, 16.0, 19.0, 23.0, 28.0, 33.0, 38.0}

local CUBE_VERTS = {
    -- Front (+Z) - Cyan
    {-1.0, -1.0,  1.0,  0.0,  0.0,  1.0,  60, 180, 255, 255},
    { 1.0, -1.0,  1.0,  0.0,  0.0,  1.0,  60, 180, 255, 255},
    { 1.0,  1.0,  1.0,  0.0,  0.0,  1.0,  60, 180, 255, 255},
    {-1.0,  1.0,  1.0,  0.0,  0.0,  1.0,  60, 180, 255, 255},
    -- Back (-Z) - Orange
    { 1.0, -1.0, -1.0,  0.0,  0.0, -1.0, 255, 140,  50, 255},
    {-1.0, -1.0, -1.0,  0.0,  0.0, -1.0, 255, 140,  50, 255},
    {-1.0,  1.0, -1.0,  0.0,  0.0, -1.0, 255, 140,  50, 255},
    { 1.0,  1.0, -1.0,  0.0,  0.0, -1.0, 255, 140,  50, 255},
    -- Left (-X) - Green
    {-1.0, -1.0, -1.0, -1.0,  0.0,  0.0,  70, 220, 120, 255},
    {-1.0, -1.0,  1.0, -1.0,  0.0,  0.0,  70, 220, 120, 255},
    {-1.0,  1.0,  1.0, -1.0,  0.0,  0.0,  70, 220, 120, 255},
    {-1.0,  1.0, -1.0, -1.0,  0.0,  0.0,  70, 220, 120, 255},
    -- Right (+X) - Magenta
    { 1.0, -1.0,  1.0,  1.0,  0.0,  0.0, 230,  80, 190, 255},
    { 1.0, -1.0, -1.0,  1.0,  0.0,  0.0, 230,  80, 190, 255},
    { 1.0,  1.0, -1.0,  1.0,  0.0,  0.0, 230,  80, 190, 255},
    { 1.0,  1.0,  1.0,  1.0,  0.0,  0.0, 230,  80, 190, 255},
    -- Top (+Y) - Yellow
    {-1.0,  1.0,  1.0,  0.0,  1.0,  0.0, 250, 220,  60, 255},
    { 1.0,  1.0,  1.0,  0.0,  1.0,  0.0, 250, 220,  60, 255},
    { 1.0,  1.0, -1.0,  0.0,  1.0,  0.0, 250, 220,  60, 255},
    {-1.0,  1.0, -1.0,  0.0,  1.0,  0.0, 250, 220,  60, 255},
    -- Bottom (-Y) - Red
    {-1.0, -1.0, -1.0,  0.0, -1.0,  0.0, 235,  65,  65, 255},
    { 1.0, -1.0, -1.0,  0.0, -1.0,  0.0, 235,  65,  65, 255},
    { 1.0, -1.0,  1.0,  0.0, -1.0,  0.0, 235,  65,  65, 255},
    {-1.0, -1.0,  1.0,  0.0, -1.0,  0.0, 235,  65,  65, 255}
}

local CUBE_IDX = {
     0,  1,  2,   0,  2,  3,
     4,  5,  6,   4,  6,  7,
     8,  9, 10,   8, 10, 11,
    12, 13, 14,  12, 14, 15,
    16, 17, 18,  16, 18, 19,
    20, 21, 22,  20, 22, 23
}

local MENU_ITEMS   = 10
local SAVE_VERSION = 2

local angleX        = 25.0
local angleY        = 35.0
local camPanX       = 0.0
local camPanY       = 0.0
local camZoomOffset = 0.0
local menuOpen      = false
local menuCursor    = 1

local settings = {
    lighting       = 1,
    fpsMode        = 1,
    vsync          = 1,
    spinMode       = 2,
    cubeCountIndex = 1, -- 1-based (1..11)
    frustumCull    = 1,
    faceCull       = 1,
}

local batch   = nil
local frustum = nil
local saveMgr = nil

local function apply_fps()
    core.set_fps_cap((settings.fpsMode == 1) and 30 or 60)
end

local function apply_settings()
    apply_fps()
    if zsdk_set_swap_interval then
        zsdk_set_swap_interval(settings.vsync)
    end
end

local function adjust_menu_item(dir)
    if menuCursor == 1 then
        settings.cubeCountIndex = ((settings.cubeCountIndex - 1 + 11 + dir) % 11) + 1
        camPanX = 0.0
        camPanY = 0.0
        camZoomOffset = 0.0
    elseif menuCursor == 2 then
        settings.frustumCull = (settings.frustumCull == 0) and 1 or 0
    elseif menuCursor == 3 then
        settings.faceCull = (settings.faceCull == 0) and 1 or 0
    elseif menuCursor == 4 then
        settings.lighting = (settings.lighting == 0) and 1 or 0
    elseif menuCursor == 5 then
        settings.fpsMode = (settings.fpsMode == 0) and 1 or 0
        apply_fps()
    elseif menuCursor == 6 then
        settings.vsync = (settings.vsync == 0) and 1 or 0
        if zsdk_set_swap_interval then zsdk_set_swap_interval(settings.vsync) end
    elseif menuCursor == 7 then
        settings.spinMode = (settings.spinMode + 4 + dir) % 4
    elseif menuCursor == 8 then
        core.set_overlay_visible(not core.is_overlay_visible())
    elseif menuCursor == 9 then
        if saveMgr then
            local blob = string.char(
                settings.lighting,
                settings.fpsMode,
                settings.vsync,
                settings.spinMode,
                settings.cubeCountIndex - 1,
                settings.frustumCull,
                settings.faceCull
            )
            saveMgr:save_slot(0, blob, SAVE_VERSION)
            log.info("CUBE_LUA", "Saved settings to udata.")
        end
    elseif menuCursor == 10 then
        core.request_quit()
    end
end

local function update_menu(input)
    if input:is_pressed(0, inp.BTN_DPAD_UP) then
        menuCursor = (menuCursor == 1) and MENU_ITEMS or (menuCursor - 1)
    end
    if input:is_pressed(0, inp.BTN_DPAD_DOWN) then
        menuCursor = (menuCursor % MENU_ITEMS) + 1
    end
    if input:is_pressed(0, inp.BTN_DPAD_RIGHT) or input:is_pressed(0, inp.BTN_ACTION1) then
        adjust_menu_item(1)
    end
    if input:is_pressed(0, inp.BTN_DPAD_LEFT) then
        adjust_menu_item(-1)
    end
    if input:is_pressed(0, inp.BTN_BACK) then
        menuOpen = false
    end
end

local function render_hud(w, h, totalCubes, drawnCubes)
    batch:begin_virtual(640.0, 480.0, w, h)
    batch:draw_rect(10.0, 10.0, 290.0, 36.0, 10, 14, 22, 180)
    batch:flush()

    local line1 = string.format("CUBES: %d  DRAWN: %d", totalCubes, drawnCubes)
    local line2 = string.format("FRUSTUM:%s FACE:%s",
        (settings.frustumCull ~= 0) and "ON " or "OFF ",
        (settings.faceCull ~= 0) and "ON" or "OFF")

    gfx.draw_text_2d(18.0, 16.0, 1.0, line1, 235, 245, 255, 255)
    gfx.draw_text_2d(18.0, 30.0, 1.0, line2, 160, 215, 255, 255)
    batch:finish()
end

local function render_menu(w, h)
    batch:begin_virtual(640.0, 480.0, w, h)

    local menuW = 360.0
    local menuH = 224.0
    local mx = (640.0 - menuW) * 0.5
    local my = (480.0 - menuH) * 0.5

    batch:draw_rect(mx, my, menuW, menuH, 12, 16, 26, 230)
    batch:draw_rect(mx, my, menuW, 24.0, 35, 75, 130, 240)
    local selY = my + 34.0 + (menuCursor - 1) * 18.0
    batch:draw_rect(mx + 8.0, selY - 3.0, menuW - 16.0, 15.0, 55, 110, 180, 200)
    batch:flush()

    gfx.draw_text_2d(mx + 12.0, my + 8.0, 1.0, "ZSDK SETTINGS (START)", 255, 255, 255, 255)

    local spinNames = {[0]="Paused", [1]="Slow", [2]="Normal", [3]="Fast"}
    local labels = {
        string.format("Cube Count  : %d", CUBE_COUNTS[settings.cubeCountIndex]),
        (settings.frustumCull ~= 0) and "Frustum Cull: ON" or "Frustum Cull: OFF",
        (settings.faceCull    ~= 0) and "Face Cull   : ON" or "Face Cull   : OFF",
        (settings.lighting    ~= 0) and "Lighting    : ON" or "Lighting    : OFF",
        "FPS Cap     : " .. ((settings.fpsMode == 1) and "30 FPS" or "60 FPS"),
        (settings.vsync       ~= 0) and "VSync       : ON" or "VSync       : OFF",
        "Spin Speed  : " .. (spinNames[settings.spinMode] or "Normal"),
        core.is_overlay_visible() and "Debug Panel : SHOW" or "Debug Panel : HIDE",
        "Save Config (udata)",
        "Exit App"
    }

    for i = 1, MENU_ITEMS do
        local ry = my + 34.0 + (i - 1) * 18.0
        if i == menuCursor then
            gfx.draw_text_2d(mx + 16.0, ry, 1.0, labels[i], 255, 240, 120, 255)
        else
            gfx.draw_text_2d(mx + 16.0, ry, 1.0, labels[i], 210, 220, 230, 255)
        end
    end

    batch:finish()
end

function zsdk_on_init(w, h)
    batch = gfx.new_sprite_batch(64)
    frustum = gfx.new_frustum()
    gfx.configure_light(0, 6.0, 8.0, 10.0, 64, 64, 80, 255, 242, 242, 230, 255, 204, 204, 204, 255)

    saveMgr = fs.new_savegame()
    local res, blob = saveMgr:load_slot(0, 16, SAVE_VERSION)
    if res == zsdk.OK and blob and #blob == 7 then
        local b1, b2, b3, b4, b5, b6, b7 = string.byte(blob, 1, 7)
        settings.lighting       = b1
        settings.fpsMode        = b2
        settings.vsync          = b3
        settings.spinMode       = b4 % 4
        settings.cubeCountIndex = (b5 <= 10) and (b5 + 1) or 1
        settings.frustumCull    = (b6 ~= 0) and 1 or 0
        settings.faceCull       = (b7 ~= 0) and 1 or 0
        apply_settings()
        log.info("CUBE_LUA", "Loaded settings from udata.")
    else
        apply_settings()
        log.info("CUBE_LUA", string.format("Spinning Cube ready (%dx%d)", w, h))
    end
end

function zsdk_on_update(delta_ms, input)
    if not input then return end
    local dt = delta_ms * 0.001

    local menuBtn = input:is_pressed(0, inp.BTN_START) or input:is_pressed(0, inp.BTN_HOME)
    if menuBtn and not input:is_down(0, inp.BTN_DPAD_DOWN) then
        menuOpen = not menuOpen
        log.info("MENU", menuOpen and "Opened" or "Closed")
    end

    if menuOpen then
        update_menu(input)
    else
        local spinTable = {[0]=0.0, [1]=20.0, [2]=48.0, [3]=110.0}
        local baseSpin = spinTable[settings.spinMode % 4] or 48.0
        angleY = angleY + baseSpin * dt
        angleX = angleX + (baseSpin * 0.65) * dt

        local panX, panY = input:left_stick(0)
        if input:is_down(0, inp.BTN_DPAD_LEFT)  then panX = panX - 1.0 end
        if input:is_down(0, inp.BTN_DPAD_RIGHT) then panX = panX + 1.0 end
        if input:is_down(0, inp.BTN_DPAD_UP)    then panY = panY + 1.0 end
        if input:is_down(0, inp.BTN_DPAD_DOWN)  then panY = panY - 1.0 end

        camPanX = camPanX + panX * 16.0 * dt
        camPanY = camPanY - panY * 16.0 * dt
        if camPanX >  40.0 then camPanX =  40.0 end
        if camPanX < -40.0 then camPanX = -40.0 end
        if camPanY >  30.0 then camPanY =  30.0 end
        if camPanY < -30.0 then camPanY = -30.0 end

        local _, ry = input:right_stick(0)
        camZoomOffset = camZoomOffset + ry * 20.0 * dt
        if input:is_down(0, inp.BTN_SHOULDER_LEFT)  then camZoomOffset = camZoomOffset - 14.0 * dt end
        if input:is_down(0, inp.BTN_SHOULDER_RIGHT) then camZoomOffset = camZoomOffset + 14.0 * dt end
        if camZoomOffset < -30.0 then camZoomOffset = -30.0 end
        if camZoomOffset >  50.0 then camZoomOffset =  50.0 end

        if input:is_pressed(0, inp.BTN_ACTION1) then
            settings.lighting = (settings.lighting == 0) and 1 or 0
            log.info("CUBE_LUA", "Lighting: " .. ((settings.lighting ~= 0) and "ON" or "OFF"))
        end
        if input:is_pressed(0, inp.BTN_ACTION2) then
            settings.fpsMode = (settings.fpsMode == 1) and 0 or 1
            apply_fps()
        end
    end
end

function zsdk_on_render(w, h)
    gfx.set_viewport(0, 0, w, h)
    gfx.clear(16, 20, 32, 255, true, false)

    local aspect = (h > 0) and (w / h) or 1.333
    local fovY   = 55.0
    local nearZ  = 0.5
    local farZ   = 150.0

    local idx     = settings.cubeCountIndex
    local cols    = GRID_COLS[idx]
    local rows    = GRID_ROWS[idx]
    local camDist = GRID_DIST[idx] + camZoomOffset
    if camDist < 2.5  then camDist = 2.5  end
    if camDist > 90.0 then camDist = 90.0 end

    gfx.set_perspective(fovY, aspect, nearZ, farZ)
    gfx.reset_modelview()
    gfx.look_at(camPanX, camPanY, camDist, camPanX, camPanY, 0.0, 0.0, 1.0, 0.0)

    if settings.frustumCull ~= 0 then
        frustum:extract_camera(camPanX, camPanY, camDist, camPanX, camPanY, 0.0, 0.0, 1.0, 0.0, fovY, aspect, nearZ, farZ)
    end

    gfx.set_depth_test(true)
    gfx.set_cull_face(settings.faceCull ~= 0)
    gfx.set_lighting(settings.lighting ~= 0)

    gfx.bind_lit_mesh(CUBE_VERTS, CUBE_IDX)

    local totalCubes = CUBE_COUNTS[idx]
    local perLayer   = cols * rows
    local layers     = math.floor((totalCubes + perLayer - 1) / perLayer)
    local spacing    = 3.0
    local halfW      = (cols - 1) * 0.5 * spacing
    local halfH      = (rows - 1) * 0.5 * spacing
    local halfD      = (layers - 1) * 0.5 * spacing
    local cubeRadius = 1.75

    local drawn = 0
    for i = 0, totalCubes - 1 do
        local l  = math.floor(i / perLayer)
        local r  = i % perLayer
        local cx = r % cols
        local cy = math.floor(r / cols)

        local px = cx * spacing - halfW
        local py = cy * spacing - halfH
        local pz = -l * spacing + halfD

        if settings.frustumCull == 0 or frustum:test_sphere(px, py, pz, cubeRadius) then
            gfx.push_matrix()
            gfx.translate(px, py, pz)
            gfx.rotate(angleX + (i % 8) * 12.0, 1.0, 0.0, 0.0)
            gfx.rotate(angleY + (math.floor(i / 8) % 8) * 12.0, 0.0, 1.0, 0.0)
            gfx.draw_bound_mesh()
            gfx.pop_matrix()
            core.record_draw_call(24)
            drawn = drawn + 1
        end
    end

    gfx.unbind_mesh()
    gfx.set_lighting(false)

    if totalCubes == 1 then
        gfx.draw_blob_shadow(0.0, -1.6, 0.0, 1.45, 0, 0, 0, 120)
    end

    gfx.set_depth_test(false)

    render_hud(w, h, totalCubes, drawn)
    if menuOpen then
        render_menu(w, h)
    end
end
