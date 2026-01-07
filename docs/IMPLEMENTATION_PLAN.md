# Limbo Engine - Implementation Plan

This document provides a detailed, actionable implementation plan for the milestones defined in `ROADMAP.md`.

---

## Milestone 1 — Hardening & Developer Experience (DX)

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| CMake Presets | Missing | Need CMakePresets.json |
| compile_commands.json | Done | Already enabled |
| Unity/PCH builds | Missing | Optional optimization |
| clang-format | Done | CI checks format |
| clang-tidy | Missing | Not in CI |
| Sanitizers | Missing | ASan/UBSan not configured |
| Logging | Partial | spdlog exists, no categories |
| Assertions | Done | Comprehensive macros exist |
| Tests | Partial | Framework ready, few tests |
| Debug overlay | Partial | Stats panel exists, needs expansion |

### Phase 1.1: Build System Improvements

#### Task 1.1.1: Create CMakePresets.json

Create presets for:
- `debug` - Debug build with full symbols
- `release` - Optimized release build
- `relwithdebinfo` - Release with debug info
- `asan` - Debug build with Address Sanitizer
- `ubsan` - Debug build with Undefined Behavior Sanitizer

Structure:
```json
{
  "version": 6,
  "configurePresets": [
    { "name": "base", "hidden": true, "generator": "Ninja", "binaryDir": "${sourceDir}/build/${presetName}" },
    { "name": "debug", "inherits": "base", "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "release", "inherits": "base", "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } },
    { "name": "relwithdebinfo", "inherits": "base", "cacheVariables": { "CMAKE_BUILD_TYPE": "RelWithDebInfo" } }
  ],
  "buildPresets": [...],
  "testPresets": [...]
}
```

**Acceptance**: `cmake --preset debug && cmake --build --preset debug` works.

#### Task 1.1.2: Add Sanitizer Support

Add CMake module for sanitizers with options:
- `LIMBO_ENABLE_ASAN` - Address Sanitizer
- `LIMBO_ENABLE_UBSAN` - Undefined Behavior Sanitizer
- `LIMBO_ENABLE_TSAN` - Thread Sanitizer (for later)

Only enabled on Clang/GCC (not MSVC).

**Acceptance**: Build with `-DLIMBO_ENABLE_ASAN=ON` and run tests.

#### Task 1.1.3: Unity Build Support (Optional)

Add `LIMBO_UNITY_BUILD` option using CMake's `UNITY_BUILD` target property.

---

### Phase 1.2: Quality Gates

#### Task 1.2.1: Add clang-tidy CMake Target

Create CMake function that adds a `clang-tidy-check` target running clang-tidy on all engine sources.

**Acceptance**: `cmake --build build --target clang-tidy-check` runs without errors.

#### Task 1.2.2: Enhance CI Pipeline

Add jobs:
- clang-tidy analysis (Linux)
- ASan build + test (Linux)
- Optional: code coverage with lcov/gcov

---

### Phase 1.3: Diagnostics & Logging

#### Task 1.3.1: Add Log Categories

Create named loggers for subsystems:
```cpp
namespace limbo::log {
    spdlog::logger& core();
    spdlog::logger& render();
    spdlog::logger& physics();
    spdlog::logger& audio();
    spdlog::logger& script();
    spdlog::logger& editor();
    spdlog::logger& asset();
}
```

Add macros:
```cpp
#define LIMBO_LOG_CORE_INFO(...)    limbo::log::core().info(__VA_ARGS__)
#define LIMBO_LOG_RENDER_WARN(...)  limbo::log::render().warn(__VA_ARGS__)
// etc.
```

**Acceptance**: `LIMBO_LOG_RENDER_INFO("Draw calls: {}", count)` compiles and logs correctly.

#### Task 1.3.2: Add Validation Layer

Runtime validation checks (Debug builds only):
- Renderer state (begin/end scene matching)
- ECS invariants (entity validity before component access)
- Asset handle validity

