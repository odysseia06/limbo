# Limbo Engine

A modern C++20 2D game engine with ECS architecture, physics, scripting, and a visual editor.

## Features

- OpenGL 4.5 batched 2D rendering
- EnTT-based Entity Component System
- Parent-child entity hierarchies
- Box2D physics integration
- Lua scripting via sol2
- Prefab system with instance overrides
- Hot-reloading assets
- JSON scene/prefab serialization

### Editor

- Dockable panel layout (persisted)
- Scene hierarchy with drag-drop reparenting
- Component inspector with property editing
- Transform gizmos (translate/rotate/scale with snapping)
- Undo/redo system (command pattern)
- Play mode with scene state preservation
- Asset browser with search and drag-drop
- Viewport with grid and camera controls

## Building

Limbo now uses a preset-first CMake workflow. Dependencies are managed through CMake
`FetchContent` (no external package manager required).

### Recommended (one command)

```bash
# Configure + build + run tests (Debug)
cmake --workflow --preset dev-debug

# Configure + build + run tests (Release)
cmake --workflow --preset dev-release
```

### Manual preset commands

```bash
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
```

### Sanitizers

```bash
cmake --workflow --preset sanitizer-asan
cmake --workflow --preset sanitizer-ubsan
```

### Run binaries

```bash
./build/debug/bin/sandbox        # Demo
./build/debug/bin/limbo_editor   # Editor
```

Legacy `build.ps1` / `build.sh` / `build.bat` scripts are kept for compatibility, but presets are
the primary build interface.

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
        sprite.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1, 0.5f, 0.2f, 1));
    }
    
    void onRender(limbo::f32) override {
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

LIMBO_MAIN(MyGame, "My Game")
```

## Documentation

- [Editor Workflows](docs/EDITOR_WORKFLOWS.md)
- [Scenes and Prefabs](docs/SCENES_AND_PREFABS.md)
- [Asset Pipeline](docs/ASSET_PIPELINE.md)
- [Engine Lifecycle](docs/ENGINE_LIFECYCLE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [API Reference](docs/API.md)

## Requirements

- CMake 3.25+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 15+)

## License

MIT License
