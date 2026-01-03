# Limbo Engine API Reference

## Core Types

### Type Aliases (`limbo/core/Types.hpp`)

```cpp
namespace limbo {
    // Numeric types
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using f32 = float;
    using f64 = double;
    
    // Smart pointers
    template<typename T> using Unique = std::unique_ptr<T>;
    template<typename T> using Shared = std::shared_ptr<T>;
    template<typename T> using Weak = std::weak_ptr<T>;
    
    // Containers
    using String = std::string;
}
```

---

## Window & Input

### Window (`limbo/platform/Window.hpp`)

```cpp
class Window {
public:
    static Unique<Window> create(const WindowConfig& config);
    
    void update();                          // Poll events
    bool shouldClose() const;
    void setShouldClose(bool close);
    
    u32 getWidth() const;
    u32 getHeight() const;
    void setTitle(const String& title);
    
    void* getNativeHandle() const;          // GLFW window pointer
};

struct WindowConfig {
    String title = "Limbo";
    u32 width = 1280;
    u32 height = 720;
    bool resizable = true;
    bool vsync = true;
};
```

### Input (`limbo/platform/Input.hpp`)

```cpp
class Input {
public:
    // Keyboard
    static bool isKeyDown(Key key);         // Held down
    static bool isKeyPressed(Key key);      // Just pressed this frame
    static bool isKeyReleased(Key key);     // Just released this frame
    
    // Mouse
    static bool isMouseButtonDown(MouseButton button);
    static bool isMouseButtonPressed(MouseButton button);
    static bool isMouseButtonReleased(MouseButton button);
    static f32 getMouseX();
    static f32 getMouseY();
    static glm::vec2 getMousePosition();
    static f32 getScrollX();
    static f32 getScrollY();
};
```

---

## Rendering

### Renderer2D (`limbo/render/Renderer2D.hpp`)

```cpp
class Renderer2D {
public:
    static void init();
    static void shutdown();
    
    static void beginScene(const OrthographicCamera& camera);
    static void endScene();
    
    // Draw colored quad
    static void drawQuad(const glm::vec3& position, const glm::vec2& size, 
                         const glm::vec4& color);
    
    // Draw textured quad
    static void drawQuad(const glm::vec3& position, const glm::vec2& size,
                         const Texture2D& texture, f32 tilingFactor = 1.0f,
                         const glm::vec4& tintColor = glm::vec4(1.0f));
    
    // Draw with transform matrix
    static void drawQuad(const glm::mat4& transform, const glm::vec4& color);
    static void drawQuad(const glm::mat4& transform, const Texture2D& texture,
                         f32 tilingFactor = 1.0f, 
                         const glm::vec4& tintColor = glm::vec4(1.0f));
    
    // Draw with custom UVs (for sprite sheets)
    static void drawQuad(const glm::mat4& transform, const Texture2D& texture,
                         const glm::vec2& uvMin, const glm::vec2& uvMax,
                         const glm::vec4& tintColor = glm::vec4(1.0f));
    
    // Statistics
    static void resetStats();
    static const Stats& getStats();
    
    struct Stats {
        u32 drawCalls = 0;
        u32 quadCount = 0;
    };
};
```

### OrthographicCamera (`limbo/render/Camera.hpp`)

```cpp
class OrthographicCamera {
public:
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top);
    
    void setProjection(f32 left, f32 right, f32 bottom, f32 top);
    
    const glm::vec3& getPosition() const;
    void setPosition(const glm::vec3& position);
    
    f32 getRotation() const;
    void setRotation(f32 rotation);
    
    const glm::mat4& getProjectionMatrix() const;
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getViewProjectionMatrix() const;
};
```

### Texture2D (`limbo/render/Texture.hpp`)

```cpp
class Texture2D {
public:
    void create(const TextureSpec& spec, const void* data = nullptr);
    void destroy();
    
    void bind(u32 slot = 0) const;
    void unbind() const;
    
    u32 getWidth() const;
    u32 getHeight() const;
    u32 getId() const;
    
    static Unique<Texture2D> load(const std::filesystem::path& path);
};

struct TextureSpec {
    u32 width = 0;
    u32 height = 0;
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrapS = TextureWrap::Repeat;
    TextureWrap wrapT = TextureWrap::Repeat;
    bool generateMipmaps = true;
};
```

---

## Entity Component System

