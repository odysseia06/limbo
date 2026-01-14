# Limbo Engine - 2D Renderer Guide

This document covers the 2D rendering system in Limbo Engine, including the batched renderer, materials, text rendering, and debug visualization.

## Overview

The 2D rendering system consists of:

- **Renderer2D**: Batched sprite and primitive renderer
- **SpriteMaterial**: Custom shader/uniform system for sprite effects
- **TextRenderer**: Bitmap font text rendering
- **GPUTimer**: GPU-side performance measurement
- **Debug visualization**: Entity bounds, physics shapes, and more

## Renderer2D

The `Renderer2D` class provides efficient batched rendering for 2D games. It automatically batches draw calls to minimize GPU state changes.

### Basic Usage

```cpp
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/render/common/Camera.hpp"

// Initialize (call once at startup)
Renderer2D::init();

// Each frame:
Renderer2D::resetStats();
Renderer2D::beginScene(camera);

// Draw colored quad
Renderer2D::drawQuad({0, 0}, {1, 1}, {1, 0, 0, 1});  // Red quad at origin

// Draw textured quad
Renderer2D::drawQuad({2, 0}, {1, 1}, myTexture);

// Draw rotated quad
Renderer2D::drawRotatedQuad({4, 0}, {1, 1}, glm::radians(45.0f), {0, 1, 0, 1});

// Draw with transform matrix
glm::mat4 transform = entity.getComponent<TransformComponent>().getMatrix();
Renderer2D::drawQuad(transform, {1, 1, 1, 1});

Renderer2D::endScene();

// Shutdown (call once at exit)
Renderer2D::shutdown();
```

### Draw Methods

#### Colored Quads
```cpp
// 2D position (z = 0)
drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

// 3D position
drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

// With transform matrix
drawQuad(const glm::mat4& transform, const glm::vec4& color);
```

#### Textured Quads
```cpp
// Basic textured quad
drawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2D& texture,
         f32 tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

// With custom UV coordinates (for sprite sheets)
drawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2D& texture,
         const glm::vec2& uvMin, const glm::vec2& uvMax,
         const glm::vec4& tintColor = glm::vec4(1.0f));
```

#### Rotated Quads
```cpp
drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                const glm::vec4& color);

drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, f32 rotation,
                const Texture2D& texture, f32 tilingFactor = 1.0f,
                const glm::vec4& tintColor = glm::vec4(1.0f));
```

#### Debug Primitives
```cpp
// Lines
drawLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color);

// Wireframe rectangle
drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
drawRect(const glm::vec3& position, const glm::vec2& size, f32 rotation, const glm::vec4& color);

// Wireframe circle
drawCircle(const glm::vec2& center, f32 radius, const glm::vec4& color, i32 segments = 32);

// Filled circle
drawFilledCircle(const glm::vec2& center, f32 radius, const glm::vec4& color, i32 segments = 32);
```

### Statistics

```cpp
Renderer2D::Statistics stats = Renderer2D::getStats();

// Available stats:
stats.drawCalls;    // Number of GPU draw calls
stats.quadCount;    // Number of quads rendered
stats.lineCount;    // Number of lines rendered
stats.textureBinds; // Number of texture binds
stats.batchCount;   // Number of batches
stats.vertexCount(); // Total vertices (computed)
```

## SpriteMaterial System

The `SpriteMaterial` class allows custom shaders and uniforms for sprite effects.

### Creating a Material

```cpp
#include "limbo/render/2d/SpriteMaterial.hpp"

// Create with default shader
auto material = SpriteMaterial::create();

// Create with custom shader
auto shader = std::make_shared<Shader>();
shader->loadFromFile("assets/shaders/2d/outline.vert", "assets/shaders/2d/outline.frag");
auto material = SpriteMaterial::create(shader);
```

### Setting Properties

