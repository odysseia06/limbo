-- platformer_player.lua - Platformer demo showcasing physics integration
-- Demonstrates: Physics queries, velocity control, collision callbacks

-- Configuration
local moveSpeed = 5.0
local jumpForce = 8.0
local groundCheckDistance = 0.55 -- Slightly more than half the player height

-- State
local isGrounded = false
local canJump = true
local jumpCooldown = 0.0

-- Called once when the script starts
function onStart()
    log.info("Platformer player initialized!")
    log.info("  Controls: A/D to move, Space to jump")
    log.info("  Entity: " .. self:getName())
end

-- Called every frame with delta time
function onUpdate(dt)
    -- Update cooldowns
    if jumpCooldown > 0 then
        jumpCooldown = jumpCooldown - dt
    end

    -- Ground check using raycast
    local pos = self:getPosition()
    local groundHit = Physics.raycast(
        Vec2.new(pos.x, pos.y),
        Vec2.new(0, -1), -- Cast downward
        groundCheckDistance,
        false            -- Don't include triggers
    )

    isGrounded = (groundHit ~= nil)

    -- Horizontal movement
    local vel = self:getVelocity()
    local targetVelX = 0

    if Input.isKeyDown(Key.A) or Input.isKeyDown(Key.Left) then
        targetVelX = -moveSpeed
    end
    if Input.isKeyDown(Key.D) or Input.isKeyDown(Key.Right) then
        targetVelX = moveSpeed
    end

    -- Apply horizontal velocity directly (responsive controls)
    self:setVelocity(Vec2.new(targetVelX, vel.y))

    -- Jumping
    if isGrounded and jumpCooldown <= 0 then
        if Input.isKeyPressed(Key.Space) or Input.isKeyPressed(Key.W) or Input.isKeyPressed(Key.Up) then
            -- Apply jump impulse
            self:applyImpulse(Vec2.new(0, jumpForce))
            jumpCooldown = 0.2 -- Small cooldown to prevent double jumps
            log.debug("Jump!")
        end
    end

    -- Visual feedback: change color based on state
    local color = self:getColor()
    if isGrounded then
        color.r = 0.2
        color.g = 0.8
        color.b = 0.2
    else
        -- In air: blue tint
        color.r = 0.2
        color.g = 0.4
        color.b = 0.9
    end
    color.a = 1.0
    self:setColor(color)
end

-- Called when collision begins (self-relative: normal points away from self)
function onCollisionBegin(other, normal)
    -- Check if we landed on top of something (stomping)
    if normal.y > 0.7 then
        log.debug("Landed on: " .. (other:getName() or "unknown"))
    end

    -- Check if we hit our head
    if normal.y < -0.7 then
        log.debug("Bumped head!")
        -- Cancel upward velocity
        local vel = self:getVelocity()
        if vel.y > 0 then
            self:setVelocity(Vec2.new(vel.x, 0))
        end
    end
end

-- Called when entering a trigger
function onTriggerEnter(other)
    local otherName = other:getName() or "unknown"
    log.info("Entered trigger: " .. otherName)

    -- Example: collectibles, hazards, etc.
    if otherName == "Coin" then
        log.info("Collected a coin!")
        -- other:destroy()  -- Would destroy the coin
    elseif otherName == "Hazard" then
        log.warn("Hit a hazard!")
        -- Respawn logic would go here
    end
end

-- Called when exiting a trigger
function onTriggerExit(other)
    local otherName = other:getName() or "unknown"
    log.debug("Exited trigger: " .. otherName)
end

-- Called when the script/entity is destroyed
function onDestroy()
    log.info("Platformer player destroyed")
end
