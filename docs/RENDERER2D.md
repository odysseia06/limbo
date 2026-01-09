# Renderer2D - 2D Rendering System

The Renderer2D is a high-performance batched 2D rendering system designed for sprite-based games. It efficiently renders thousands of quads per frame by batching draw calls and minimizing state changes.

## Architecture Overview

### Batching System

The renderer uses a batched approach to minimize draw calls:

- **Quad Batching**: Up to 10,000 quads per batch
- **Texture Slots**: Up to 32 textures per batch (GPU-dependent)
- **Line Batching**: Up to 10,000 lines per batch (separate batch)

When a batch fills up (too many quads or textures), it automatically flushes and starts a new batch.

### Vertex Format

**QuadVertex:**
- `position` (vec3) - World position
- `color` (vec4) - RGBA color
- `texCoord` (vec2) - Texture coordinates
- `texIndex` (float) - Texture slot index
- `tilingFactor` (float) - Texture repeat factor

**LineVertex:**
- `position` (vec3) - World position
- `color` (vec4) - RGBA color

## Basic Usage

```cpp
#include <limbo/Limbo.hpp>

// Initialize once at startup
Renderer2D::init();

// Each frame
void render() {
    Renderer2D::resetStats();  // Reset per-frame statistics
    
    Renderer2D::beginScene(camera);
    
    // Draw colored quads
    Renderer2D::drawQuad({0, 0}, {1, 1}, {1, 0, 0, 1});  // Red quad
    
    // Draw textured quads
    Renderer2D::drawQuad({2, 0}, {1, 1}, texture);
    
    // Draw rotated quads
    Renderer2D::drawRotatedQuad({4, 0}, {1, 1}, glm::radians(45.0f), {0, 1, 0, 1});
    
    Renderer2D::endScene();
}

// Cleanup at shutdown
Renderer2D::shutdown();
```

## API Reference

### Lifecycle

```cpp
static void init();      // Initialize renderer (call once)
static void shutdown();  // Release resources (call once)
```

### Scene Management

```cpp
static void beginScene(const OrthographicCamera& camera);  // Start rendering
static void endScene();                                     // End and flush
static void flush();                                        // Manual flush
```

### Drawing Quads

**Colored Quads:**
```cpp
static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
static void drawQuad(const glm::mat4& transform, const glm::vec4& color);
```

**Textured Quads:**
```cpp
static void drawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const Texture2D& texture, f32 tilingFactor = 1.0f,
                     const glm::vec4& tintColor = glm::vec4(1.0f));

// With custom UVs (for sprite sheets)
static void drawQuad(const glm::vec3& position, const glm::vec2& size,
                     const Texture2D& texture, const glm::vec2& uvMin, const glm::vec2& uvMax,
                     const glm::vec4& tintColor = glm::vec4(1.0f));
```

**Rotated Quads:**
```cpp
static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, 
                            f32 rotation, const glm::vec4& color);
static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                            f32 rotation, const Texture2D& texture, f32 tilingFactor = 1.0f,
                            const glm::vec4& tintColor = glm::vec4(1.0f));
```

### Debug Primitives

**Lines:**
```cpp
static void drawLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color);
static void drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);
```

**Wireframe Shapes:**
```cpp
static void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
static void drawRect(const glm::vec2& position, const glm::vec2& size, f32 rotation, const glm::vec4& color);
static void drawCircle(const glm::vec2& center, f32 radius, const glm::vec4& color, i32 segments = 32);
```

**Filled Shapes:**
```cpp
static void drawFilledCircle(const glm::vec2& center, f32 radius, const glm::vec4& color, i32 segments = 32);
```

### Statistics

```cpp
struct Statistics {
    u32 drawCalls;     // Number of GPU draw calls
    u32 quadCount;     // Total quads rendered
    u32 lineCount;     // Total lines rendered
    u32 textureBinds;  // Total texture binds
    u32 batchCount;    // Number of batches used
    
    u32 vertexCount() const;  // Computed: quadCount * 4 + lineCount * 2
    u32 indexCount() const;   // Computed: quadCount * 6
};

static Statistics getStats();
static void resetStats();  // Call at start of frame
```

