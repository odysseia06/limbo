-- Coin Script
-- Collectible that destroys itself when touched by player

-- Optional: rotation animation
local rotationSpeed = 2.0
local time = 0

function onUpdate(dt)
    -- Simple floating animation (optional)
    time = time + dt * rotationSpeed
    -- Could animate position.y with sin(time) for bobbing effect
end

function onTriggerEnter(other)
    -- Check if the other entity is the player
    -- For now, just destroy on any contact (player would have the only dynamic body)
    local otherName = other:getName()
    if otherName == "Player" or otherName:find("Player") then
        -- Destroy this coin
        self:destroy()
    end
end
