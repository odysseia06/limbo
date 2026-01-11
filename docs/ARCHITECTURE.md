# Limbo Engine Architecture

This document describes the high-level architecture of the Limbo Engine.

## Overview

Limbo is a 2D game engine built with modern C++20, designed around an Entity Component System (ECS) architecture. The engine is structured as a static library that applications link against.

```
┌─────────────────────────────────────────────────────────────┐
│                      Application                             │
│  (Sandbox, Editor, Your Game)                               │
├─────────────────────────────────────────────────────────────┤
│                     Limbo Engine                             │
├──────────┬──────────┬──────────┬──────────┬─────────────────┤
│ Runtime  │ Render   │   ECS    │ Physics  │     Audio       │
├──────────┼──────────┼──────────┼──────────┼─────────────────┤
│ Platform │ Assets   │ Scene    │ Scripting│   Particles     │
├──────────┼──────────┼──────────┼──────────┼─────────────────┤
│ Tilemap  │   UI     │  ImGui   │  Debug   │     Core        │
├──────────┴──────────┴──────────┴──────────┴─────────────────┤
│                    Third-Party Libraries                     │
│  GLFW | OpenGL | EnTT | Box2D | Lua | miniaudio | ImGui     │
└─────────────────────────────────────────────────────────────┘
```

## Core Layers

### 1. Core (`limbo/core/`)

The foundation layer providing basic types and utilities:

- **Types.hpp**: Type aliases (`f32`, `u32`, `String`, `Shared<T>`, `Unique<T>`)
- **Base.hpp**: Platform detection, API export macros
- **Result.hpp**: Error handling with `Result<T, E>` type
- **Memory.hpp**: Memory utilities and allocators

### 2. Platform (`limbo/platform/`)

Platform abstraction for OS-specific functionality:

- **Window.hpp**: GLFW window wrapper with event handling
- **Input.hpp**: Keyboard, mouse, and gamepad input polling

### 3. Render (`limbo/render/`)

OpenGL-based 2D rendering system:

- **RenderContext.hpp**: OpenGL context management
- **Shader.hpp**: GLSL shader compilation and uniforms
- **Texture.hpp**: 2D texture loading and binding
- **Buffer.hpp**: Vertex and index buffers
- **VertexArray.hpp**: VAO management
- **Camera.hpp**: Orthographic camera with projection
- **Renderer2D.hpp**: Batched sprite rendering

The Renderer2D uses a batching system to minimize draw calls:

```
┌─────────────────────────────────────────┐
│           Renderer2D::beginScene()       │
├─────────────────────────────────────────┤
│  drawQuad() → Batch vertices             │
│  drawQuad() → Batch vertices             │
│  drawQuad() → Batch vertices             │
│  ... (up to batch limit)                 │
├─────────────────────────────────────────┤
│  Flush batch when full or texture change │
├─────────────────────────────────────────┤
│           Renderer2D::endScene()         │
└─────────────────────────────────────────┘
```

### 4. ECS (`limbo/ecs/`)

Entity Component System using EnTT:

- **World.hpp**: Entity registry wrapper, system management
- **Entity.hpp**: Entity handle with component access
- **Components.hpp**: Built-in components (Name, Transform, Sprite, etc.)
- **System.hpp**: Base class for game systems

#### Component Types

| Component | Purpose |
|-----------|---------|
| `NameComponent` | Entity name/tag |
| `TransformComponent` | Position, rotation, scale |
| `SpriteRendererComponent` | Sprite color and texture |
| `Rigidbody2DComponent` | Physics body configuration |
| `BoxCollider2DComponent` | Box collision shape |
| `CircleCollider2DComponent` | Circle collision shape |
| `AnimatorComponent` | Sprite animation state |
| `ScriptComponent` | Lua script attachment |
| `AudioSourceComponent` | Audio playback |
| `ParticleEmitterComponent` | Particle emission |
| `TilemapComponent` | Tilemap reference |
| `UICanvasComponent` | UI widget container |

#### System Execution Order

Systems run in priority order during `World::update()`:

```
1. ScriptSystem (priority: 5)     - Run Lua scripts
2. MovementSystem (user-defined)  - Custom game logic
3. AudioSystem (priority: 50)     - Audio updates
4. TilemapRenderSystem (85)       - Tilemap culling
5. AnimationSystem (priority: 90) - Animation frame updates
6. ParticleRenderSystem (95)      - Particle simulation
7. PhysicsSystem (priority: 100)  - Box2D step
8. UISystem (priority: 200)       - UI input/rendering
```

### 5. Physics (`limbo/physics/`)

Box2D 2.4 integration:

- **Physics2D.hpp**: Physics world wrapper
- **PhysicsComponents.hpp**: Rigidbody and collider components
- **PhysicsSystem.hpp**: Syncs transforms with physics bodies

### 6. Audio (`limbo/audio/`)

miniaudio-based audio system:

- **AudioEngine.hpp**: Audio device and mixing
- **AudioClip.hpp**: Audio data container
- **AudioSource.hpp**: Playback control
- **AudioSystem.hpp**: ECS integration

### 7. Scripting (`limbo/scripting/`)

