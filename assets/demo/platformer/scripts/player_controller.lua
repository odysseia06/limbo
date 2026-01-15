-- Player Controller Script
-- A simple platformer player with movement and jump

-- Movement settings
local moveSpeed = 5.0
local jumpForce = 10.0

-- State
local spaceWasDown = false
local groundEntities = {} -- Track which entities we're standing on

function onUpdate(dt)
    local rb = self:getRigidbody()
    if not rb then return end

    -- Get horizontal input
    local vx = 0
    if Input.isKeyDown("A") or Input.isKeyDown("Left") then
        vx = -moveSpeed
    end
    if Input.isKeyDown("D") or Input.isKeyDown("Right") then
        vx = moveSpeed
    end

    -- Apply horizontal velocity, keep vertical
    local vy = rb:getVelocityY()
    rb:setVelocity(vx, vy)

    -- Manual "just pressed" detection for Space
    local spaceDown = Input.isKeyDown("Space")
    local spaceJustPressed = spaceDown and not spaceWasDown
    spaceWasDown = spaceDown

    -- Ground check: we're grounded if we're standing on any entity
    local isGrounded = next(groundEntities) ~= nil

    -- Jump when grounded and space just pressed
    if isGrounded and spaceJustPressed then
        rb:setVelocityY(jumpForce)
    end
end

function onCollisionBegin(other, normal)
    -- Check if collision is from below (we're standing on something)
    -- normal points from self to other, so if other is below us, normal.y < -0.5
    if normal.y < -0.5 then
        -- Track this entity as a ground contact
        local otherId = other:getId()
        groundEntities[otherId] = true
    end
end

function onCollisionEnd(other)
    -- Remove this entity from ground contacts if it was one
    local otherId = other:getId()
    groundEntities[otherId] = nil
end
