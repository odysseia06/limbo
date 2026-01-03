# Limbo Engine

A modern C++20 2D game engine with ECS architecture, physics, audio, scripting, and a visual editor.

## Features

- OpenGL 4.5 batched 2D rendering
- EnTT-based Entity Component System
- Box2D physics integration
- Lua scripting via sol2
- Particle system
- Tilemap support
- In-game UI widgets
- Visual level editor
- Hot-reloading assets
- JSON scene serialization

## Building

All dependencies are fetched automatically via CMake - no package manager required.

```bash
cmake -B build -S .
cmake --build build --config Debug

# Run the demo
./build/bin/sandbox

# Run the editor
./build/bin/limbo_editor
```

## Quick Start

```cpp
#include <limbo/Limbo.hpp>

class MyGame : public limbo::Application {
protected:
    void onInit() override {
        m_renderContext = limbo::RenderContext::create();
        m_renderContext->init(getWindow());
        limbo::Renderer2D::init();
        
        float aspect = getWindow().getWidth() / (float)getWindow().getHeight();
        m_camera = limbo::OrthographicCamera(-aspect, aspect, -1.0f, 1.0f);
        
        auto sprite = getWorld().createEntity("Sprite");
        sprite.addComponent<limbo::TransformComponent>();
        sprite.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
    }
    
    void onUpdate(limbo::f32 dt) override { }
    
    void onRender() override {
        m_renderContext->clear(0.1f, 0.1f, 0.2f, 1.0f);
        limbo::Renderer2D::beginScene(m_camera);
        
        getWorld().each<limbo::TransformComponent, limbo::SpriteRendererComponent>(
            [](auto, auto& t, auto& s) { limbo::Renderer2D::drawQuad(t.getMatrix(), s.color); }
        );
        
        limbo::Renderer2D::endScene();
    }
    
    void onShutdown() override {
        limbo::Renderer2D::shutdown();
        m_renderContext->shutdown();
    }

private:
    limbo::Unique<limbo::RenderContext> m_renderContext;
    limbo::OrthographicCamera m_camera{-1, 1, -1, 1};
};

int main() {
    limbo::debug::init();
    MyGame game;
    limbo::ApplicationConfig config;
    config.window.title = "My Game";
    if (game.init(config)) { game.run(); game.shutdown(); }
    limbo::debug::shutdown();
}
```

## Documentation

- [Getting Started](docs/GETTING_STARTED.md)
- [Architecture](docs/ARCHITECTURE.md)
- [API Reference](docs/API.md)
- [Examples](docs/EXAMPLES.md)
- [Editor Guide](docs/EDITOR.md)

## Requirements

- CMake 3.25+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 15+)

## License

MIT License
