# Limbo Engine - Claude Configuration

## Project Overview

Limbo is a modern C++20 2D game engine featuring:
- OpenGL 4.5 batched 2D rendering
- EnTT-based Entity Component System (ECS)
- Parent-child entity hierarchies
- Box2D physics integration
- Lua scripting via sol2
- Prefab system with instance overrides
- Visual level editor (ImGui-based)
- Hot-reloading assets
- JSON scene/prefab serialization

### Editor Features
- Undo/redo system (command pattern)
- Transform gizmos (translate/rotate/scale with snapping)
- Play mode with scene state preservation
- Asset browser with search and drag-drop
- Docking layout persistence

## Quality Standards

**IMPORTANT**: All code changes must follow the quality standards defined in `docs/QUALITY_BAR.md`.

### Before Every Code Change

1. **Follow naming conventions** - See Naming Conventions section below
2. **Apply clang-format** - Run on all changed files before committing
3. **Check clang-tidy** - No new warnings allowed
4. **Write tests** - Unit tests required for new functionality
5. **Run tests** - All existing tests must pass
6. **Update documentation** - Document public APIs and user-facing features

### Definition of Done Checklist

Before considering any task complete:
- [ ] Code compiles without errors
- [ ] No new compiler warnings
- [ ] clang-format applied
- [ ] clang-tidy passes
- [ ] Unit tests written and passing
- [ ] Public APIs documented
- [ ] Commit message follows conventional format

### Commit Message Format

Use conventional commits:
```
type(scope): description

[optional body]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
```
feat(ecs): add parent-child hierarchy component
fix(render): correct texture slot overflow handling
docs(readme): update build instructions
test(scene): add serialization roundtrip tests
```

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

# Format code (MUST run before committing - CI uses clang-format-15)
find . -path ./build -prune -o \( -name '*.cpp' -o -name '*.hpp' \) -print | xargs clang-format -i
```

**IMPORTANT**: Always run clang-format before pushing. CI uses clang-format-15, so formatting differences may occur with newer versions. Use `// clang-format off` for macros that format differently across versions.

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
├── tools/            # CLI tools (assetcooker, etc.)
├── third_party/      # External dependencies
├── extern/           # Additional externals
├── assets/           # Game assets
├── docs/             # Documentation
│   ├── QUALITY_BAR.md       # Quality standards (MUST READ)
│   ├── ENGINE_LIFECYCLE.md  # Time, Input, App lifecycle
│   ├── ASSET_PIPELINE.md    # Asset system documentation
│   ├── SCENES_AND_PREFABS.md # Prefab and hierarchy system
│   ├── EDITOR_WORKFLOWS.md  # Editor usage guide
│   └── ROADMAP.md           # Development milestones
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

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `TransformComponent` |
| Functions/Methods | camelCase | `getWorldTransform()` |
| Variables | camelCase | `entityCount` |
| Member variables | m_ prefix | `m_registry` |
| Constants | PascalCase or UPPER_CASE | `MaxTextureSlots` |
| Namespaces | lowercase | `limbo::render` |
| Macros | LIMBO_ prefix, UPPER_CASE | `LIMBO_ASSERT` |

### Include Order

1. Corresponding header (for .cpp files)
2. Project headers (`limbo/...`)
3. Third-party headers
4. Standard library headers

```cpp
#include "limbo/ecs/World.hpp"       // Corresponding header

#include "limbo/core/Types.hpp"      // Project headers
#include "limbo/ecs/Components.hpp"

#include <entt/entt.hpp>             // Third-party
#include <glm/glm.hpp>

#include <memory>                    // Standard library
#include <vector>
```

### Code Patterns

- Use `limbo::` namespace for engine code
- Prefer `limbo::Unique<T>` and `limbo::Shared<T>` over raw pointers
- Use `limbo::f32`, `limbo::u32`, etc. for numeric types
- Components are simple data structs
- Systems operate on components via ECS queries
- Use `[[nodiscard]]` on functions returning values that shouldn't be ignored
- Mark methods `const` when they don't modify state

### Error Handling

| Mechanism | Use Case |
|-----------|----------|
| `LIMBO_ASSERT` | Programmer errors, should never happen |
| `LIMBO_VERIFY` | Condition with side effects |
| `LIMBO_ENSURE` | Recoverable errors, continue execution |
| `Result<T>` | Operations that can fail at runtime |

Never use exceptions in engine code.

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

Tests are located in `tests/` directory using Catch2:

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific category
./build/bin/limbo_tests "[ecs]"    # Run ECS tests only
./build/bin/limbo_tests "[math]"   # Run math tests only