### World (`limbo/ecs/World.hpp`)

```cpp
class World {
public:
    using EntityId = entt::entity;
    
    // Entity management
    Entity createEntity(const String& name = "Entity");
    void destroyEntity(EntityId id);
    bool isValid(EntityId id) const;
    size_t entityCount() const;
    void clear();
    
    // Component access
    template<typename T> T& getComponent(EntityId id);
    template<typename T> bool hasComponent(EntityId id) const;
    
    // Iteration
    template<typename... Components>
    auto view();
    
    template<typename... Components, typename Func>
    void each(Func&& func);
    
    // Systems
    SystemManager& getSystems();
};
```

### Entity (`limbo/ecs/Entity.hpp`)

```cpp
class Entity {
public:
    Entity() = default;
    Entity(World::EntityId id, World* world);
    
    bool isValid() const;
    World::EntityId id() const;
    
    // Component access
    template<typename T, typename... Args>
    T& addComponent(Args&&... args);
    
    template<typename T>
    void removeComponent();
    
    template<typename T>
    T& getComponent();
    
    template<typename T>
    const T& getComponent() const;
    
    template<typename T>
    bool hasComponent() const;
};
```

### Built-in Components (`limbo/ecs/Components.hpp`)

```cpp
struct NameComponent {
    String name;
};

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // Euler angles in radians
    glm::vec3 scale{1.0f};
    
    glm::mat4 getMatrix() const;
};

struct SpriteRendererComponent {
    glm::vec4 color{1.0f};
    Texture2D* texture = nullptr;
    glm::vec2 uvMin{0.0f, 0.0f};
    glm::vec2 uvMax{1.0f, 1.0f};
};
```

### System (`limbo/ecs/System.hpp`)

```cpp
class System {
public:
    virtual ~System() = default;
    
    virtual void onAttach(World& world) {}
    virtual void update(World& world, f32 deltaTime) {}
    virtual void onDetach(World& world) {}
    
    void setPriority(i32 priority);
    i32 getPriority() const;
    
    void setEnabled(bool enabled);
    bool isEnabled() const;
};

class SystemManager {
public:
    template<typename T, typename... Args>
    T* addSystem(Args&&... args);
    
    template<typename T>
    T* getSystem();
    
    template<typename T>
    void removeSystem();
    
    void update(World& world, f32 deltaTime);
};
```

---

## Physics

### Physics2D (`limbo/physics/Physics2D.hpp`)

```cpp
class Physics2D {
public:
    void init(const glm::vec2& gravity = {0.0f, -9.81f});
    void shutdown();
    
    void step(f32 deltaTime);
    
    void setGravity(const glm::vec2& gravity);
    glm::vec2 getGravity() const;
    
    b2World* getWorld();
};
```

### Physics Components (`limbo/physics/PhysicsComponents.hpp`)

```cpp
enum class BodyType { Static, Kinematic, Dynamic };

struct Rigidbody2DComponent {
    BodyType type = BodyType::Dynamic;
    bool fixedRotation = false;
    b2Body* body = nullptr;  // Set by PhysicsSystem
};

struct BoxCollider2DComponent {
    glm::vec2 size{0.5f, 0.5f};
    glm::vec2 offset{0.0f, 0.0f};
    f32 density = 1.0f;
    f32 friction = 0.5f;
    f32 restitution = 0.0f;
};

struct CircleCollider2DComponent {
    f32 radius = 0.5f;
    glm::vec2 offset{0.0f, 0.0f};
    f32 density = 1.0f;
    f32 friction = 0.5f;
    f32 restitution = 0.0f;
};
```

---

## Audio

### AudioEngine (`limbo/audio/AudioEngine.hpp`)

```cpp
class AudioEngine {
public:
    bool init();
    void shutdown();
    
    void setMasterVolume(f32 volume);
    f32 getMasterVolume() const;
    
    void registerSource(AudioSource* source);
    void unregisterSource(AudioSource* source);
    
    u32 getSampleRate() const;
    u32 getChannels() const;
};
```

### AudioClip (`limbo/audio/AudioClip.hpp`)

```cpp
class AudioClip {
public:
    bool loadFromFile(const std::filesystem::path& path);
    void unload();
    
    void generateTestTone(f32 frequency, f32 duration, f32 volume = 1.0f);
    
    bool isLoaded() const;
    f32 getDuration() const;
    u32 getSampleRate() const;
    u32 getChannels() const;
};
```