Controlled by `LIMBO_ENABLE_VALIDATION` preprocessor define.

---

### Phase 1.4: Testing Expansion

#### Task 1.4.1: Add Core Tests

New tests for:
- Math utilities - glm wrapper tests, transform matrices
- ECS - Entity creation, component add/remove
- Serialization - Scene save/load roundtrip

#### Task 1.4.2: Add Headless Smoke Test

Boot engine subsystems without creating a window (skip graphics initialization or use mock).

---

### Phase 1.5: Debug Overlay Enhancement

#### Task 1.5.1: Expand Stats Panel

Add to existing stats panel:
- Memory usage estimate
- Entity count (total and by component type)
- Physics body count
- Asset cache size

#### Task 1.5.2: Add Debug Console Panel

ImGui-based console that:
- Shows log output from spdlog
- Supports filtering by log level and category
- Scrolls to bottom on new messages

---

### Phase 1.6: Documentation

#### Task 1.6.1: Write QUALITY_BAR.md

Document:
- Definition of Done rules (from ROADMAP.md)
- Coding standards summary (reference .clang-format)
- Error handling policy (when to assert vs return error)
- Performance budgets (target 60fps, max 16ms frame time)

---

### Milestone 1 Deliverables Checklist

- [ ] `CMakePresets.json` with Debug/Release/RelWithDebInfo
- [ ] Sanitizers CMake module with ASan/UBSan support
- [ ] clang-tidy CMake target
- [ ] CI: clang-tidy job added
- [ ] CI: sanitizer build job added (Linux)
- [ ] Log categories implemented
- [ ] Validation layer for Debug builds
- [ ] 5+ new unit tests added
- [ ] Headless smoke test
- [ ] Enhanced debug overlay with console
- [ ] `docs/QUALITY_BAR.md`
- [ ] Release tag: `v0.1.0-alpha`

---

## Milestone 2 — Engine Core: Time, Input, and App Lifecycle

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Game loop | Basic | Variable timestep only |
| Fixed timestep | Missing | Needed for deterministic physics |
| Input system | Basic | Direct GLFW polling |
| Action mapping | Missing | No configurable input bindings |
| Lifecycle docs | Missing | Subsystem ordering undocumented |

### Phase 2.1: Time System

#### Task 2.1.1: Implement Fixed Timestep

Configuration:
```cpp
struct TimeConfig {
    f32 fixedDeltaTime = 1.0f / 60.0f;  // 60 Hz physics
    f32 maxDeltaTime = 0.25f;            // Prevent spiral of death
    bool enableInterpolation = true;
};
```

Game loop pattern:
```cpp
while (running) {
    f32 deltaTime = clock.restart();
    deltaTime = std::min(deltaTime, config.maxDeltaTime);
    
    accumulator += deltaTime;
    while (accumulator >= config.fixedDeltaTime) {
        fixedUpdate(config.fixedDeltaTime);  // Physics, deterministic logic
        accumulator -= config.fixedDeltaTime;
    }
    
    f32 alpha = accumulator / config.fixedDeltaTime;
    update(deltaTime);   // Game logic, animations
    render(alpha);       // Interpolated rendering
}
```

#### Task 2.1.2: Delta Time Smoothing

Average delta time over N frames (e.g., 11 frames) to reduce jitter from OS scheduling.

---

### Phase 2.2: Input System

#### Task 2.2.1: Input Manager

Features:
- Keyboard state tracking (current frame + previous frame)
- Mouse state (position, buttons, scroll)
- `isKeyDown()`, `isKeyPressed()`, `isKeyReleased()` helpers
- Mouse position and delta since last frame

#### Task 2.2.2: Action Mapping System

JSON configuration (`assets/config/input.json`):
```json
{
    "actions": {
        "jump": { "keys": ["Space", "GamepadA"] },
        "fire": { "keys": ["MouseLeft", "GamepadRightTrigger"] }
    },
    "axes": {
        "move_x": { "positive": ["D", "Right", "GamepadLeftStickX+"], "negative": ["A", "Left", "GamepadLeftStickX-"] },
        "move_y": { "positive": ["W", "Up"], "negative": ["S", "Down"] }
    }
}
```

