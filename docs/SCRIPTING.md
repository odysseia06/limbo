# Limbo Scripting API Reference

Limbo uses Lua for scripting, powered by Sol2. Scripts can be attached to entities via the `ScriptComponent` to define custom behavior.

## Quick Start

1. Create a `.lua` file in `assets/scripts/`
2. Add a `ScriptComponent` to an entity in the Inspector
3. Select your script file using the file picker
4. Enter Play mode to run your script

```lua
-- Example: simple_script.lua

function onStart()
    log.info("Hello from " .. self:getName())
end

function onUpdate(dt)
    -- Called every frame with delta time
end
```

## Lifecycle Callbacks

### onStart()
Called once when the script initializes, after the entity is set up.

```lua
function onStart()
    log.info("Script started on: " .. self:getName())
end
```

### onUpdate(dt)
Called every frame during Play mode.

- `dt` (number): Delta time in seconds since last frame

```lua
function onUpdate(dt)
    local pos = self:getPosition()
    pos.x = pos.x + 5.0 * dt  -- Move 5 units per second
    self:setPosition(pos)
end
```

### onDestroy()
Called when the entity is destroyed or Play mode ends.

```lua
function onDestroy()
    log.info("Goodbye!")
end
```

## Collision Callbacks

### onCollisionBegin(other, normal)
Called when a collision starts between this entity and another.

- `other` (Entity): The other entity involved in the collision
- `normal` (Vec2): Collision normal pointing away from self

```lua
function onCollisionBegin(other, normal)
    log.info("Collided with: " .. other:getName())
    
    -- Check if we landed on top (stomping)
    if normal.y > 0.7 then
        log.info("Landed on something!")
    end
end
```

### onCollisionEnd(other)
Called when a collision ends.

- `other` (Entity): The other entity

```lua
function onCollisionEnd(other)
    log.debug("No longer touching: " .. other:getName())
end
```

### onTriggerEnter(other)
Called when entering a trigger volume.

- `other` (Entity): The trigger entity entered

```lua
function onTriggerEnter(other)
    if other:getName() == "Coin" then
        score = score + 100
    end
end
```

### onTriggerExit(other)
Called when exiting a trigger volume.

- `other` (Entity): The trigger entity exited

```lua
function onTriggerExit(other)
    log.debug("Left zone: " .. other:getName())
end
```

## Hot Reload Callbacks

Scripts are automatically reloaded when saved during Play mode. Use these optional callbacks to preserve state across reloads.

### onBeforeReload()
Called just before the script is reloaded. Return a table of values to preserve.

```lua
function onBeforeReload()
    return {
        score = score,
        health = health,
    }
end
```

### onAfterReload(preserved)
Called after reload with the preserved data.

```lua
function onAfterReload(preserved)
    if preserved then
        score = preserved.score or 0
        health = preserved.health or 100
    end
end
```

## Global Objects

### self (Entity)
Reference to the entity this script is attached to.

```lua
local name = self:getName()
local pos = self:getPosition()
```

### Time
Global time information.

| Field | Type | Description |
|-------|------|-------------|
| `Time.deltaTime` | number | Time since last frame (seconds) |
| `Time.totalTime` | number | Time since Play mode started (seconds) |

```lua
function onUpdate(dt)
    -- Both are equivalent
    local dt1 = dt
    local dt2 = Time.deltaTime
end
```

## Entity API

Methods available on `self` or any `Entity` object.

### Identification

| Method | Returns | Description |
|--------|---------|-------------|
| `getName()` | string | Entity name |
| `setName(name)` | void | Set entity name |
| `getId()` | number | Unique entity ID |
| `isValid()` | bool | Check if entity still exists |

### Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `getPosition()` | Vec3 | World position |
| `setPosition(pos)` | void | Set position (Vec3 or Vec2) |
| `getRotation()` | Vec3 | Rotation in radians |
| `setRotation(rot)` | void | Set rotation (Vec3 or number for 2D) |
| `getScale()` | Vec3 | Scale |
| `setScale(scale)` | void | Set scale (Vec3 or Vec2) |

