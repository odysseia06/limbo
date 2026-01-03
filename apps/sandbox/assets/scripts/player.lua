-- player.lua - Demo script showing Lua scripting capabilities
-- This script controls a green square using I/J/K/L keys

-- Movement speed
local speed = 2.0

-- Called once when the script starts
function onStart()
    log.info("Player script started!")
    log.info("  Entity: " .. self:getName())
    log.info("  Position: " .. self:getPosition().x .. ", " .. self:getPosition().y)
end

-- Called every frame with delta time
function onUpdate(dt)
    local pos = self:getPosition()
    local moved = false

    -- I/K for up/down movement
    if Input.isKeyDown(Key.I) then
        pos.y = pos.y + speed * dt
        moved = true
    end
    if Input.isKeyDown(Key.K) then
        pos.y = pos.y - speed * dt
        moved = true
    end

    -- J/L for left/right movement
    if Input.isKeyDown(Key.J) then
        pos.x = pos.x - speed * dt
        moved = true
    end
    if Input.isKeyDown(Key.L) then
        pos.x = pos.x + speed * dt
        moved = true
    end

    -- Apply position if moved
    if moved then
        self:setPosition(pos)
    end

    -- Change color based on position (just for fun)
    local color = self:getColor()
    color.r = 0.2 + math.abs(math.sin(Time.totalTime)) * 0.3
    color.g = 1.0
    color.b = 0.4 + math.abs(math.cos(Time.totalTime * 0.5)) * 0.4
    self:setColor(color)
end

-- Called when the script/entity is destroyed
function onDestroy()
    log.info("Player script destroyed")
end