Lua scripting via sol2:

- **ScriptEngine.hpp**: Lua state management, API binding
- **ScriptComponent.hpp**: Script attachment
- **ScriptSystem.hpp**: Script execution

Lua API provides access to:
- Entity transforms
- Input polling
- Component queries
- Debug logging

### 8. Animation (`limbo/animation/`)

Sprite sheet animation:

- **SpriteSheet.hpp**: Texture atlas with frame regions
- **AnimationClip.hpp**: Frame sequence with timing
- **AnimatorComponent.hpp**: Animation state machine
- **AnimationSystem.hpp**: Frame advancement

### 9. Particles (`limbo/particles/`)

Pooled particle system:

- **ParticleSystem.hpp**: Particle pool and emitter
- **ParticleComponents.hpp**: Emitter configuration
- **ParticleRenderSystem.hpp**: Update and render

### 10. Tilemap (`limbo/tilemap/`)

Layer-based tilemaps:

- **Tileset.hpp**: Tile atlas with properties
- **Tilemap.hpp**: 2D tile grid with layers
- **TilemapComponent.hpp**: ECS attachment
- **TilemapRenderer.hpp**: Frustum-culled rendering

### 11. UI (`limbo/ui/`)

In-game UI widgets:

- **Widget.hpp**: Base class with anchoring, styling
- **Widgets.hpp**: Panel, Label, Button, ProgressBar, Image
- **UICanvas.hpp**: Widget container and input routing
- **UISystem.hpp**: Screen-space rendering

### 12. Assets (`limbo/assets/`)

Asset loading and management:

- **AssetManager.hpp**: Asset registry with hot-reload
- **Asset.hpp**: Base asset class with states
- **TextureAsset.hpp**: Texture loading
- **ShaderAsset.hpp**: Shader compilation

### 13. Scene (`limbo/scene/`)

Scene serialization:

- **SceneSerializer.hpp**: JSON save/load for entities

### 14. Runtime (`limbo/runtime/`)

Application framework:

- **Application.hpp**: Main game loop, system management
- **ApplicationConfig.hpp**: Startup configuration

## Data Flow

### Frame Loop

```
┌─────────────────────────────────────────────────────────────┐
│                    Application::run()                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  1. Poll Input (GLFW events → Input state)                  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  2. onUpdate(deltaTime)                                      │
│     - User game logic                                        │
│     - Systems update (physics, audio, animation, etc.)       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  3. onRender()                                               │
│     - Clear screen                                           │
│     - Renderer2D::beginScene(camera)                         │
│     - Draw sprites, tilemaps, particles                      │
│     - Renderer2D::endScene()                                 │
│     - UI rendering                                           │
│     - ImGui rendering                                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  4. Swap Buffers (present frame)                            │
└─────────────────────────────────────────────────────────────┘
```

### Rendering Pipeline

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Camera     │───▶│  View-Proj   │───▶│   Shader     │
│  (position,  │    │   Matrix     │    │  (uniforms)  │
│   zoom)      │    │              │    │              │
└──────────────┘    └──────────────┘    └──────────────┘
                                               │
                                               ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Sprites    │───▶│   Vertex     │───▶│    Draw      │
│  (position,  │    │   Buffer     │    │    Call      │
│   color, UV) │    │  (batched)   │    │              │
└──────────────┘    └──────────────┘    └──────────────┘
```

## Memory Management

The engine uses smart pointers consistently:

- `Unique<T>` (`std::unique_ptr<T>`): Exclusive ownership
- `Shared<T>` (`std::shared_ptr<T>`): Shared ownership

EnTT manages component storage internally with contiguous arrays for cache efficiency.

## Threading Model

The engine keeps a dedicated main thread for all frame-critical work, while background tasks can be scheduled through the `ThreadPool`. The main thread handles:
- Window events
- Game logic
- Rendering
- Audio mixing (via miniaudio's internal thread)
- Processing the `MainThreadQueue` for jobs that must run on the main thread

Worker threads are for background tasks (including file I/O and data processing). Do not issue OpenGL or ImGui calls from worker threads; instead, enqueue main-thread work via `MainThreadQueue` so the render/UI thread performs those actions.

## Extending the Engine

### Adding a New Component

1. Define the component struct in a header:
```cpp
struct MyComponent {
    float value = 0.0f;
};
```

2. Use it with entities:
```cpp
entity.addComponent<MyComponent>();
entity.getComponent<MyComponent>().value = 42.0f;
```

### Adding a New System

1. Inherit from `System`:
```cpp
class MySystem : public limbo::System {
public:
    void update(World& world, f32 deltaTime) override {
        world.each<MyComponent>([](auto, MyComponent& comp) {
            // Process component
        });
    }
};
```

2. Register with the world:
```cpp
getSystems().addSystem<MySystem>();
```

### Adding Asset Types

1. Inherit from `Asset`:
```cpp
class MyAsset : public limbo::Asset {
public:
    bool load(const std::filesystem::path& path) override;
    void unload() override;
};
```

2. Register with AssetManager:
```cpp
assetManager.load<MyAsset>("path/to/asset");
```
