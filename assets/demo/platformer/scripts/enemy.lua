-- Enemy Script
-- Simple enemy that dies when stomped from above by the player

function onCollisionBegin(other, normal)
    -- Only react to player (check by name for now)
    local otherName = other:getName()
    if otherName ~= "Player" then
        return
    end

    -- Check if stomped from above
    -- normal points from self (enemy) to other (player)
    -- If player is above us, normal.y > 0.5 (pointing upward toward player)
    if normal.y > 0.5 then
        -- Stomped! Destroy this enemy
        self:destroy()

        -- Give the player a small bounce
        local otherRb = other:getRigidbody()
        if otherRb then
            otherRb:setVelocityY(5.0) -- Small bounce
        end
    end
end

function onUpdate(dt)
    -- Could add patrol behavior here
end