API:
```cpp
if (input.isActionPressed("jump")) { player.jump(); }
f32 moveX = input.getAxis("move_x");  // -1.0 to 1.0
```

#### Task 2.2.3: Editor vs Game Input Context

```cpp
enum class InputContext { Editor, Game, UI };
void setInputContext(InputContext ctx);
bool isInputContextActive(InputContext ctx);
```

When in Editor context, game input queries return false/zero.

---

### Phase 2.3: Application Lifecycle

#### Task 2.3.1: Subsystem Manager

```cpp
class ISubsystem {
public:
    virtual ~ISubsystem() = default;
    virtual void init() = 0;
    virtual void shutdown() = 0;
    virtual std::string_view name() const = 0;
    virtual i32 initPriority() const = 0;  // Lower = earlier init
};
```

SubsystemManager handles:
- Registration
- Ordered initialization
- Reverse-order shutdown
- Dependency validation (optional)

#### Task 2.3.2: Document Lifecycle

Document:
- Subsystem init order (logging -> window -> input -> renderer -> physics -> audio -> scripting)
- Application callbacks (onInit, onUpdate, onFixedUpdate, onRender, onShutdown)
- Shutdown sequence

---

### Milestone 2 Deliverables Checklist

- [ ] Time system with fixed timestep support
- [ ] Accumulator-based game loop in Application
- [ ] Transform interpolation helper for smooth rendering
- [ ] Delta time smoothing
- [ ] InputManager with keyboard/mouse state
- [ ] Action/axis mapping system
- [ ] Input config JSON loading
- [ ] Editor/game input context switching
- [ ] Subsystem manager with ordered init/shutdown
- [ ] `docs/ENGINE_LIFECYCLE.md`
- [ ] Demo: physics simulation with fixed timestep
- [ ] Demo: remappable input in sandbox
- [ ] Release tag: `v0.2.0-alpha`

---

## Milestone 3 — Asset Pipeline v1

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Asset loading | Basic | Direct file loading, path-based |
| Asset IDs | Partial | AssetId type exists |
| UUID system | Missing | No stable UUIDs |
| Asset cooker | Missing | No import/build pipeline |
| Hot reload | Missing | Manual restart required |

### Phase 3.1: Asset ID Strategy

#### Task 3.1.1: UUID Generation

Simple UUID v4 implementation or use a small library.

#### Task 3.1.2: Asset Registry

```cpp
struct AssetMetadata {
    UUID uuid;
    std::filesystem::path sourcePath;
    std::filesystem::path importedPath;
    u64 sourceHash;           // For change detection
    std::string importerType; // "texture", "audio", "scene", etc.
    nlohmann::json importSettings;
};

class AssetRegistry {
public:
    AssetMetadata* findByUUID(const UUID& uuid);
    AssetMetadata* findByPath(const std::filesystem::path& path);
    void registerAsset(AssetMetadata metadata);
    void save(const std::filesystem::path& registryPath);
    void load(const std::filesystem::path& registryPath);
};
```

#### Task 3.1.3: Meta Files

For each source asset `texture.png`, generate `texture.png.meta`:
```json
{
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "importer": "texture",
    "settings": {
        "filterMode": "linear",
        "wrapMode": "repeat",
        "generateMipmaps": true
    }
}
```

---

### Phase 3.2: Import Pipeline

#### Task 3.2.1: Asset Cooker Tool

CLI interface:
```bash
assetcooker --input assets/ --output build/cooked/ --registry build/asset_registry.json
```

#### Task 3.2.2: Texture Importer

- Load source image (PNG, JPG, etc.)
- Apply import settings (resize, format conversion)
- Write to cooked directory
- Update registry with metadata

#### Task 3.2.3: Sprite Atlas Builder

