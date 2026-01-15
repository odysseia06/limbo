-- Moving Platform Script
-- A kinematic platform that moves up and down

-- Movement settings
local amplitude = 2.0 -- How far up/down to move
local speed = 1.0     -- Speed of oscillation

-- State
local startY = 0
local time = 0

function onStart()
    -- Store initial position
    startY = self.position.y
end

function onUpdate(dt)
    time = time + dt * speed

    -- Calculate target Y position using sine wave
    local targetY = startY + math.sin(time) * amplitude

    -- For kinematic bodies, we set velocity to reach target
    local rb = self:getRigidbody()
    if rb then
        local currentY = self.position.y
        local velocityY = (targetY - currentY) / dt
        -- Clamp velocity to avoid huge jumps
        velocityY = math.max(-10, math.min(10, velocityY))
        rb:setVelocity(0, velocityY)
    end
end