# Verbose output
./build/bin/limbo_tests -v
```

### Test Requirements

- Unit tests required for all public API functions
- Test edge cases and error conditions
- All tests must pass before committing

## Logging

Use category-specific logging macros:

```cpp
LIMBO_LOG_CORE_INFO("Message");
LIMBO_LOG_RENDER_WARN("Warning: {}", value);
LIMBO_LOG_PHYSICS_ERROR("Error occurred");
```

| Category | Use For |
|----------|---------|
| CORE | Engine initialization, lifecycle |
| RENDER | Rendering, shaders, textures |
| PHYSICS | Physics simulation, collisions |
| AUDIO | Audio playback, sound loading |
| SCRIPT | Lua scripting, script errors |
| EDITOR | Editor-specific messages |
| ASSET | Asset loading, hot-reload |
| INPUT | Input handling |
| ECS | Entity/component operations |

## Debug Tools

- **Stats Panel**: FPS, frame time, renderer stats
- **Entity Inspector**: View/edit entity components
- **Log Console**: Filter logs by level and category
- **Profiler Panel**: CPU timing, frame allocator stats, thread pool status
- Use `DebugPanels::showProfilerPanel()` for performance analysis

## Common Tasks

### Adding a New Component

1. Define component struct in `engine/include/limbo/ecs/components/`
2. Register in component registry if needed
3. Add serialization support in scene loader
4. Write unit tests

### Adding a New System

1. Create system class in `engine/src/ecs/systems/`
2. Register in World initialization
3. Call update in game loop
4. Write unit tests

### Adding a New Asset Type

1. Define asset struct and `AssetType` enum value
2. Implement `IAssetImporter` for the new type
3. Register importer in `AssetImporterManager`
4. Update `assetcooker` tool if needed
5. Write unit tests

## Performance Infrastructure (Milestone 9)

The engine includes performance tools for profiling and optimization:

### CPU Profiler
```cpp
#include "limbo/debug/Profiler.hpp"

void MySystem::update(World& world, f32 dt) {
    LIMBO_PROFILE_SCOPE("MySystem::update");
    // ... work ...
}

// Export data
profiler::Profiler::exportToCSV("profiler_data.csv");
```

### Frame Allocator
```cpp
#include "limbo/core/FrameAllocator.hpp"

// Use FrameVector instead of std::vector for per-frame temporary data
FrameVector<Entity> entities;
entities.reserve(100);
// Memory automatically freed at frame end
```

### Thread Pool
```cpp
#include "limbo/core/ThreadPool.hpp"
#include "limbo/core/MainThreadQueue.hpp"

// Submit background work
ThreadPool::submit([]() {
    // File I/O, decoding - NO OpenGL/ImGui calls!
});

// Queue GPU work for main thread
MainThreadQueue::enqueue([texture, data]() {
    texture->upload(data);  // Safe on main thread
});
```

### Async Asset Loading
```cpp
#include "limbo/assets/AssetLoader.hpp"

AssetLoader::loadAsync<TextureAsset>(manager, "textures/player.png",
    [](AssetId id, bool success) {
        // Called on main thread when ready
    });
```

See `docs/PERFORMANCE.md` for complete API documentation.

## Notes

- All dependencies are fetched via CMake FetchContent
- Use `compile_commands.json` for IDE integration
- Editor saves scenes as JSON in assets directory
- See `docs/QUALITY_BAR.md` for complete quality standards

---

## Session History

### 2026-01-09: Milestone 9 Complete

**Implemented Performance & Architecture Scale-Up:**

1. **CPU Profiler** (`engine/include/limbo/debug/Profiler.hpp`)
   - Hierarchical scoped timing with `LIMBO_PROFILE_SCOPE`
   - Frame history ring buffer (120 frames)
   - CSV export for external analysis

2. **Frame Allocator** (`engine/include/limbo/core/FrameAllocator.hpp`)
   - Bump-pointer allocator reset each frame
   - `FrameVector<T>` replaces std::vector for temporary data
   - Eliminates per-frame heap allocations

3. **Thread Pool** (`engine/include/limbo/core/ThreadPool.hpp`)
   - Worker threads for background jobs
   - `MainThreadQueue` for GPU work deferral
   - Thread safety rules documented

4. **Async Asset Loading** (`engine/include/limbo/assets/AssetLoader.hpp`)
   - Background IO with main-thread GPU upload
   - New states: Queued, LoadingIO, LoadingGPU

5. **Profiler Panel** (`DebugPanels::showProfilerPanel()`)
   - CPU timeline visualization
   - Smoothed values (updates every 100ms)
   - Pause/Resume for inspection
   - Color-coded percentages

6. **SpriteRenderSystem Optimization**
   - Dirty flag pattern with EnTT signals
   - Only re-sorts when entities added/removed

**Files Created:**
- `engine/include/limbo/debug/Profiler.hpp`
- `engine/include/limbo/core/FrameAllocator.hpp`
- `engine/include/limbo/core/ThreadPool.hpp`
- `engine/include/limbo/core/MainThreadQueue.hpp`
- `engine/include/limbo/assets/AssetLoader.hpp`
- `docs/PERFORMANCE.md`

**Next Steps (Remaining Milestones):**
- Milestone 6: Rendering v2 (materials, text, debug rendering)
- Milestone 7: Physics & Gameplay Integration (queries, events)
- Milestone 8: Scripting v2 (stable API, hot reload)