- Read atlas definition (list of sprite paths)
- Pack sprites using simple bin-packing
- Generate atlas texture + UV coordinate JSON

---

### Phase 3.3: Hot Reload

#### Task 3.3.1: File Watcher

Platform-specific file watching:
- Windows: `ReadDirectoryChangesW`
- Linux: `inotify`
- Or use a small cross-platform library

```cpp
class FileWatcher {
public:
    void watchDirectory(const std::filesystem::path& dir);
    void poll();  // Call each frame
    
    std::function<void(const std::filesystem::path&)> onFileModified;
    std::function<void(const std::filesystem::path&)> onFileCreated;
    std::function<void(const std::filesystem::path&)> onFileDeleted;
};
```

#### Task 3.3.2: Asset Reload Manager

- Subscribe to FileWatcher events
- On source file change: re-import asset, invalidate cache
- Notify dependent systems (e.g., sprites using a texture)

---

### Milestone 3 Deliverables Checklist

- [ ] UUID type implementation
- [ ] `.meta` file generation and parsing
- [ ] Asset registry with save/load
- [ ] Asset cooker CLI tool
- [ ] Texture importer
- [ ] Audio importer (basic)
- [ ] Sprite atlas builder
- [ ] File watcher implementation
- [ ] Hot reload integration in AssetManager
- [ ] Editor shows reload status
- [ ] `docs/ASSET_PIPELINE.md`
- [ ] Demo: texture hot reload in sandbox
- [ ] Release tag: `v0.3.0-alpha`

---

## Milestone 4 — Scene/Prefab Workflow v1

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Scene serialization | Done | JSON-based |
| Parent-child hierarchy | Missing | Flat entity list |
| Prefabs | Missing | No reusable templates |
| Schema versioning | Missing | No migration support |

### Phase 4.1: Scene Graph

#### Task 4.1.1: Hierarchy Component

```cpp
struct HierarchyComponent {
    entt::entity parent = entt::null;
    entt::entity firstChild = entt::null;
    entt::entity nextSibling = entt::null;
    entt::entity prevSibling = entt::null;
};
```

#### Task 4.1.2: Hierarchy System

Functions:
```cpp
void setParent(Entity child, Entity parent);
void removeParent(Entity child);
std::vector<Entity> getChildren(Entity parent);
Entity getParent(Entity child);
glm::mat4 getWorldTransform(Entity entity);  // Recursive parent multiplication
```

---

### Phase 4.2: Prefab System

#### Task 4.2.1: Prefab Asset Type

```cpp
class Prefab {
public:
    UUID uuid;
    nlohmann::json prototype;  // Serialized entity hierarchy
    
    static Prefab createFromEntity(World& world, Entity root);
    Entity instantiate(World& world) const;
};
```

#### Task 4.2.2: Prefab Instance Component

```cpp
struct PrefabInstanceComponent {
    UUID prefabUUID;
    nlohmann::json overrides;  // Properties that differ from prototype
};
```

#### Task 4.2.3: Override Tracking

When saving a prefab instance:
1. Compare current component values to prototype
2. Store only the differences in `overrides`

When loading:
1. Instantiate prototype
2. Apply overrides on top

---

### Phase 4.3: Serialization Versioning

#### Task 4.3.1: Schema Version Field

Add version to scene files:
```json
{
    "version": 1,
    "entities": [...]
}
```

#### Task 4.3.2: Migration Registry

```cpp
class SchemaMigration {
public:
    void registerMigration(i32 fromVersion, i32 toVersion, MigrationFunc func);
    nlohmann::json migrate(nlohmann::json scene, i32 targetVersion);
};

// Example migration
void migrateV1toV2(nlohmann::json& scene) {
    // Rename "position" to "translation" in TransformComponent
    for (auto& entity : scene["entities"]) {
        if (entity.contains("TransformComponent")) {
            auto& t = entity["TransformComponent"];
            if (t.contains("position")) {
                t["translation"] = t["position"];
                t.erase("position");
            }
        }
    }
}
```

---

### Milestone 4 Deliverables Checklist