### AudioSource (`limbo/audio/AudioSource.hpp`)

```cpp
enum class AudioState { Stopped, Playing, Paused };

class AudioSource {
public:
    void setClip(AudioClip* clip);
    AudioClip* getClip() const;
    
    void play();
    void pause();
    void stop();
    
    AudioState getState() const;
    
    void setVolume(f32 volume);
    f32 getVolume() const;
    
    void setLooping(bool loop);
    bool isLooping() const;
    
    void setPitch(f32 pitch);
    f32 getPitch() const;
};
```

---

## Scripting

### ScriptEngine (`limbo/scripting/ScriptEngine.hpp`)

```cpp
class ScriptEngine {
public:
    bool init();
    void shutdown();
    
    bool loadScript(const std::filesystem::path& path, World::EntityId entity);
    void unloadScript(World::EntityId entity);
    
    void callOnStart(World::EntityId entity, World& world);
    void callOnUpdate(World::EntityId entity, World& world, f32 deltaTime);
    void callOnDestroy(World::EntityId entity, World& world);
    
    sol::state& getLuaState();
};
```

### ScriptComponent (`limbo/scripting/ScriptComponent.hpp`)

```cpp
struct ScriptComponent {
    std::filesystem::path scriptPath;
    bool loaded = false;
};
```

---

## Animation

### SpriteSheet (`limbo/animation/SpriteSheet.hpp`)

```cpp
class SpriteSheet {
public:
    void setTexture(Texture2D* texture);
    Texture2D* getTexture() const;
    
    void createFromGrid(u32 frameWidth, u32 frameHeight);
    void addFrame(const glm::vec4& uvRect);  // x, y, width, height in pixels
    
    size_t getFrameCount() const;
    glm::vec4 getFrameUV(size_t index) const;  // Returns UV coords
};
```

### AnimationClip (`limbo/animation/AnimationClip.hpp`)

```cpp
enum class AnimationPlayMode { Once, Loop, PingPong };

class AnimationClip {
public:
    AnimationClip(const String& name);
    
    void setSpriteSheet(SpriteSheet* sheet);
    SpriteSheet* getSpriteSheet() const;
    
    void addFrame(u32 frameIndex, f32 duration);
    void addFrameRange(u32 startFrame, u32 endFrame, f32 frameDuration);
    
    void setPlayMode(AnimationPlayMode mode);
    void setSpeed(f32 speed);
    
    size_t getFrameCount() const;
    f32 getDuration() const;
};
```

### AnimatorComponent (`limbo/animation/AnimatorComponent.hpp`)

```cpp
struct AnimatorComponent {
    std::unordered_map<String, Shared<AnimationClip>> clips;
    String defaultClip;
    bool playOnStart = true;
    
    AnimationState currentState;
    
    void addClip(const String& name, Shared<AnimationClip> clip);
    void play(const String& clipName);
    void stop();
};
```

---

## Particles

### ParticleEmitterProps (`limbo/particles/ParticleSystem.hpp`)

```cpp
struct ParticleEmitterProps {
    glm::vec3 positionVariance{0.0f};
    glm::vec3 velocity{0.0f, 1.0f, 0.0f};
    glm::vec3 velocityVariance{0.5f, 0.5f, 0.0f};
    
    glm::vec4 colorStart{1.0f};
    glm::vec4 colorEnd{1.0f, 1.0f, 1.0f, 0.0f};
    
    f32 sizeStart = 0.1f;
    f32 sizeEnd = 0.0f;
    f32 sizeVariance = 0.0f;
    
    f32 lifetime = 1.0f;
    f32 lifetimeVariance = 0.0f;
    
    f32 rotationSpeed = 0.0f;
    f32 rotationSpeedVariance = 0.0f;
    
    f32 emissionRate = 10.0f;
};
```

### ParticleEmitterComponent (`limbo/particles/ParticleComponents.hpp`)

```cpp
struct ParticleEmitterComponent {
    ParticleEmitterProps props;
    bool enabled = true;
};
```

---

## Tilemap

### Tileset (`limbo/tilemap/Tileset.hpp`)

