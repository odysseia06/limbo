# Engine Lifecycle

This document describes the lifecycle of the Limbo Engine, including initialization, the main game loop, and shutdown procedures.

## Overview

The engine follows a structured lifecycle:

1. **Initialization** - Platform, window, input, time, and systems setup
2. **Main Loop** - Fixed timestep updates, variable updates, and rendering
3. **Shutdown** - Clean teardown in reverse order

## Application Lifecycle

### Initialization Order

```cpp
Application::init(config)
├── platform::init()
├── Window::create()
├── Input::init()
├── InputManager::init()
├── Time::init()
├── onInit()              // User override
└── SystemManager::init() // Calls System::onAttach() for each system
```

### Main Loop

The engine uses a **fixed timestep** pattern for deterministic physics while allowing variable frame rates for rendering:

```cpp
while (running) {
    Time::beginFrame()           // Calculate delta time, update smoothing
    
    Input::update()              // Update raw input state
    InputManager::update()       // Update action/axis mappings
    
    window.pollEvents()          // Process OS events
    
    // Fixed timestep loop (physics, deterministic logic)
    while (Time::shouldFixedUpdate()) {
        SystemManager::fixedUpdate(fixedDeltaTime)
        onFixedUpdate(fixedDeltaTime)    // User override
    }
    
    // Variable timestep (animations, AI, etc.)
    SystemManager::update(deltaTime)
    onUpdate(deltaTime)          // User override
    
    // Render with interpolation
    onRender(interpolationAlpha) // User override
    
    window.swapBuffers()
}
```

### Shutdown Order

```cpp
Application::shutdown()
├── SystemManager::shutdown()    // Calls System::onDetach() in reverse order
├── onShutdown()                 // User override
├── World::clear()               // Destroy all entities
├── InputManager::shutdown()
├── Window destruction
└── platform::shutdown()
```

## Time System

### Configuration

```cpp
struct TimeConfig {
    f32 fixedDeltaTime = 1.0f / 60.0f;  // 60 Hz physics
    f32 maxDeltaTime = 0.25f;           // Prevent spiral of death
    u32 smoothingFrames = 11;           // Delta smoothing window
    bool enableSmoothing = true;         // Enable/disable smoothing
    u32 maxFixedUpdatesPerFrame = 8;    // Prevent lockup
};
```

### Fixed Timestep Pattern

The fixed timestep pattern ensures deterministic behavior regardless of frame rate:

```cpp
// Called in main loop
Time::beginFrame();  // Adds delta to accumulator

// Consume accumulated time in fixed chunks
while (Time::shouldFixedUpdate()) {
    // Physics, networking, replays
    physics.step(Time::getFixedDeltaTime());
}

// Smooth rendering between physics states
f32 alpha = Time::getInterpolationAlpha();
renderInterpolated(previousState, currentState, alpha);
```

### Time Queries

| Method | Description |
|--------|-------------|
| `Time::getDeltaTime()` | Smoothed delta time (affected by time scale) |
| `Time::getRawDeltaTime()` | Unsmoothed delta time |
| `Time::getFixedDeltaTime()` | Fixed timestep for physics |
| `Time::getTimeSinceStart()` | Total time since init |
| `Time::getFrameCount()` | Total frames rendered |
| `Time::getFPS()` | Averaged frames per second |
| `Time::getInterpolationAlpha()` | Blend factor [0,1] for rendering |

### Time Scale

```cpp
Time::setTimeScale(0.5f);  // Half speed (slow motion)
Time::setTimeScale(2.0f);  // Double speed (fast forward)
Time::setTimeScale(0.0f);  // Paused (deltaTime = 0)
```

## Input System

### Low-Level Input (Input class)

Direct polling of keyboard, mouse, and modifiers:

```cpp
if (Input::isKeyPressed(Key::Space)) {
    jump();
}

glm::vec2 mouseDelta = Input::getMouseDelta();
f32 scroll = Input::getScrollY();
```

### High-Level Input (InputManager)

Action and axis mappings with context switching:

```cpp
// Query actions
if (InputManager::isActionPressed("Jump")) {
    player.jump();
}

// Query axes
f32 horizontal = InputManager::getAxisValue("MoveHorizontal");
f32 vertical = InputManager::getAxisValue("MoveVertical");
```

### Input Contexts

Switch between game, editor, menu, and cutscene contexts:

```cpp
// Switch context
InputManager::setContext(InputContext::Menu);

// Push/pop for temporary changes
InputManager::pushContext(InputContext::Cutscene);
// ... cutscene plays ...
InputManager::popContext();  // Returns to previous context
```

### JSON Configuration

```json
{
  "actions": [
    {
      "name": "Jump",
      "context": "Game",
      "bindings": [
        { "type": "key", "key": "Space" },
        { "type": "mouse", "button": "Left", "modifiers": ["Control"] }
      ]
    }
  ],
  "axes": [
    {
      "name": "MoveHorizontal",
      "context": "Game",
      "deadzone": 0.1,
      "sensitivity": 1.0,
      "bindings": [
        { "type": "keys", "positive": "D", "negative": "A" }
      ]
    },
    {
      "name": "LookVertical",
      "context": "Game",
      "bindings": [
        { "type": "mouse", "axis": "Y", "sensitivity": 0.5, "inverted": true }
      ]
    }
  ]
}
```

### Rebinding

```cpp
// Start listening for new binding
InputManager::startRebinding("Jump", 0);

// Check status
if (InputManager::isRebinding()) {
    // Show "Press any key..." UI
}

// Cancel if needed
InputManager::cancelRebinding();
```

## System Lifecycle

Systems are managed components that operate on entities:

```cpp
class PhysicsSystem : public System {
public:
    void onAttach(World& world) override {
        // Initialize physics world
    }
    
    void fixedUpdate(World& world, f32 fixedDeltaTime) override {
        // Step physics simulation
    }
    
    void update(World& world, f32 deltaTime) override {
        // Interpolate visual positions
    }
    
    void onDetach(World& world) override {
        // Cleanup physics world
    }
};
```

### System Priority

Lower priority values run first:

```cpp
auto* physics = getSystems().addSystem<PhysicsSystem>();
physics->setPriority(-100);  // Runs early

auto* rendering = getSystems().addSystem<RenderSystem>();
rendering->setPriority(100);  // Runs late
```

### Fixed vs Variable Update

| Method | When to Use |
|--------|-------------|
| `fixedUpdate()` | Physics, networking, replays, deterministic logic |
| `update()` | Animations, AI, camera, visual effects |

## Best Practices

### Frame Rate Independence

Always multiply by delta time for movement:

```cpp
// Wrong - speed varies with frame rate
position += velocity;

// Correct - consistent speed
position += velocity * deltaTime;
```

### Physics Interpolation

Use the interpolation alpha for smooth visuals:

```cpp
void onRender(f32 alpha) {
    // Interpolate between physics states
    glm::vec3 renderPos = glm::mix(previousPos, currentPos, alpha);
    drawSprite(renderPos);
}
```

### Avoid Spiral of Death

The engine automatically caps `maxDeltaTime` and `maxFixedUpdatesPerFrame` to prevent the game loop from locking up when frames take too long.

### Input in Fixed Update

For physics-based input (like applying forces), sample input in the fixed update:

```cpp
void fixedUpdate(World& world, f32 fixedDeltaTime) {
    if (InputManager::isActionDown("Jump")) {
        applyJumpForce();
    }
}
```

For one-shot actions (like UI interaction), use variable update:

```cpp
void update(World& world, f32 deltaTime) {
    if (InputManager::isActionPressed("Pause")) {
        togglePauseMenu();
    }
}
```