- [ ] HierarchyComponent for parent-child relationships
- [ ] Hierarchy system with world transform calculation
- [ ] Prefab asset type
- [ ] Create prefab from selected entity (editor)
- [ ] Instantiate prefab in scene
- [ ] Override tracking and serialization
- [ ] Schema version field in scene files
- [ ] Migration registry with V1->V2 example
- [ ] `docs/SCENES_AND_PREFABS.md`
- [ ] Demo: prefab workflow in editor
- [ ] Release tag: `v0.4.0-alpha`

---

## Milestone 5 — Editor UX: "Daily Driver" Upgrade

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Panel system | Basic | ImGui windows exist |
| Layout persistence | Missing | Resets on restart |
| Undo/redo | Missing | No history |
| Gizmos | Missing | No visual manipulation |
| Play mode | Missing | No runtime preview |

### Phase 5.1: Editor Architecture

#### Task 5.1.1: Panel Base Class

```cpp
class Panel {
public:
    virtual ~Panel() = default;
    virtual void onImGui() = 0;
    virtual std::string_view name() const = 0;
    
    bool isOpen = true;
};
```

#### Task 5.1.2: Core Panels

- Scene hierarchy panel - Entity tree view
- Inspector panel - Component editing
- Content browser panel - Asset browser
- Console panel - Log output

#### Task 5.1.3: Layout Persistence

Save/load ImGui docking layout:
```cpp
void saveLayout(const std::filesystem::path& path);  // ImGui::SaveIniSettingsToDisk
void loadLayout(const std::filesystem::path& path);  // ImGui::LoadIniSettingsFromDisk
```

---

### Phase 5.2: Undo/Redo System

#### Task 5.2.1: Command Pattern

```cpp
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
};

class CommandHistory {
public:
    void execute(std::unique_ptr<ICommand> cmd);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
};
```

#### Task 5.2.2: Common Commands

- TransformCommand - Position/rotation/scale changes
- CreateEntityCommand
- DeleteEntityCommand
- AddComponentCommand
- RemoveComponentCommand
- PropertyChangeCommand - Generic property edits

---

### Phase 5.3: Gizmos

#### Task 5.3.1: Transform Gizmos

Features:
- Translate mode (arrows)
- Rotate mode (circles)
- Scale mode (boxes)
- Local vs world space toggle
- Snap to grid option

Consider using ImGuizmo library for faster implementation.

---

### Phase 5.4: Play Mode

#### Task 5.4.1: Play Mode Controller

```cpp
class EditorPlayMode {
public:
    void enterPlayMode();   // Serialize current scene, create runtime copy
    void exitPlayMode();    // Discard runtime, restore serialized scene
    void pausePlayMode();
    void resumePlayMode();
    void stepFrame();       // Advance one fixed update while paused
    
    bool isPlaying() const;
    bool isPaused() const;
};
```

Key design: Editor world and runtime world are separate. Runtime changes don't affect the saved scene.

---

### Milestone 5 Deliverables Checklist

- [ ] Panel base class and registry
- [ ] Scene hierarchy panel with tree view
- [ ] Inspector panel with component editors
- [ ] Content browser with search and filtering
- [ ] Console panel with log filtering
- [ ] Layout save/load on editor start/exit
- [ ] Command pattern implementation
- [ ] CommandHistory with undo/redo stacks
- [ ] Undo/redo for: transform, create/delete entity, add/remove component
- [ ] Transform gizmos (translate/rotate/scale)
- [ ] Grid snapping
- [ ] Play/pause/step controls
- [ ] Editor vs runtime world separation
- [ ] `docs/EDITOR_WORKFLOWS.md`
- [ ] Release tag: `v0.5.0-alpha`

---

## Milestone 6 — Rendering v2

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Batching | Done | Quad batching works |
| Materials | Missing | Hardcoded shaders |
| Text rendering | Missing | No font support |
| Debug drawing | Partial | Basic stats only |

### Phase 6.1: Material System