## Text Rendering

Text rendering is provided through the `Font` and `TextRenderer` classes.

### Loading Fonts

```cpp
auto fontResult = Font::loadFromFile("assets/fonts/arial.ttf", 32.0f);
if (fontResult) {
    m_font = std::move(fontResult.value());
}
```

### Drawing Text

```cpp
// Draw text (must be between beginScene/endScene)
TextRenderer::drawText("Hello World", {10, 100}, *m_font, 1.0f, {1, 1, 1, 1});

// Measure text bounds
glm::vec2 size = TextRenderer::measureText("Hello World", *m_font, 1.0f);
```

### Font Class

```cpp
class Font {
    static Result<Unique<Font>, String> loadFromFile(const std::filesystem::path& path,
                                                     f32 fontSize, i32 firstChar = 32,
                                                     i32 charCount = 95);
    
    const Glyph& getGlyph(char c) const;
    const Texture2D* getAtlas() const;
    f32 getFontSize() const;
    f32 getLineHeight() const;
    f32 getAscent() const;
    f32 getDescent() const;
};
```

## Sprite Materials

The `SpriteMaterial` class allows custom shaders and uniforms for sprite effects.

### Creating Materials

```cpp
// Default material
auto material = SpriteMaterial::create();

// With custom shader
auto material = SpriteMaterial::create(customShader);
```

### Setting Properties

```cpp
material->setColor({1, 0.5f, 0.5f, 1});  // Tint color
material->setTexture(texture);
material->setTilingFactor(2.0f);

// Custom uniforms for shader effects
material->setFloat("u_Time", time);
material->setVector2("u_Offset", offset);
material->setVector4("u_OutlineColor", outlineColor);
```

### Using Materials

```cpp
material->bind();
// Draw sprites...
material->unbind();
```

## Sprite Sorting

Sprites are sorted by layer and order for correct rendering:

```cpp
struct SpriteRendererComponent {
    glm::vec4 color;
    AssetId textureId;
    i32 sortingLayer = 0;  // Layer takes priority
    i32 sortingOrder = 0;  // Order within layer
    glm::vec2 uvMin, uvMax;
};
```

Lower values render first (behind higher values). The `SpriteRenderSystem` automatically sorts sprites before rendering.

## Performance Guidelines

1. **Minimize Texture Switches**: Group sprites by texture when possible
2. **Use Sprite Atlases**: Combine multiple sprites into one texture
3. **Batch Similar Sprites**: Same texture and similar properties batch together
4. **Monitor Statistics**: Use `getStats()` to track draw calls and batches
5. **Reset Stats Each Frame**: Call `resetStats()` at the start of each frame

### Typical Performance

- 10,000+ quads per batch
- 1-3 draw calls for simple scenes
- 60 FPS with thousands of sprites on modest hardware

## Debug Rendering Guide

Debug primitives are useful for visualizing game state:

```cpp
// Draw entity bounds
Renderer2D::drawRect(entity.position, entity.size, {0, 1, 0, 1});

// Draw physics colliders
for (auto& collider : colliders) {
    if (collider.isCircle) {
        Renderer2D::drawCircle(collider.center, collider.radius, {1, 0, 0, 1});
    } else {
        Renderer2D::drawRect(collider.center, collider.size, collider.rotation, {1, 0, 0, 1});
    }
}

// Draw velocity vectors
Renderer2D::drawLine(entity.position, entity.position + entity.velocity, {1, 1, 0, 1});

// Draw path waypoints
for (size_t i = 0; i < path.size() - 1; i++) {
    Renderer2D::drawLine(path[i], path[i + 1], {0, 0, 1, 1});
}
```

## Statistics Interpretation

| Metric | Ideal | Action if High |
|--------|-------|----------------|
| drawCalls | 1-5 | Reduce texture variety, use atlases |
| batchCount | 1-2 | Group sprites by texture |
| textureBinds | < 10 | Use sprite atlases |
| quadCount | N/A | Cull off-screen sprites |
