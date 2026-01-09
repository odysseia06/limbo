# Physics System (2D)

Limbo's 2D physics system is built on Box2D v2.4.2 and provides rigid body dynamics, collision detection, and physics queries.

## Architecture

```
Physics2D          - Low-level Box2D wrapper (world management, queries)
PhysicsSystem2D    - ECS system (body creation, fixed timestep, interpolation)
ContactListener2D  - Collision event buffering and dispatch
PhysicsDebug2D     - Debug visualization from Box2D world
```

## Components

### Rigidbody2DComponent

Defines a physics body for an entity.

```cpp
struct Rigidbody2DComponent {
    BodyType type = BodyType::Dynamic;  // Static, Kinematic, Dynamic
    f32 gravityScale = 1.0f;
    bool fixedRotation = false;
    glm::vec2 linearVelocity{0.0f};
    f32 angularVelocity = 0.0f;
    f32 linearDamping = 0.0f;
    f32 angularDamping = 0.01f;
};
```

**Body Types:**
- `Static` - Does not move, infinite mass (platforms, walls)
- `Kinematic` - Moves via velocity, not affected by forces (moving platforms)
- `Dynamic` - Fully simulated, affected by forces and collisions

### BoxCollider2DComponent

Box-shaped collider.

```cpp
struct BoxCollider2DComponent {
    glm::vec2 size{0.5f, 0.5f};     // Half-extents
    glm::vec2 offset{0.0f, 0.0f};   // Offset from entity center
    f32 density = 1.0f;
    f32 friction = 0.3f;
    f32 restitution = 0.0f;         // Bounciness (0-1)
    bool isTrigger = false;         // Sensor (no physical response)
};
```

### CircleCollider2DComponent

Circle-shaped collider.

```cpp
struct CircleCollider2DComponent {
    f32 radius = 0.5f;
    glm::vec2 offset{0.0f, 0.0f};
    f32 density = 1.0f;
    f32 friction = 0.3f;
    f32 restitution = 0.0f;
    bool isTrigger = false;
};
```

## Fixed Timestep & Interpolation

Physics runs at a fixed rate (default 60Hz) for determinism. Render transforms are interpolated between physics states for smooth visuals.

**Key principle:** Interpolated render pose NEVER affects simulation.

```cpp
PhysicsSystem2D& physicsSystem = ...;

// Configure fixed timestep (default: 1/60)
physicsSystem.setFixedTimestep(1.0f / 60.0f);

// Prevent spiral-of-death (default: 8)
physicsSystem.setMaxFixedUpdatesPerFrame(8);

// Enable/disable interpolation
physicsSystem.setInterpolationEnabled(true);
```

When the frame rate drops below physics rate, accumulated updates are clamped and a warning is logged.

## Physics Queries

All queries support an `includeTriggers` flag to include/exclude sensor fixtures.

### Raycast

Cast a ray and get the first hit:

```cpp
// C++
RaycastHit2D hit = physics.raycast(origin, direction, maxDistance, includeTriggers);
if (hit.hit) {
    // hit.point, hit.normal, hit.distance, hit.body, hit.fixture
}
```

```lua
-- Lua
local hit = Physics.raycast(Vec2.new(0, 0), Vec2.new(1, 0), 10.0, false)
if hit then
    local entity = hit:getEntity()
    print("Hit at distance: " .. hit.distance)
end
```

### RaycastAll

Get all hits sorted by distance:

```cpp
// C++
std::vector<RaycastHit2D> hits = physics.raycastAll(origin, direction, maxDistance);
```

```lua
-- Lua
local hits = Physics.raycastAll(origin, direction, maxDistance, false)
for i, hit in ipairs(hits) do
    print("Hit #" .. i .. " at " .. hit.distance)
end
```

### Overlap Queries

Find bodies overlapping a shape. Uses narrow-phase testing (not just AABB).

```cpp
// C++ - Circle overlap
std::vector<b2Body*> bodies = physics.overlapCircle(center, radius, includeTriggers);

// C++ - Box overlap  
std::vector<b2Body*> bodies = physics.overlapBox(center, halfExtents, includeTriggers);
```