#### Task 6.1.1: Material Asset

```cpp
class Material {
public:
    Shared<Shader> shader;
    
    void setFloat(const std::string& name, f32 value);
    void setVec2(const std::string& name, glm::vec2 value);
    void setVec4(const std::string& name, glm::vec4 value);
    void setTexture(const std::string& name, Shared<Texture> texture);
    
    void bind() const;  // Set shader uniforms
};
```

#### Task 6.1.2: Material Serialization

Materials stored as JSON:
```json
{
    "shader": "assets/shaders/sprite.glsl",
    "uniforms": {
        "u_Tint": [1.0, 1.0, 1.0, 1.0],
        "u_EmissionStrength": 0.0
    },
    "textures": {
        "u_Texture": "assets/textures/player.png"
    }
}
```

---

### Phase 6.2: Text Rendering

#### Task 6.2.1: Bitmap Font Loader

Load BMFont format (.fnt + texture atlas).

#### Task 6.2.2: Text Renderer

```cpp
class TextRenderer {
public:
    static void init();
    static void shutdown();
    
    static void drawText(const std::string& text, glm::vec2 position, 
                         const Font& font, glm::vec4 color = glm::vec4(1.0f),
                         f32 scale = 1.0f);
};
```

---

### Phase 6.3: Debug Rendering

#### Task 6.3.1: Debug Draw API

```cpp
namespace limbo::DebugDraw {
    void init();
    void shutdown();
    
    // Immediate mode API (draws this frame only)
    void line(glm::vec2 a, glm::vec2 b, glm::vec4 color, f32 thickness = 1.0f);
    void rect(glm::vec2 min, glm::vec2 max, glm::vec4 color, f32 thickness = 1.0f);
    void rectFilled(glm::vec2 min, glm::vec2 max, glm::vec4 color);
    void circle(glm::vec2 center, f32 radius, glm::vec4 color, i32 segments = 32);
    void point(glm::vec2 pos, glm::vec4 color, f32 size = 4.0f);
    
    // Call at end of frame to render all queued primitives
    void flush(const glm::mat4& viewProjection);
}
```

#### Task 6.3.2: Physics Debug Visualization

Add option to draw:
- Collider shapes (boxes, circles)
- Contact points
- Velocity vectors

---

### Milestone 6 Deliverables Checklist

- [ ] Material class with uniform management
- [ ] Material asset loading from JSON
- [ ] SpriteRendererComponent supports Material
- [ ] BMFont loader
- [ ] TextRenderer with batching
- [ ] TextComponent for entities
- [ ] DebugDraw API
- [ ] Debug drawing for physics shapes
- [ ] Grid rendering in editor
- [ ] Render statistics (draw calls, vertices, GPU time if possible)
- [ ] `docs/RENDERER2D.md`
- [ ] Demo: text rendering in sandbox
- [ ] Release tag: `v0.6.0-alpha`

---

## Milestone 7 — Physics & Gameplay Integration

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Physics stepping | Basic | Variable timestep |
| Queries | Missing | No raycast/overlap |
| Events | Missing | No collision callbacks |
| Debug tools | Missing | No visualization |

### Phase 7.1: Fixed Timestep Integration

#### Task 7.1.1: Physics Fixed Update

Step physics in `fixedUpdate()` at consistent rate (e.g., 60 Hz).

#### Task 7.1.2: Transform Interpolation

Store previous transform, interpolate for rendering:
```cpp
glm::vec2 renderPos = glm::mix(prevPosition, currentPosition, alpha);
f32 renderAngle = glm::mix(prevAngle, currentAngle, alpha);
```

---

### Phase 7.2: Physics Queries

#### Task 7.2.1: Query API

