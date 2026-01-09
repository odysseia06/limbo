-- gameplay_demo.lua - Complete gameplay demo showcasing scripting features
-- Demonstrates: Camera follow, shooting projectiles, triggers, hot reload

-- ============================================================================
-- Configuration (modify these values and save to test hot reload!)
-- ============================================================================
local config = {
    -- Movement
    moveSpeed = 6.0,
    jumpForce = 10.0,

    -- Shooting
    shootCooldown = 0.3,
    projectileSpeed = 15.0,

    -- Camera
    cameraFollowSpeed = 5.0,
    cameraOffset = Vec2.new(0, 2),

    -- Ground detection
    groundCheckDist = 0.55,
}

-- ============================================================================
-- State
-- ============================================================================
local state = {
    isGrounded = false,
    facingRight = true,
    shootTimer = 0,
    score = 0,
}

-- ============================================================================
-- Lifecycle Callbacks
-- ============================================================================

function onStart()
    log.info("=== Gameplay Demo Started ===")
    log.info("Controls:")
    log.info("  A/D or Left/Right - Move")
    log.info("  Space or W - Jump")
    log.info("  F or Left Click - Shoot")
    log.info("")
    log.info("Try editing this script and saving while playing!")
    log.info("The script will hot-reload automatically.")
end

function onUpdate(dt)
    updateMovement(dt)
    updateShooting(dt)
    updateCameraFollow(dt)
    updateVisuals()
end

function onDestroy()
    log.info("=== Gameplay Demo Ended ===")
    log.info("Final Score: " .. state.score)
end

-- ============================================================================
-- Hot Reload Callbacks (optional)
-- ============================================================================

-- Called before script is reloaded - save any state you want to preserve
function onBeforeReload()
    log.info("Script reloading... Score: " .. state.score)
    -- Return a table of values to preserve
    return {
        score = state.score,
        facingRight = state.facingRight,
    }
end

-- Called after script is reloaded - restore preserved state
function onAfterReload(preserved)
    if preserved then
        state.score = preserved.score or 0
        state.facingRight = preserved.facingRight or true
        log.info("Script reloaded! Score preserved: " .. state.score)
    end
end

-- ============================================================================
-- Movement System
-- ============================================================================

function updateMovement(dt)
    -- Ground check
    local pos = self:getPosition()
    local hit = Physics.raycast(
        Vec2.new(pos.x, pos.y),
        Vec2.new(0, -1),
        config.groundCheckDist,
        false
    )
    state.isGrounded = (hit ~= nil)

    -- Horizontal movement
    local vel = self:getVelocity()
    local moveDir = 0

    if Input.isKeyDown(Key.A) or Input.isKeyDown(Key.Left) then
        moveDir = -1
        state.facingRight = false
    end
    if Input.isKeyDown(Key.D) or Input.isKeyDown(Key.Right) then
        moveDir = 1
        state.facingRight = true
    end

    self:setVelocity(Vec2.new(moveDir * config.moveSpeed, vel.y))

    -- Jumping
    if state.isGrounded then
        if Input.isKeyPressed(Key.Space) or Input.isKeyPressed(Key.W) then
            self:applyImpulse(Vec2.new(0, config.jumpForce))
            log.debug("Jump!")
        end
    end
end

-- ============================================================================
-- Shooting System
-- ============================================================================

function updateShooting(dt)
    state.shootTimer = math.max(0, state.shootTimer - dt)

    -- Shoot with F key or left mouse button
    local wantsToShoot = Input.isKeyPressed(Key.F) or Input.isMouseButtonPressed(0)

    if wantsToShoot and state.shootTimer <= 0 then
        shoot()
        state.shootTimer = config.shootCooldown
    end
end

function shoot()
    local pos = self:getPosition()
    local direction = state.facingRight and 1 or -1

    -- Log shooting action (in a real game, we'd spawn a projectile entity)
    log.info("Pew! Shot " .. (state.facingRight and "right" or "left"))

    -- Check for hit using raycast
    local hit = Physics.raycast(
        Vec2.new(pos.x + direction * 0.5, pos.y),
        Vec2.new(direction, 0),
        10.0,
        false
    )

    if hit then
        local targetName = hit.entity:getName() or "unknown"
        log.info("Hit: " .. targetName .. " at distance " .. string.format("%.2f", hit.distance))

        -- Award points for hitting targets
        if targetName == "Target" or targetName == "Enemy" then
            state.score = state.score + 100
            log.info("Score: " .. state.score)
        end
    end
end

-- ============================================================================
-- Camera System
-- ============================================================================

function updateCameraFollow(dt)
    -- This demonstrates how camera follow could work
    -- In practice, you'd need a Camera entity to move
    local pos = self:getPosition()
    local targetCamPos = Vec2.new(
        pos.x + config.cameraOffset.x,
        pos.y + config.cameraOffset.y
    )

    -- You could store this and apply to a camera entity
    -- For now, we just demonstrate the concept
end

-- ============================================================================
-- Visual Feedback
-- ============================================================================

function updateVisuals()
    local color = self:getColor()

    if state.isGrounded then
        -- Grounded: green tint
        color.r = 0.3
        color.g = 0.9
        color.b = 0.3
    else
        -- Airborne: blue tint
        color.r = 0.3
        color.g = 0.5
        color.b = 0.9
    end

    -- Flash red briefly after shooting
    if state.shootTimer > config.shootCooldown * 0.5 then
        color.r = 1.0
        color.g = 0.5
        color.b = 0.2
    end

    color.a = 1.0
    self:setColor(color)
end

-- ============================================================================
-- Collision Callbacks
-- ============================================================================

function onCollisionBegin(other, normal)
    local otherName = other:getName() or "unknown"

    -- Check if we landed on an enemy (stomping)
    if normal.y > 0.7 and otherName == "Enemy" then
        log.info("Stomped an enemy!")
        state.score = state.score + 200
        log.info("Score: " .. state.score)
    end
end

function onCollisionEnd(other)
    -- Called when collision ends
end

function onTriggerEnter(other)
    local otherName = other:getName() or "unknown"

    if otherName == "Coin" then
        state.score = state.score + 50
        log.info("Collected coin! Score: " .. state.score)
    elseif otherName == "Checkpoint" then
        log.info("Checkpoint reached!")
    elseif otherName == "Goal" then
        log.info("Level complete! Final score: " .. state.score)
    elseif otherName == "Hazard" then
        log.warn("Ouch! Hit a hazard!")
    end
end

function onTriggerExit(other)
    local otherName = other:getName() or "unknown"
    log.debug("Left trigger zone: " .. otherName)
end
