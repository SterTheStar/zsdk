local W, H = gfx.size()
local state, score, best, lives, t
local px, stars, seed = 300, {}, 1234
local padless = false   -- Zeebo requires games to handle gamepad detection

local function rnd(n)            -- ZX81 LCG (fits int32: Lua runs with 32-bit integers on Zeebo)
  seed = (seed * 75 + 74) % 65537
  return seed % n
end

local function reset()
  score, lives, t, px, stars = 0, 3, 0, 300, {}
  state = "play"
end

function init()
  local s = save.read(0)
  best = s and tonumber(s) or 0
  state = "title"
  padless = not input.connected(0)
  app.log("Star Catcher ready, best=" .. best .. (app.sdcard() and " (SD)" or ""))
end

function on_pad(i, connected)
  if i == 0 then padless = not connected end
end

function update(dt)
  if input.pressed("home") then app.quit() return end
  if padless then return end
  if state ~= "play" then
    if input.pressed("a") then reset() end
    return
  end
  t = t + dt
  local sx = input.stick(0)
  local dir = sx
  if input.down("left")  then dir = dir - 1 end
  if input.down("right") then dir = dir + 1 end
  px = math.max(0, math.min(W - 96, px + dir * 420 * dt))

  if rnd(100) < 3 + math.min(6, t // 10) then
    stars[#stars + 1] = { x = rnd(W - 24), y = -24, v = 120 + rnd(120) }
  end
  for i = #stars, 1, -1 do
    local s = stars[i]
    s.y = s.y + s.v * dt
    if s.y > H - 40 and s.y < H - 12 and s.x + 24 > px and s.x < px + 96 then
      score = score + 1
      table.remove(stars, i)
    elseif s.y > H then
      lives = lives - 1
      table.remove(stars, i)
    end
  end
  if lives <= 0 then
    state = "over"
    if score > best then best = score; save.write_async(0, tostring(best)) end
  end
end

function draw()
  gfx.clear(10, 12, 28)
  if padless then
    gfx.text(180, 220, "CONNECT A CONTROLLER", 255, 200, 80, 2)
    return
  end
  if state == "title" then
    gfx.text(220, 200, "STAR CATCHER (Lua)", 255, 230, 90, 2)
    gfx.text(250, 260, "Press Button 1", 200, 220, 255)
    gfx.text(250, 280, "Best: " .. best, 160, 255, 160)
    return
  end
  for _, s in ipairs(stars) do gfx.rect(s.x, s.y, 24, 24, 255, 220, 60) end
  gfx.rect(px, H - 28, 96, 16, 90, 190, 255)
  gfx.rect(0, 0, W, 28, 0, 0, 0, 160)
  gfx.text(10, 10, "Score " .. score .. "   Lives " .. lives .. "   Best " .. best, 255, 255, 255)
  if state == "over" then
    gfx.text(250, 220, "GAME OVER", 255, 90, 90, 2)
    gfx.text(235, 260, "Button 1 to retry", 200, 220, 255)
  end
end