```cpp
struct RaycastHit {
    Entity entity;
    glm::vec2 point;
    glm::vec2 normal;
    f32 fraction;  // 0-1 along ray
};

class PhysicsQueries {
public:
    std::optional<RaycastHit> raycast(glm::vec2 origin, glm::vec2 direction, 
                                       f32 maxDistance, u32 layerMask = ~0u);
    
    std::vector<RaycastHit> raycastAll(glm::vec2 origin, glm::vec2 direction,
                                        f32 maxDistance, u32 layerMask = ~0u);
    
    std::vector<Entity> overlapCircle(glm::vec2 center, f32 radius, u32 layerMask = ~0u);
    std::vector<Entity> overlapBox(glm::vec2 center, glm::vec2 halfExtents, u32 layerMask = ~0u);
};
```

---

### Phase 7.3: Collision Events

#### Task 7.3.1: Event System

```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    glm::vec2 point;
    glm::vec2 normal;
    f32 impulse;
};

struct TriggerEvent {
    Entity triggerEntity;
    Entity otherEntity;
};

// Registration
void onCollisionEnter(std::function<void(const CollisionEvent&)> callback);
void onCollisionExit(std::function<void(const CollisionEvent&)> callback);
void onTriggerEnter(std::function<void(const TriggerEvent&)> callback);
void onTriggerExit(std::function<void(const TriggerEvent&)> callback);
```

#### Task 7.3.2: Box2D Contact Listener

Implement Box2D's `b2ContactListener` to capture collision events and dispatch to registered callbacks.

---

### Milestone 7 Deliverables Checklist

- [ ] Physics stepping in fixedUpdate at fixed rate
- [ ] Transform interpolation for smooth rendering
- [ ] Raycast query (single hit)
- [ ] Raycast query (all hits)
- [ ] Overlap circle query
- [ ] Overlap box query
- [ ] Layer masks for filtering
- [ ] Collision enter/exit events
- [ ] Trigger enter/exit events
- [ ] Physics debug visualization toggle
- [ ] Lua bindings for queries and events
- [ ] `docs/PHYSICS.md`
- [ ] Demo: platformer with raycasting and triggers
- [ ] Release tag: `v0.7.0-alpha`

---

## Milestone 8 — Scripting v2

### Current State Assessment

| Area | Status | Notes |
|------|--------|-------|
| Lua bindings | Basic | sol2 integration exists |
| API surface | Minimal | Few bindings exposed |
| Hot reload | Missing | Requires restart |
| Error handling | Basic | Crashes on error |

### Phase 8.1: Stable API Surface

#### Task 8.1.1: Core Bindings

Bind these APIs:
```lua
-- Entity operations
local entity = world:createEntity("Name")
entity:destroy()
local transform = entity:getComponent("Transform")
entity:addComponent("RigidBody", { type = "dynamic" })

-- Transform
transform.position = Vec2(100, 200)
transform.rotation = 45
transform.scale = Vec2(2, 2)

-- Input
if Input.isActionPressed("jump") then ... end
local moveX = Input.getAxis("move_x")

-- Physics queries
local hit = Physics.raycast(origin, direction, maxDistance)
if hit then
    print(hit.entity, hit.point, hit.normal)
end

-- Time
local dt = Time.deltaTime
local fixedDt = Time.fixedDeltaTime

-- Debug
Debug.log("Message")
Debug.drawLine(a, b, color)
```

---

### Phase 8.2: Hot Reload

#### Task 8.2.1: Script Reloader

```cpp
class ScriptReloader {
public:
    void watchScriptDirectory(const std::filesystem::path& dir);
    void checkForChanges();  // Call each frame
    
private:
    void reloadScript(const std::filesystem::path& path);
    void preserveState(ScriptComponent& script);
    void restoreState(ScriptComponent& script);
};
```

State preservation strategy:
1. Serialize script's public variables to JSON before reload
2. Reload script file
3. Restore serialized variables

---

### Phase 8.3: Error Handling

#### Task 8.3.1: Safe Script Execution

Wrap all Lua calls in protected mode:
```cpp
auto result = lua.safe_script(code, sol::script_pass_on_error);
if (!result.valid()) {
    sol::error err = result;
    LIMBO_LOG_SCRIPT_ERROR("{}:{}: {}", filename, line, err.what());
    // Don't crash, mark script as errored
}
```

