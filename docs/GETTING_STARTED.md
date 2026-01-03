# Getting Started with Limbo Engine

This guide will walk you through setting up Limbo Engine and creating your first game.

## Prerequisites

- **CMake 3.25+**: Download from [cmake.org](https://cmake.org/download/)
- **C++20 Compiler**:
  - Windows: Visual Studio 2022, MinGW-w64 with GCC 12+
  - Linux: GCC 12+ or Clang 15+
  - macOS: Xcode 14+ or Clang 15+
- **Git**: For cloning the repository

## Installation

### Clone the Repository

```bash
git clone https://github.com/your-username/limbo.git
cd limbo
```

### Build the Engine

```bash
# Configure (all dependencies are fetched automatically)
cmake -B build -S .

# Build
cmake --build build --config Debug

# Verify by running the sandbox
./build/bin/sandbox
```

You should see a window with bouncing sprites, particles, and UI elements.

## Creating Your First Project

### Project Structure

Create a new folder for your game:

```
my_game/
├── CMakeLists.txt
├── main.cpp
└── assets/
    ├── textures/
    ├── scripts/
    └── scenes/
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add limbo as subdirectory (adjust path as needed)
add_subdirectory(path/to/limbo)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE limbo::engine)
```

### main.cpp - Minimal Example

```cpp
#include <limbo/Limbo.hpp>
#include <spdlog/spdlog.h>

class MyGame : public limbo::Application {
protected:
    void onInit() override {
        // Create render context
        m_renderContext = limbo::RenderContext::create();
        m_renderContext->init(getWindow());
        limbo::Renderer2D::init();
        
        // Setup camera
        float aspect = static_cast<float>(getWindow().getWidth()) / 
                       static_cast<float>(getWindow().getHeight());
        m_camera = limbo::OrthographicCamera(-aspect, aspect, -1.0f, 1.0f);
        
        // Create a simple sprite
        auto sprite = getWorld().createEntity("MySprite");
        sprite.addComponent<limbo::TransformComponent>();
        sprite.addComponent<limbo::SpriteRendererComponent>(
            glm::vec4(1.0f, 0.5f, 0.2f, 1.0f)  // Orange color
        );
        
        spdlog::info("Game initialized!");
    }
    
    void onUpdate(limbo::f32 deltaTime) override {
        // Handle input
        if (limbo::Input::isKeyDown(limbo::Key::Escape)) {
            requestExit();
        }
        
        // Move camera with arrow keys
        glm::vec3 pos = m_camera.getPosition();
        float speed = 2.0f * deltaTime;
        
        if (limbo::Input::isKeyDown(limbo::Key::Up)) pos.y += speed;
        if (limbo::Input::isKeyDown(limbo::Key::Down)) pos.y -= speed;
        if (limbo::Input::isKeyDown(limbo::Key::Left)) pos.x -= speed;
        if (limbo::Input::isKeyDown(limbo::Key::Right)) pos.x += speed;
        
        m_camera.setPosition(pos);
    }
    
    void onRender() override {
        m_renderContext->clear(0.1f, 0.1f, 0.2f, 1.0f);
        
        limbo::Renderer2D::beginScene(m_camera);
        
        // Render all sprites
        getWorld().each<limbo::TransformComponent, limbo::SpriteRendererComponent>(
            [](auto, auto& transform, auto& sprite) {
                limbo::Renderer2D::drawQuad(transform.getMatrix(), sprite.color);
            }
        );
        
        limbo::Renderer2D::endScene();
    }
    
    void onShutdown() override {
        limbo::Renderer2D::shutdown();
        m_renderContext->shutdown();
    }

private:
    limbo::Unique<limbo::RenderContext> m_renderContext;
    limbo::OrthographicCamera m_camera{-1.0f, 1.0f, -1.0f, 1.0f};
};

int main() {
    limbo::debug::init();
    
    MyGame game;
    
    limbo::ApplicationConfig config;
    config.appName = "My Game";
    config.window.title = "My First Limbo Game";
    config.window.width = 1280;
    config.window.height = 720;
    
    if (auto result = game.init(config); !result) {
        spdlog::critical("Failed to init: {}", result.error());
        return 1;
    }
    
    game.run();
    game.shutdown();
    
    limbo::debug::shutdown();
    return 0;
}
```

## Core Concepts

### Entity Component System (ECS)

Limbo uses an ECS architecture where:

- **Entities** are unique IDs (handles to game objects)
- **Components** are pure data (position, color, physics properties)
- **Systems** contain logic that operates on entities with specific components

```cpp
// Create an entity
auto player = getWorld().createEntity("Player");

// Add components
player.addComponent<limbo::TransformComponent>();
player.addComponent<limbo::SpriteRendererComponent>(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

// Access components
auto& transform = player.getComponent<limbo::TransformComponent>();
transform.position = glm::vec3(1.0f, 0.0f, 0.0f);

// Check for components
if (player.hasComponent<limbo::SpriteRendererComponent>()) {
    // ...
}

// Remove components
player.removeComponent<limbo::SpriteRendererComponent>();

// Destroy entity
getWorld().destroyEntity(player.id());
```

### Systems

Systems process entities with specific component combinations:

```cpp
class MovementSystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 deltaTime) override {
        world.each<limbo::TransformComponent, VelocityComponent>(
            [deltaTime](auto, auto& transform, auto& velocity) {
                transform.position += velocity.velocity * deltaTime;
            }
        );
    }
};

// Register the system
getSystems().addSystem<MovementSystem>();
```

### Rendering

All 2D rendering goes through `Renderer2D`:

```cpp
// Begin a render pass
limbo::Renderer2D::beginScene(camera);

// Draw colored quads
limbo::Renderer2D::drawQuad(
    glm::vec3(0.0f, 0.0f, 0.0f),  // position
    glm::vec2(1.0f, 1.0f),         // size
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)  // color (red)
);

// Draw textured quads
limbo::Renderer2D::drawQuad(
    glm::vec3(2.0f, 0.0f, 0.0f),
    glm::vec2(1.0f, 1.0f),
    *myTexture,
    1.0f,  // tiling factor
    glm::vec4(1.0f)  // tint
);

// End render pass (flushes batches)
limbo::Renderer2D::endScene();
```

### Input Handling

```cpp
// Keyboard
if (limbo::Input::isKeyPressed(limbo::Key::Space)) {
    // Just pressed this frame
    jump();
}

if (limbo::Input::isKeyDown(limbo::Key::W)) {
    // Held down
    moveForward();
}

// Mouse
glm::vec2 mousePos = limbo::Input::getMousePosition();

if (limbo::Input::isMouseButtonPressed(limbo::MouseButton::Left)) {
    shoot();
}

float scroll = limbo::Input::getScrollY();
zoom += scroll * 0.1f;
```

## Adding Features

### Physics

```cpp
// In onInit():
m_physics.init({0.0f, -9.81f});
getSystems().addSystem<limbo::PhysicsSystem>(m_physics);

// Create a dynamic body
auto ball = getWorld().createEntity("Ball");
auto& t = ball.addComponent<limbo::TransformComponent>();
t.position = glm::vec3(0.0f, 5.0f, 0.0f);
ball.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
ball.addComponent<limbo::Rigidbody2DComponent>(limbo::BodyType::Dynamic);
auto& collider = ball.addComponent<limbo::CircleCollider2DComponent>(0.5f);
collider.restitution = 0.8f;  // Bouncy!

// Create static ground
auto ground = getWorld().createEntity("Ground");
auto& gt = ground.addComponent<limbo::TransformComponent>();
gt.position = glm::vec3(0.0f, -3.0f, 0.0f);
gt.scale = glm::vec3(10.0f, 0.5f, 1.0f);
ground.addComponent<limbo::SpriteRendererComponent>(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
ground.addComponent<limbo::Rigidbody2DComponent>(limbo::BodyType::Static);
ground.addComponent<limbo::BoxCollider2DComponent>(glm::vec2(0.5f, 0.5f));
```

### Audio

```cpp
// In onInit():
m_audioEngine.init();
getSystems().addSystem<limbo::AudioSystem>(m_audioEngine);

// Load and play a sound
m_jumpSound.loadFromFile("assets/audio/jump.wav");
m_jumpSource.setClip(&m_jumpSound);

// When jumping:
m_jumpSource.play();
```

### Scripting with Lua

Create `assets/scripts/player.lua`:
```lua
local speed = 5.0

function on_start(entity)
    print("Player spawned!")
end

function on_update(entity, dt)
    local pos = entity:get_position()
    
    if Input.is_key_down("D") then
        pos.x = pos.x + speed * dt
    end
    if Input.is_key_down("A") then
        pos.x = pos.x - speed * dt
    end
    
    entity:set_position(pos.x, pos.y, pos.z)
end
```

In your game:
```cpp
// In onInit():
m_scriptEngine.init();
getSystems().addSystem<limbo::ScriptSystem>(m_scriptEngine);

// Create scripted entity
auto player = getWorld().createEntity("Player");
player.addComponent<limbo::TransformComponent>();
player.addComponent<limbo::SpriteRendererComponent>(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
auto& script = player.addComponent<limbo::ScriptComponent>();
script.scriptPath = "assets/scripts/player.lua";
```

### Sprite Animation

```cpp
// Load sprite sheet texture
m_spriteTexture = limbo::Texture2D::load("assets/textures/player_sheet.png");

// Setup sprite sheet (4x4 grid)
m_spriteSheet.setTexture(m_spriteTexture.get());
m_spriteSheet.createFromGrid(32, 32);  // 32x32 pixel frames

// Create animation clip
auto walkClip = std::make_shared<limbo::AnimationClip>("walk");
walkClip->setSpriteSheet(&m_spriteSheet);
walkClip->addFrameRange(0, 3, 0.1f);  // Frames 0-3, 0.1s each
walkClip->setPlayMode(limbo::AnimationPlayMode::Loop);

// Add to entity
auto& animator = player.addComponent<limbo::AnimatorComponent>();
animator.addClip("walk", walkClip);
animator.defaultClip = "walk";
animator.playOnStart = true;

// Add animation system
getSystems().addSystem<limbo::AnimationSystem>();
```

## Using the Editor

Run the editor:
```bash
./build/bin/limbo_editor
```

### Creating a Scene

1. **Create entities**: `Entity > Create Sprite`
2. **Position them**: Select entity, edit Transform in Inspector
3. **Add components**: Click "Add Component" in Inspector
4. **Save scene**: `File > Save As` (saves to `assets/scenes/`)

### Loading Scenes in Code

```cpp
limbo::SceneSerializer serializer(getWorld());
if (serializer.loadFromFile("assets/scenes/level1.json")) {
    spdlog::info("Scene loaded!");
} else {
    spdlog::error("Failed: {}", serializer.getError());
}
```

## Next Steps

- Explore the `sandbox` demo for more examples
- Read the [Architecture](ARCHITECTURE.md) document
- Check the [API Reference](API.md) for detailed documentation
- Look at [Examples](EXAMPLES.md) for common patterns