```cpp
// Common properties
material->setColor({1, 0, 0, 1});      // Tint color
material->setTexture(myTexture);        // Main texture
material->setTilingFactor(2.0f);        // Texture repeat

// Custom uniforms (for custom shaders)
material->setFloat("u_OutlineWidth", 2.0f);
material->setInt("u_UseEffect", 1);
material->setVector2("u_Offset", {0.5f, 0.5f});
material->setVector3("u_LightDir", {1, 1, 0});
material->setVector4("u_EdgeColor", {1, 0.5f, 0, 1});
```

### Using with ECS

Add a `SpriteMaterialComponent` to entities that need custom materials:

```cpp
#include "limbo/ecs/Components.hpp"

// Create entity with sprite and material
auto entity = world.createEntity("EffectSprite");
entity.addComponent<TransformComponent>(glm::vec3(0, 0, 0));
entity.addComponent<SpriteRendererComponent>(glm::vec4(1, 1, 1, 1));
entity.addComponent<SpriteMaterialComponent>(material);
```

**Note**: Entities with `SpriteMaterialComponent` are rendered separately from the batch, which impacts performance. Use sparingly for special effects.

### SpriteRenderSystem Integration

The `SpriteRenderSystem` automatically handles both batched and material-based sprites:

```cpp
SpriteRenderSystem spriteSystem;
spriteSystem.setCamera(&camera);
spriteSystem.setAssetManager(&assetManager);  // Required for texture lookup
spriteSystem.onAttach(world);

// Each frame:
spriteSystem.update(world, deltaTime);
```

## Example Shaders

Limbo includes example shaders in `assets/shaders/2d/`:

### Outline Effect (`outline.frag`)
Renders a colored outline around sprites.

```cpp
auto material = SpriteMaterial::create(outlineShader);
material->setFloat("u_OutlineWidth", 2.0f);
material->setVector4("u_OutlineColor", {1, 0, 0, 1});  // Red outline
material->setVector2("u_TextureSize", {64, 64});
```

### Dissolve Effect (`dissolve.frag`)
Creates a dissolve/burn effect using procedural noise.

```cpp
auto material = SpriteMaterial::create(dissolveShader);
material->setFloat("u_DissolveAmount", 0.5f);  // 0 = visible, 1 = dissolved
material->setVector4("u_EdgeColor", {1, 0.5f, 0, 1});  // Orange burn edge
material->setFloat("u_EdgeWidth", 0.1f);
material->setFloat("u_NoiseScale", 10.0f);
```

### Flash Effect (`flash.frag`)
Overlays a solid color for damage/hit effects.

```cpp
auto material = SpriteMaterial::create(flashShader);
material->setFloat("u_FlashAmount", 1.0f);  // 0 = normal, 1 = fully white
material->setVector4("u_FlashColor", {1, 1, 1, 1});  // White flash
```

## Text Rendering

### Font Loading

```cpp
#include "limbo/render/2d/Font.hpp"
#include "limbo/assets/FontAsset.hpp"

// Direct loading
auto result = Font::loadFromFile("assets/fonts/roboto.ttf", 32);
if (result.isOk()) {
    Unique<Font> font = std::move(result.value());
}

// Via AssetManager
auto fontAsset = assetManager.load<FontAsset>("fonts/roboto.ttf");
```

### Drawing Text

```cpp
#include "limbo/render/2d/TextRenderer.hpp"

// Initialize (call once)
TextRenderer::init();

// Draw text
TextRenderer::drawText(*font, "Hello World", {100, 100}, 1.0f, {1, 1, 1, 1});

// Measure text
glm::vec2 size = TextRenderer::measureText(*font, "Hello World", 1.0f);
```

### TextRendererComponent

```cpp
auto entity = world.createEntity("Label");
entity.addComponent<TransformComponent>(glm::vec3(0, 0, 0));
entity.addComponent<TextRendererComponent>("Hello World", fontAssetId);

// Configure
auto& text = entity.getComponent<TextRendererComponent>();
text.scale = 2.0f;
text.color = {1, 1, 0, 1};  // Yellow
text.sortingLayer = 10;     // Render on top
```

## GPU Profiling

### GPUTimer Usage