#### Task 8.3.2: Editor Error Display

Show script errors with:
- File path
- Line number
- Error message
- Stack trace

---

### Milestone 8 Deliverables Checklist

- [ ] Entity manipulation bindings
- [ ] Transform bindings
- [ ] Input bindings (actions and axes)
- [ ] Physics query bindings
- [ ] Time access bindings
- [ ] Debug.log and Debug.draw bindings
- [ ] Script hot reload on file change
- [ ] State preservation across reload
- [ ] Protected script execution
- [ ] Error display with file:line
- [ ] Stack trace on errors
- [ ] `docs/SCRIPTING.md`
- [ ] Demo: scripted gameplay (player controller, camera follow)
- [ ] Release tag: `v0.8.0-alpha`

---

## Milestone 9 — Performance & Architecture Scale-Up

### Phase 9.1: Profiling

#### Task 9.1.1: CPU Profiler

```cpp
#define LIMBO_PROFILE_SCOPE(name) \
    limbo::ProfileScope _profile_##__LINE__(name)

#define LIMBO_PROFILE_FUNCTION() \
    LIMBO_PROFILE_SCOPE(__FUNCTION__)

class ProfileScope {
public:
    ProfileScope(const char* name);
    ~ProfileScope();  // Records elapsed time
};

class Profiler {
public:
    static void beginFrame();
    static void endFrame();
    static void dumpToCSV(const std::filesystem::path& path);
    static const FrameData& getLastFrame();
};
```

#### Task 9.1.2: Profiler Overlay

Show hierarchical timing breakdown per frame in debug panels.

---

### Phase 9.2: Job System

#### Task 9.2.1: Thread Pool

```cpp
class JobSystem {
public:
    static void init(u32 numThreads = 0);  // 0 = auto-detect
    static void shutdown();
    
    static JobHandle schedule(std::function<void()> job);
    static void wait(JobHandle handle);
    static void waitAll();
    
    // Parallel for with automatic chunking
    static void parallelFor(size_t count, std::function<void(size_t index)> job);
};
```

#### Task 9.2.2: Async Asset Loading

Load assets on background thread, finalize on main thread:
```cpp
AssetFuture<Texture> loadTextureAsync(const std::filesystem::path& path);
```

---

### Phase 9.3: Memory Optimization

#### Task 9.3.1: Frame Allocator

Linear allocator that resets each frame for temporary allocations.

```cpp
class FrameAllocator {
public:
    void* allocate(size_t size, size_t alignment = 16);
    void reset();  // Call at frame start
    
    template<typename T, typename... Args>
    T* create(Args&&... args);
};
```

---

### Milestone 9 Deliverables Checklist

- [ ] LIMBO_PROFILE_SCOPE macro
- [ ] Profiler with hierarchical timing
- [ ] CSV export for offline analysis
- [ ] Profiler overlay in debug panel
- [ ] Thread pool implementation
- [ ] JobHandle for waiting
- [ ] parallelFor utility
- [ ] Async asset loading
- [ ] Frame allocator
- [ ] Reduce per-frame allocations in hot paths
- [ ] `docs/PERFORMANCE.md`
- [ ] Demo: stress test with 10,000+ entities
- [ ] Release tag: `v0.9.0-alpha`

---

## Quick Start: Beginning Milestone 1

Execute these tasks in order:

1. **Create CMakePresets.json** - Define debug/release/sanitizer presets
2. **Add Sanitizers CMake module** - ASan/UBSan configuration
3. **Add clang-tidy CMake target** - Static analysis target
4. **Update CI pipeline** - Add clang-tidy and sanitizer jobs
5. **Implement log categories** - Named loggers for subsystems
6. **Add validation layer** - Debug-only runtime checks
7. **Write new tests** - Math, ECS, serialization tests
8. **Enhance debug overlay** - Add console panel
9. **Write QUALITY_BAR.md** - Document standards
