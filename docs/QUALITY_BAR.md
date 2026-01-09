# Limbo Engine - Quality Bar

This document defines the quality standards, coding conventions, and best practices for the Limbo Engine project.

---

## Definition of Done (DoD)

A feature or milestone is considered "done" only when it meets ALL of the following criteria:

### Code Quality
- [ ] Code compiles without errors on all target platforms (Windows, Linux)
- [ ] No new compiler warnings introduced
- [ ] clang-format applied to all changed files
- [ ] clang-tidy passes without new warnings
- [ ] No memory leaks (verified with ASan when applicable)
- [ ] No undefined behavior (verified with UBSan when applicable)

### Testing
- [ ] Unit tests written for new functionality
- [ ] All existing tests pass
- [ ] Manual testing completed for user-facing features

### Documentation
- [ ] Public APIs documented with comments
- [ ] User-facing features documented in `docs/`
- [ ] CHANGELOG updated for significant changes

### Demo/Verification
- [ ] Feature demonstrated in `apps/sandbox` or dedicated demo app
- [ ] Verification checklist completed (if applicable)

---

## Coding Standards

### General Principles

1. **Clarity over cleverness** - Write code that is easy to understand
2. **Fail fast, fail loud** - Errors should be detected and reported early
3. **No silent failures** - All errors must be logged or propagated
4. **Minimize allocations** - Avoid per-frame heap allocations in hot paths

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

### Code Formatting

The project uses clang-format for consistent formatting. Key rules:

- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 100 characters
- **Brace style**: Attach (K&R)
- **Pointer alignment**: `Type* ptr` (pointer with type)

Run formatting before committing:
```bash
# Format all files
find engine apps tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

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

---

## Error Handling Policy

### When to Use Each Mechanism

| Mechanism | Use Case | Example |
|-----------|----------|---------|
| `LIMBO_ASSERT` | Programmer errors, should never happen | Invalid enum value |
| `LIMBO_VERIFY` | Condition with side effects | `LIMBO_VERIFY(init())` |
| `LIMBO_ENSURE` | Recoverable errors, continue execution | Missing optional asset |
| `Result<T>` | Operations that can fail at runtime | File loading |
| Exceptions | Never used in engine code | - |

### Guidelines

1. **Assert liberally in debug builds** - They cost nothing in release
2. **Log all errors** - Use appropriate log category
3. **Provide context** - Include relevant state in error messages
4. **Validate at boundaries** - Check inputs at public API entry points

```cpp
// Good: Clear context in assertion
LIMBO_ASSERT(entity != entt::null, "Cannot add component to null entity");

// Good: Use Result for fallible operations
Result<Texture> loadTexture(const std::string& path) {
    if (!fileExists(path)) {
        return unexpected<String>("Texture not found: " + path);
    }
    // ...
}
```

---

## Logging Guidelines

### Log Categories

| Category | Use For |
|----------|---------|
| CORE | Engine initialization, lifecycle, general |
| RENDER | Rendering, shaders, textures |
| PHYSICS | Physics simulation, collisions |
| AUDIO | Audio playback, sound loading |
| SCRIPT | Lua scripting, script errors |
| EDITOR | Editor-specific messages |
| ASSET | Asset loading, hot-reload |
| INPUT | Input handling |
| ECS | Entity/component operations |

### Log Levels

| Level | Use For |
|-------|---------|
| TRACE | Detailed debugging (disabled in release) |
| DEBUG | Development information (disabled in release) |
| INFO | Normal operational messages |
| WARN | Potential issues, degraded behavior |
| ERROR | Errors that don't crash |
| CRITICAL | Fatal errors |

### Usage

```cpp
LIMBO_LOG_RENDER_INFO("Renderer initialized with {} texture slots", maxSlots);
LIMBO_LOG_ASSET_WARN("Asset '{}' not found, using fallback", path);
LIMBO_LOG_PHYSICS_ERROR("Invalid body configuration for entity {}", entityId);
```

---

## Performance Budgets

### Frame Time Targets

| Build | Target FPS | Max Frame Time |
|-------|------------|----------------|
| Debug | 30 | 33.3 ms |
| Release | 60 | 16.6 ms |

### Per-Frame Limits (Guideline)

| Operation | Budget |
|-----------|--------|
| Physics step | 2 ms |
| Script update | 2 ms |
| Rendering | 8 ms |
| Audio | 1 ms |
| Other | 3 ms |

### Memory Guidelines

1. **Avoid per-frame allocations** in hot paths
2. **Use object pools** for frequently created/destroyed objects
3. **Pre-allocate buffers** where sizes are known
4. **Profile before optimizing** - measure, don't guess

---

## Testing Requirements

### Unit Test Coverage

New code should include tests for:

1. **Public API functions** - Expected inputs and outputs
2. **Edge cases** - Empty inputs, boundary values
3. **Error conditions** - Invalid inputs, failure modes

### Test Organization

```
tests/
├── core/           # Core utilities and types
├── ecs/            # Entity component system
├── math/           # Math utilities and transforms
├── scene/          # Scene serialization
├── physics/        # Physics queries and events
└── integration/    # Multi-system tests
```

### Running Tests

```bash
# Build and run all tests
cmake --build build --target limbo_tests
ctest --test-dir build --output-on-failure

# Run specific test category
./build/bin/limbo_tests "[ecs]"

# Run with verbose output
./build/bin/limbo_tests -v
```

---

## CI Requirements

All pull requests must pass:

1. **Build** - Compiles on Windows and Linux
2. **Tests** - All tests pass
3. **Format** - clang-format check passes
4. **Static Analysis** - clang-tidy check passes (no new warnings)
5. **Sanitizers** - ASan and UBSan pass on Linux

---

## Version Control Guidelines

### Commit Messages

Use conventional commit format:

```
type(scope): description

[optional body]

[optional footer]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
```
feat(ecs): add parent-child hierarchy component
fix(render): correct texture slot overflow handling
docs(readme): update build instructions
test(scene): add serialization roundtrip tests
```

### Branch Naming

- `feature/description` - New features
- `fix/description` - Bug fixes
- `refactor/description` - Code improvements
- `docs/description` - Documentation

---

## Review Checklist

Before requesting review, verify:

- [ ] Code follows naming conventions
- [ ] clang-format applied
- [ ] No new compiler warnings
- [ ] Tests added for new functionality
- [ ] All tests pass locally
- [ ] Documentation updated (if applicable)
- [ ] Commit messages follow convention
- [ ] No unrelated changes included