```cpp
#include "limbo/debug/GPUTimer.hpp"

GPUTimer gpuTimer;
gpuTimer.init();

// Each frame:
gpuTimer.beginFrame();

gpuTimer.begin("SceneRender");
// ... render scene ...
gpuTimer.end();

gpuTimer.begin("PostProcess");
// ... post processing ...
gpuTimer.end();

gpuTimer.endFrame();

// Get results (from previous frame due to double-buffering)
f64 sceneMs = gpuTimer.getTimeMs("SceneRender");
f64 totalMs = gpuTimer.getTotalTimeMs();
```

### Scoped Timer

```cpp
{
    ScopedGPUTimer timer(gpuTimer, "RenderSection");
    // ... rendering code ...
}  // automatically calls end()
```

### Integration with Stats Panel

```cpp
#include "limbo/imgui/DebugPanels.hpp"

// Pass GPUTimer to stats panel for display
DebugPanels::showStatsPanel(deltaTime, &gpuTimer);
```

## Debug Visualization

### Entity Bounds

```cpp
#include "limbo/imgui/DebugPanels.hpp"

// Within a Renderer2D scene:
Renderer2D::beginScene(camera);

// Draw your game objects...

// Draw debug bounds (call after regular rendering)
DebugPanels::drawEntityBounds(world,
    true,  // showTransformBounds
    true,  // showColliderBounds
    {0, 1, 0, 0.5f},  // boundsColor (green)
    {0, 0.5f, 1, 0.5f}  // colliderColor (blue)
);

Renderer2D::endScene();
```

### Physics Debug

```cpp
#include "limbo/physics/2d/PhysicsDebug2D.hpp"

PhysicsDebug2D physicsDebug;

// Configure what to show
physicsDebug.setShowStatic(true);
physicsDebug.setShowDynamic(true);
physicsDebug.setShowSensors(true);
physicsDebug.setShowAABBs(false);

// Within a scene:
Renderer2D::beginScene(camera);
physicsDebug.draw(physicsWorld);
Renderer2D::endScene();
```

## Performance Tips

1. **Batch similar sprites**: Sprites with the same texture batch together automatically.

2. **Use SpriteMaterial sparingly**: Custom materials break batching. Use them only for special effects.

3. **Sort by layer**: Use `sortingLayer` and `sortingOrder` in sprite components to control draw order without breaking batches.

4. **Sprite sheets**: Use UV coordinates to render from sprite sheets instead of many small textures.

5. **GPU Timer**: Use `GPUTimer` to identify GPU bottlenecks vs CPU bottlenecks.

6. **Statistics**: Monitor `Renderer2D::getStats()` to track batch efficiency:
   - High `drawCalls` relative to `quadCount` indicates poor batching
   - High `textureBinds` suggests texture atlas optimization needed

## API Reference

### Key Classes

| Class | Header | Purpose |
|-------|--------|---------|
| `Renderer2D` | `limbo/render/2d/Renderer2D.hpp` | Batched 2D rendering |
| `SpriteMaterial` | `limbo/render/2d/SpriteMaterial.hpp` | Custom sprite effects |
| `Font` | `limbo/render/2d/Font.hpp` | Font loading |
| `TextRenderer` | `limbo/render/2d/TextRenderer.hpp` | Text drawing |
| `GPUTimer` | `limbo/debug/GPUTimer.hpp` | GPU timing |
| `PhysicsDebug2D` | `limbo/physics/2d/PhysicsDebug2D.hpp` | Physics visualization |

### Key Components

| Component | Purpose |
|-----------|---------|
| `SpriteRendererComponent` | Basic sprite with color, texture, sorting |
| `SpriteMaterialComponent` | Custom material for shader effects |
| `QuadRendererComponent` | Colored quad primitive |
| `CircleRendererComponent` | Colored circle primitive |
| `TextRendererComponent` | Text display |

### Key Systems

| System | Purpose |
|--------|---------|
| `SpriteRenderSystem` | Renders all sprites with sorting |
| `TextRenderSystem` | Renders all text components |