```cpp
class Tileset {
public:
    void create(Texture2D* texture, u32 tileWidth, u32 tileHeight);
    
    Texture2D* getTexture() const;
    u32 getTileWidth() const;
    u32 getTileHeight() const;
    u32 getTileCount() const;
    
    glm::vec4 getTileUV(u32 tileId) const;
    
    void setTileFlags(u32 tileId, TileFlags flags);
    TileFlags getTileFlags(u32 tileId) const;
};

enum class TileFlags : u8 {
    None = 0,
    Solid = 1 << 0,
    Water = 1 << 1,
    // ...
};
```

### Tilemap (`limbo/tilemap/Tilemap.hpp`)

```cpp
class Tilemap {
public:
    void create(u32 width, u32 height, f32 tileWidth, f32 tileHeight);
    void destroy();
    
    void setTileset(Tileset* tileset);
    
    u32 addLayer(const String& name, i32 zOrder = 0);
    void removeLayer(u32 layerId);
    
    void setTile(u32 layerId, u32 x, u32 y, u32 tileId);
    u32 getTile(u32 layerId, u32 x, u32 y) const;
    void fillLayer(u32 layerId, u32 tileId);
    
    u32 getWidth() const;
    u32 getHeight() const;
    glm::vec2 getWorldSize() const;
};
```

---

## UI

### Widget (`limbo/ui/Widget.hpp`)

```cpp
enum class Anchor : u8 {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight
};

class Widget {
public:
    void setPosition(const glm::vec2& pos);
    void setSize(const glm::vec2& size);
    void setAnchor(Anchor anchor);
    void setPivot(const glm::vec2& pivot);
    
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    
    void addChild(Shared<Widget> child);
    void removeChild(Widget* child);
    
    virtual void update(f32 deltaTime);
    virtual void render(const glm::vec2& screenSize);
};
```

### Concrete Widgets (`limbo/ui/Widgets.hpp`)

```cpp
class Panel : public Widget { };

class Label : public Widget {
public:
    void setText(const String& text);
    void setTextColor(const glm::vec4& color);
};

class Button : public Widget {
public:
    void setText(const String& text);
    void setOnClick(std::function<void()> callback);
};

class ProgressBar : public Widget {
public:
    void setProgress(f32 progress);  // 0.0 to 1.0
    void setFillColor(const glm::vec4& color);
};

class Image : public Widget {
public:
    void setTexture(Texture2D* texture);
    void setTint(const glm::vec4& tint);
};
```

### UICanvas (`limbo/ui/UICanvas.hpp`)

```cpp
class UICanvas {
public:
    void addWidget(Shared<Widget> widget);
    void removeWidget(Widget* widget);
    void clear();
    
    void update(f32 deltaTime);
    void render(const glm::vec2& screenSize);
    
    void setEnabled(bool enabled);
};

struct UICanvasComponent {
    Shared<UICanvas> canvas;
    bool screenSpace = true;
};
```

---

## Assets

### AssetManager (`limbo/assets/AssetManager.hpp`)

```cpp
class AssetManager {
public:
    void setAssetRoot(const std::filesystem::path& root);
    const std::filesystem::path& getAssetRoot() const;
    
    template<typename T>
    Shared<T> load(const String& path);
    
    template<typename T>
    Shared<T> get(const String& path);
    
    void unload(const String& path);
    void unloadAll();
    
    void setHotReloadEnabled(bool enabled);
    void pollHotReload();
    
    size_t assetCount() const;
};
```

---

## Scene

### SceneSerializer (`limbo/scene/SceneSerializer.hpp`)

```cpp
class SceneSerializer {
public:
    SceneSerializer(World& world);
    
    bool saveToFile(const std::filesystem::path& path);
    bool loadFromFile(const std::filesystem::path& path);
    
    String serialize();
    bool deserialize(const String& json);
    
    const String& getError() const;
};
```

---

## Application

### Application (`limbo/runtime/Application.hpp`)

```cpp
class Application {
public:
    virtual ~Application() = default;
    
    Result<void, String> init(const ApplicationConfig& config);
    void run();
    void shutdown();
    
    void requestExit();
    
    Window& getWindow();
    World& getWorld();
    SystemManager& getSystems();

protected:
    virtual void onInit() = 0;
    virtual void onUpdate(f32 deltaTime) = 0;
    virtual void onRender() = 0;
    virtual void onShutdown() = 0;
};

struct ApplicationConfig {
    String appName = "Limbo App";
    WindowConfig window;
};
```