```lua
-- Move entity
local pos = self:getPosition()
pos.x = pos.x + 1.0
self:setPosition(pos)

-- Rotate 90 degrees in 2D
self:setRotation(math.rad(90))
```

### Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `getColor()` | Vec4 | Sprite color (RGBA) |
| `setColor(color)` | void | Set sprite color |

```lua
-- Flash red
self:setColor(Vec4.new(1, 0, 0, 1))

-- Fade out
local c = self:getColor()
c.a = c.a - 0.1
self:setColor(c)
```

### Physics

| Method | Returns | Description |
|--------|---------|-------------|
| `getVelocity()` | Vec2 | Linear velocity |
| `setVelocity(vel)` | void | Set linear velocity |
| `applyForce(force)` | void | Apply continuous force |
| `applyImpulse(impulse)` | void | Apply instant impulse |

```lua
-- Horizontal movement
local vel = self:getVelocity()
self:setVelocity(Vec2.new(5.0, vel.y))

-- Jump
self:applyImpulse(Vec2.new(0, 10))
```

## Input API

### Keyboard

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `Input.isKeyDown(key)` | Key | bool | Key currently held |
| `Input.isKeyPressed(key)` | Key | bool | Key just pressed this frame |
| `Input.isKeyReleased(key)` | Key | bool | Key just released this frame |

```lua
if Input.isKeyDown(Key.A) then
    -- Move left while held
end

if Input.isKeyPressed(Key.Space) then
    -- Jump once when pressed
end
```

### Mouse

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `Input.isMouseButtonDown(button)` | number | bool | Button held (0=left, 1=right, 2=middle) |
| `Input.isMouseButtonPressed(button)` | number | bool | Button just pressed |
| `Input.getMousePosition()` | - | Vec2 | Screen position |

```lua
if Input.isMouseButtonPressed(0) then
    local mousePos = Input.getMousePosition()
    log.info("Clicked at: " .. mousePos.x .. ", " .. mousePos.y)
end
```

### Key Constants

Common key constants available via the `Key` table:

- Letters: `Key.A` through `Key.Z`
- Numbers: `Key.Num0` through `Key.Num9`
- Arrows: `Key.Left`, `Key.Right`, `Key.Up`, `Key.Down`
- Modifiers: `Key.LeftShift`, `Key.RightShift`, `Key.LeftControl`, `Key.RightControl`
- Special: `Key.Space`, `Key.Enter`, `Key.Escape`, `Key.Tab`, `Key.Backspace`
- Function: `Key.F1` through `Key.F12`

## Physics API

### Physics.raycast(origin, direction, distance, includeTriggers)

Cast a ray and return hit information.

| Parameter | Type | Description |
|-----------|------|-------------|
| `origin` | Vec2 | Ray start position |
| `direction` | Vec2 | Normalized direction |
| `distance` | number | Maximum distance |
| `includeTriggers` | bool | Include trigger colliders |

Returns `nil` if no hit, or a table:
```lua
{
    entity = Entity,  -- Hit entity
    point = Vec2,     -- Hit point
    normal = Vec2,    -- Surface normal
    distance = number -- Distance to hit
}
```

```lua
local hit = Physics.raycast(
    Vec2.new(0, 0),
    Vec2.new(1, 0),
    10.0,
    false
)

if hit then
    log.info("Hit: " .. hit.entity:getName())
    log.info("Distance: " .. hit.distance)
end
```

### Physics.overlap(position, radius, includeTriggers)

Find all entities overlapping a circle.

| Parameter | Type | Description |
|-----------|------|-------------|
| `position` | Vec2 | Circle center |
| `radius` | number | Circle radius |
| `includeTriggers` | bool | Include trigger colliders |

Returns an array of entities.

```lua
local nearby = Physics.overlap(self:getPosition(), 5.0, false)
for i, entity in ipairs(nearby) do
    log.info("Nearby: " .. entity:getName())
end
```

## Math Types

### Vec2

2D vector with `x`, `y` components.

```lua
local v = Vec2.new(1, 2)
v.x = v.x + 1
local length = math.sqrt(v.x * v.x + v.y * v.y)
```