```lua
-- Lua - Returns entities
local entities = Physics.overlapCircle(Vec2.new(0, 0), 5.0, false)
for _, entity in ipairs(entities) do
    print("Found: " .. entity:getName())
end

local entities = Physics.overlapBox(Vec2.new(0, 0), Vec2.new(2, 1), false)
```

## Collision Events

Events are buffered during `Step()` and dispatched after, making it safe to modify the world in callbacks.

### Self-Relative Callbacks

Collision normals are "self-relative" - the normal points from the receiving entity toward the other entity.

```lua
-- In Lua script attached to an entity
function onCollisionBegin(other, normal)
    -- normal points from self toward other
    if normal.y > 0.7 then
        print("Landed on something!")  -- We're on top
    end
    if normal.y < -0.7 then
        print("Bumped our head!")      -- Something above us
    end
end

function onCollisionEnd(other)
    print("No longer touching: " .. other:getName())
end
```

### Trigger Events

Triggers (sensors) use separate callbacks:

```lua
function onTriggerEnter(other)
    if other:getName() == "Coin" then
        print("Collected coin!")
        other:destroy()
    end
end

function onTriggerExit(other)
    print("Left trigger zone")
end
```

### C++ Collision Callback

```cpp
physicsSystem.setCollisionCallback([](const CollisionEvent2D& event, CollisionEventType type) {
    // event.self - this entity
    // event.other - the other entity
    // event.normal - points from self toward other
    // event.contactPoint - world-space contact point
    // event.isTrigger - true if sensor collision
    // type - Begin or End
});
```

## Entity Physics Methods (Lua)

```lua
-- Velocity
local vel = entity:getVelocity()      -- Returns Vec2
entity:setVelocity(Vec2.new(5, 0))

local angVel = entity:getAngularVelocity()  -- Returns float
entity:setAngularVelocity(1.0)

-- Forces (applied continuously)
entity:applyForce(Vec2.new(100, 0))

-- Impulses (instantaneous, like jumps)
entity:applyImpulse(Vec2.new(0, 10))

-- Torque
entity:applyTorque(5.0)
```

## Debug Visualization

Draw physics shapes from the actual Box2D world state:

```cpp
PhysicsDebug2D debugDraw;

// Configure what to show
debugDraw.setDrawStaticBodies(true);
debugDraw.setDrawDynamicBodies(true);
debugDraw.setDrawSensors(true);
debugDraw.setDrawAABBs(false);
debugDraw.setDrawCenterOfMass(false);

// Customize colors
debugDraw.setDynamicBodyColor({0, 1, 0, 1});  // Green
debugDraw.setSensorColor({1, 1, 0, 0.5f});    // Yellow, semi-transparent

// Draw during render pass
debugDraw.draw(physics2D);
```

## Platformer Patterns

### Ground Check

Use a short raycast from the player's feet:

```lua
local pos = self:getPosition()
local groundHit = Physics.raycast(
    Vec2.new(pos.x, pos.y),
    Vec2.new(0, -1),      -- Down
    0.55,                  -- Slightly more than half player height
    false                  -- Exclude triggers
)
local isGrounded = (groundHit ~= nil)
```

### Moving Platforms

Use kinematic bodies with velocity (not `SetTransform`):

```cpp
// Set velocity for smooth movement
body->SetLinearVelocity(b2Vec2(platformSpeed, 0));

// DON'T use SetTransform for moving platforms - it teleports and breaks collisions
```

### Stomp Detection

Check the self-relative normal in collision callbacks:

```lua
function onCollisionBegin(other, normal)
    if normal.y > 0.7 then
        -- We landed on top of 'other'
        if other:getName() == "Enemy" then
            print("Stomped enemy!")
            other:destroy()
        end
    end
end
```

## Best Practices

1. **Never write interpolated state back to physics** - The render transform is for display only
2. **Use sensors for triggers** - Set `isTrigger = true` on colliders that shouldn't physically block
3. **Buffer expensive operations** - Collision callbacks run after Step, but avoid heavy work
4. **Use kinematic velocity for platforms** - Not SetTransform
5. **Raycast for ground checks** - More reliable than collision-based detection
6. **Watch for spiral-of-death** - If you see clamping warnings, reduce physics complexity or fixed timestep
