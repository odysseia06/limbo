# Limbo Engine - Claude Configuration

## Project Overview

Limbo is a modern C++20 2D game engine featuring:
- OpenGL 4.5 batched 2D rendering
- EnTT-based Entity Component System (ECS)
- Box2D physics integration
- Lua scripting via sol2
- Visual level editor (ImGui-based)
- Hot-reloading assets
- JSON scene serialization

## Build Commands

```bash
# Using CMake Presets (recommended)
cmake --preset debug        # Configure debug build
cmake --build --preset debug   # Build debug

cmake --preset release      # Configure release build
cmake --build --preset release # Build release

# Manual configuration
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel

# Run applications
./build/bin/sandbox        # Demo application
./build/bin/limbo_editor   # Visual editor

# Run tests
ctest --test-dir build --output-on-failure
```

## Project Structure

```
limbo/
├── engine/           # Core engine library
│   ├── include/      # Public headers (limbo/)
│   └── src/          # Implementation
├── apps/
│   ├── common/       # Shared app code
│   ├── editor/       # Visual level editor
│   └── sandbox/      # Demo application
├── tests/            # Unit tests
├── third_party/      # External dependencies
├── extern/           # Additional externals
├── assets/           # Game assets
├── docs/             # Documentation
└── cmake/            # CMake modules
```

## Code Style

- **Standard**: C++20
- **Formatter**: clang-format (config in `.clang-format`)
- **Linter**: clang-tidy (config in `.clang-tidy`)
- **Indent**: 4 spaces (no tabs)
- **Column limit**: 100 characters
- **Brace style**: Attach (K&R style)

### Naming Conventions

- Classes/Structs: `PascalCase`
- Functions/Methods: `camelCase`
- Variables: `camelCase`
- Member variables: `m_camelCase` prefix
- Constants/Enums: `PascalCase` or `UPPER_CASE`
- Namespaces: `lowercase`

### Code Patterns

- Use `limbo::` namespace for engine code
- Prefer `limbo::Unique<T>` and `limbo::Shared<T>` over raw pointers
- Use `limbo::f32`, `limbo::u32`, etc. for numeric types
- Components are simple data structs
- Systems operate on components via ECS queries

## Key Dependencies

- **EnTT**: Entity Component System
- **GLFW**: Window/input management
- **glad**: OpenGL loader
- **glm**: Math library
- **stb**: Image loading
- **ImGui**: Editor UI
- **Box2D**: Physics
- **sol2**: Lua bindings
- **nlohmann/json**: JSON serialization
- **spdlog**: Logging

## Testing

Tests are located in `tests/` directory using Catch2. Run with:
```bash
ctest --test-dir build --output-on-failure

# Or run directly with filtering
./build/bin/limbo_tests "[ecs]"    # Run ECS tests only
./build/bin/limbo_tests "[math]"   # Run math tests only
```

## Logging

Use category-specific logging macros:
```cpp
LIMBO_LOG_CORE_INFO("Message");
LIMBO_LOG_RENDER_WARN("Warning: {}", value);
LIMBO_LOG_PHYSICS_ERROR("Error occurred");
```

Categories: CORE, RENDER, PHYSICS, AUDIO, SCRIPT, EDITOR, ASSET, INPUT, ECS

## Debug Tools

- **Stats Panel**: FPS, frame time, renderer stats
- **Entity Inspector**: View/edit entity components
- **Log Console**: Filter logs by level and category
- Use `DebugPanels::showLogConsole()` in ImGui frame

## Common Tasks

### Adding a New Component

1. Define component struct in `engine/include/limbo/ecs/components/`
2. Register in component registry if needed
3. Add serialization support in scene loader

### Adding a New System

1. Create system class in `engine/src/ecs/systems/`
2. Register in World initialization
3. Call update in game loop

## Notes

- All dependencies are fetched via CMake FetchContent
- Use `compile_commands.json` for IDE integration
- Editor saves scenes as JSON in assets directory