### Vec3

3D vector with `x`, `y`, `z` components.

```lua
local v = Vec3.new(1, 2, 3)
local pos = self:getPosition()  -- Returns Vec3
```

### Vec4

4D vector with `x`, `y`, `z`, `w` components. Used for colors (RGBA).

```lua
local color = Vec4.new(1, 0, 0, 1)  -- Red, fully opaque
self:setColor(color)
```

## Logging API

Output messages to the Console panel.

| Method | Description |
|--------|-------------|
| `log.debug(msg)` | Debug message (gray) |
| `log.info(msg)` | Info message (cyan) |
| `log.warn(msg)` | Warning message (yellow) |
| `log.error(msg)` | Error message (red) |

```lua
log.info("Player health: " .. health)
log.warn("Low health!")
log.error("Something went wrong!")
```

## Best Practices

### 1. Use Local Variables
Keep script state in local variables for better performance and encapsulation.

```lua
local speed = 5.0
local health = 100

function onUpdate(dt)
    -- Use local variables
end
```

### 2. Cache Frequently Used Values
Avoid repeated lookups in hot paths.

```lua
local pos  -- Cache position

function onStart()
    pos = self:getPosition()
end

function onUpdate(dt)
    pos.x = pos.x + speed * dt
    self:setPosition(pos)
end
```

### 3. Handle Hot Reload Gracefully
Use reload callbacks to preserve important state.

```lua
local score = 0

function onBeforeReload()
    return { score = score }
end

function onAfterReload(data)
    score = data and data.score or 0
end
```

### 4. Validate Entity References
Entities can be destroyed at any time.

```lua
function onTriggerEnter(other)
    if other:isValid() then
        local name = other:getName()
    end
end
```

### 5. Use Ground Checks for Platformers
Raycast downward for reliable ground detection.

```lua
local function isGrounded()
    local pos = self:getPosition()
    local hit = Physics.raycast(
        Vec2.new(pos.x, pos.y),
        Vec2.new(0, -1),
        0.55,  -- Just past the collider edge
        false
    )
    return hit ~= nil
end
```

## Example: Complete Platformer

```lua
-- platformer.lua

local moveSpeed = 5.0
local jumpForce = 10.0
local isGrounded = false

function onStart()
    log.info("Platformer started!")
end

function onUpdate(dt)
    -- Ground check
    local pos = self:getPosition()
    local hit = Physics.raycast(
        Vec2.new(pos.x, pos.y),
        Vec2.new(0, -1),
        0.55,
        false
    )
    isGrounded = (hit ~= nil)
    
    -- Movement
    local vel = self:getVelocity()
    local moveDir = 0
    
    if Input.isKeyDown(Key.A) then moveDir = -1 end
    if Input.isKeyDown(Key.D) then moveDir = 1 end
    
    self:setVelocity(Vec2.new(moveDir * moveSpeed, vel.y))
    
    -- Jumping
    if isGrounded and Input.isKeyPressed(Key.Space) then
        self:applyImpulse(Vec2.new(0, jumpForce))
    end
    
    -- Visual feedback
    local color = self:getColor()
    color.g = isGrounded and 1.0 or 0.5
    self:setColor(color)
end

function onCollisionBegin(other, normal)
    if normal.y > 0.7 then
        log.debug("Landed!")
    end
end

function onTriggerEnter(other)
    if other:getName() == "Coin" then
        log.info("Got coin!")
    end
end
```

## Troubleshooting

### Script Not Running
- Ensure the script path is set in the ScriptComponent
- Check the Console panel for error messages
- Verify the script file exists at the specified path

### Errors Show in Console
- Check the line number in the error message
- Common issues: nil access, type mismatches, syntax errors
- The Inspector shows error details with line numbers

### Hot Reload Not Working
- Save the file (Ctrl+S in your editor)
- Hot reload only works during Play mode
- Check Console for "Script reloaded" message

### Physics Not Responding
- Ensure entity has Rigidbody2D component
- Check that body type is Dynamic (not Static)
- Verify collider component is attached
