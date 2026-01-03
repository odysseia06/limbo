# Limbo Engine 3D Architecture Design

This document outlines the architecture for extending Limbo from a 2D game engine to support both 2D and 3D rendering, physics, and gameplay.

## Design Philosophy

1. **Parallel, Not Replacement**: 3D systems exist alongside 2D systems, not replacing them
2. **Shared Foundation**: Common infrastructure (ECS, assets, platform) serves both
3. **Incremental Adoption**: Projects can use 2D-only, 3D-only, or mixed rendering
4. **Clean Separation**: 2D and 3D code in separate directories with clear boundaries

## Directory Structure

```
engine/include/limbo/
├── core/                    # Generic foundation
│   ├── Types.hpp
│   ├── Base.hpp
│   └── Assert.hpp
│
├── ecs/                     # Entity Component System (generic)
│   ├── World.hpp
│   ├── Entity.hpp
│   ├── System.hpp
│   └── Components.hpp       # TransformComponent, NameComponent, CameraComponent
│
├── render/
│   ├── common/              # Shared rendering infrastructure
│   │   ├── Buffer.hpp       # VAO, VBO, IBO abstractions
│   │   ├── Shader.hpp       # Shader program management
│   │   ├── Texture.hpp      # 2D textures (used by both 2D and 3D)
│   │   ├── Camera.hpp       # OrthographicCamera, PerspectiveCamera
│   │   ├── RenderContext.hpp
│   │   └── RenderCommand.hpp # Low-level draw commands
│   │
│   ├── 2d/                  # 2D-specific rendering
│   │   ├── Renderer2D.hpp   # Batched quad/sprite rendering
│   │   └── SpriteRenderSystem.hpp
│   │
│   └── 3d/                  # 3D-specific rendering
│       ├── Renderer3D.hpp   # Mesh rendering pipeline
│       ├── Mesh.hpp         # Vertex data, submeshes
│       ├── Material.hpp     # Shader + uniforms + textures
│       ├── Model.hpp        # Mesh + materials hierarchy
│       ├── Lighting.hpp     # Light types and management
│       ├── Skybox.hpp       # Environment mapping
│       └── MeshRenderSystem.hpp
│
├── physics/
│   ├── 2d/                  # Box2D wrapper (existing)
│   │   ├── Physics2D.hpp
│   │   ├── PhysicsComponents2D.hpp
│   │   └── PhysicsSystem2D.hpp
│   │
│   └── 3d/                  # 3D physics (future)
│       ├── Physics3D.hpp    # Jolt/Bullet wrapper
│       ├── PhysicsComponents3D.hpp
│       └── PhysicsSystem3D.hpp
│
├── animation/
│   ├── 2d/                  # Sprite animation (existing)
│   │   ├── SpriteSheet.hpp
│   │   ├── Animation.hpp
│   │   └── AnimatorComponent.hpp
│   │
│   └── 3d/                  # Skeletal animation (future)
│       ├── Skeleton.hpp
│       ├── SkeletalAnimation.hpp
│       └── SkeletalAnimator.hpp
│
├── assets/                  # Generic asset system
│   ├── AssetManager.hpp
│   ├── Asset.hpp
│   ├── TextureAsset.hpp
│   ├── ShaderAsset.hpp
│   ├── AudioAsset.hpp
│   ├── MeshAsset.hpp        # NEW: 3D mesh loading
│   └── MaterialAsset.hpp    # NEW: Material definitions
│
├── audio/                   # Generic audio (unchanged)
├── scripting/               # Generic scripting (unchanged)
├── ui/                      # Generic UI (unchanged)
├── scene/                   # Scene management (unchanged)
├── platform/                # Platform abstraction (unchanged)
└── runtime/                 # Application framework (unchanged)
```

## Component Architecture

### Shared Components (ecs/Components.hpp)

```cpp
// Already 3D-ready
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // Euler angles (pitch, yaw, roll)
    glm::vec3 scale{1.0f};
    glm::mat4 getMatrix() const;
};

struct NameComponent {
    std::string name;
};

struct CameraComponent {
    enum class ProjectionType { Perspective, Orthographic };
    ProjectionType projectionType = ProjectionType::Perspective;
    float fov = 45.0f;           // For perspective
    float orthoSize = 10.0f;     // For orthographic
    float nearClip = 0.1f;
    float farClip = 1000.0f;
};
```

### 2D Components (render/2d/ or ecs/components/)

```cpp
struct SpriteRendererComponent {
    glm::vec4 color{1.0f};
    AssetId textureId;
    glm::vec4 uvRect{0, 0, 1, 1};
    int sortingOrder = 0;
};

struct Rigidbody2DComponent { /* Box2D body */ };
struct BoxCollider2DComponent { /* Box2D fixture */ };
struct CircleCollider2DComponent { /* Box2D fixture */ };
```

### 3D Components (render/3d/ or ecs/components/)

```cpp
struct MeshRendererComponent {
    AssetId meshId;
    AssetId materialId;
    bool castShadows = true;
    bool receiveShadows = true;
};

struct MeshFilterComponent {
    AssetId meshId;
    u32 submeshIndex = 0;
};

struct LightComponent {
    enum class Type { Directional, Point, Spot };
    Type type = Type::Point;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;        // Point/Spot
    float spotAngle = 45.0f;    // Spot only
    bool castShadows = false;
};

struct Rigidbody3DComponent {
    enum class Type { Static, Kinematic, Dynamic };
    Type type = Type::Dynamic;
    float mass = 1.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.05f;
    bool useGravity = true;
    void* runtimeBody = nullptr;  // Physics engine body
};

struct BoxCollider3DComponent {
    glm::vec3 halfExtents{0.5f};
    glm::vec3 offset{0.0f};
    float friction = 0.5f;
    float restitution = 0.0f;
    bool isTrigger = false;
};

struct SphereCollider3DComponent {
    float radius = 0.5f;
    glm::vec3 offset{0.0f};
    float friction = 0.5f;
    float restitution = 0.0f;
    bool isTrigger = false;
};

struct CapsuleCollider3DComponent {
    float radius = 0.5f;
    float height = 2.0f;
    glm::vec3 offset{0.0f};
};

struct MeshCollider3DComponent {
    AssetId meshId;
    bool convex = false;  // Convex for dynamic, concave for static only
};
```

## Rendering Architecture

### Renderer3D Design

```cpp
namespace limbo {

class Renderer3D {
public:
    static void init();
    static void shutdown();
    
    static void beginScene(const PerspectiveCamera& camera);
    static void beginScene(const glm::mat4& viewProjection);
    static void endScene();
    
    // Core mesh rendering
    static void submit(const Mesh& mesh, const Material& material, 
                       const glm::mat4& transform);
    static void submit(const Model& model, const glm::mat4& transform);
    
    // Instanced rendering for performance
    static void submitInstanced(const Mesh& mesh, const Material& material,
                                const std::vector<glm::mat4>& transforms);
    
    // Lighting
    static void setAmbientLight(const glm::vec3& color, float intensity);
    static void submitLight(const LightComponent& light, const glm::vec3& position,
                           const glm::vec3& direction = {0, -1, 0});
    
    // Environment
    static void setSkybox(const Cubemap& skybox);
    
    // Debug rendering
    static void drawLine(const glm::vec3& start, const glm::vec3& end,
                        const glm::vec4& color = {1, 1, 1, 1});
    static void drawWireBox(const glm::vec3& center, const glm::vec3& size,
                           const glm::vec4& color = {1, 1, 1, 1});
    static void drawWireSphere(const glm::vec3& center, float radius,
                              const glm::vec4& color = {1, 1, 1, 1});

private:
    struct SceneData {
        glm::mat4 viewProjection;
        glm::vec3 cameraPosition;
        std::vector<LightData> lights;
        glm::vec3 ambientColor;
        float ambientIntensity;
    };
    
    static SceneData s_sceneData;
};

}  // namespace limbo
```

### Material System

```cpp
namespace limbo {

struct MaterialProperty {
    std::string name;
    std::variant<float, glm::vec2, glm::vec3, glm::vec4, 
                 i32, AssetId> value;
};

class Material {
public:
    Material(Shared<Shader> shader);
    
    void setShader(Shared<Shader> shader);
    Shader& getShader() const;
    
    // Property setters
    void setFloat(const std::string& name, float value);
    void setVector(const std::string& name, const glm::vec4& value);
    void setTexture(const std::string& name, AssetId textureId);
    
    // Bind material for rendering
    void bind() const;
    
    // Common material presets
    static Shared<Material> createPBR();      // Physically-based
    static Shared<Material> createUnlit();    // No lighting
    static Shared<Material> createPhong();    // Classic Phong

private:
    Shared<Shader> m_shader;
    std::unordered_map<std::string, MaterialProperty> m_properties;
    std::unordered_map<std::string, AssetId> m_textures;
};

}  // namespace limbo
```

### Mesh and Model

```cpp
namespace limbo {

struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    // For skeletal animation (future)
    glm::ivec4 boneIds{-1};
    glm::vec4 boneWeights{0.0f};
};

struct Submesh {
    u32 baseVertex = 0;
    u32 baseIndex = 0;
    u32 indexCount = 0;
    u32 materialIndex = 0;
    AABB boundingBox;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex3D>& vertices,
         const std::vector<u32>& indices);
    
    void addSubmesh(const Submesh& submesh);
    
    const std::vector<Submesh>& getSubmeshes() const;
    const AABB& getBoundingBox() const;
    
    void bind() const;
    void unbind() const;
    
    // Primitive generators
    static Shared<Mesh> createCube(float size = 1.0f);
    static Shared<Mesh> createSphere(float radius = 0.5f, u32 segments = 32);
    static Shared<Mesh> createPlane(float width = 1.0f, float height = 1.0f);
    static Shared<Mesh> createCylinder(float radius = 0.5f, float height = 1.0f);
    static Shared<Mesh> createCapsule(float radius = 0.5f, float height = 2.0f);

private:
    Unique<VertexArray> m_vertexArray;
    std::vector<Submesh> m_submeshes;
    AABB m_boundingBox;
};

class Model {
public:
    Model(const std::string& filepath);  // Load from file (glTF, OBJ, FBX)
    
    const std::vector<Shared<Mesh>>& getMeshes() const;
    const std::vector<Shared<Material>>& getMaterials() const;
    
    // Scene hierarchy (for complex models)
    struct Node {
        std::string name;
        glm::mat4 transform;
        std::vector<u32> meshIndices;
        std::vector<Unique<Node>> children;
    };
    
    const Node& getRootNode() const;

private:
    std::vector<Shared<Mesh>> m_meshes;
    std::vector<Shared<Material>> m_materials;
    Unique<Node> m_rootNode;
};

}  // namespace limbo
```

### Lighting System

```cpp
namespace limbo {

struct DirectionalLight {
    glm::vec3 direction{0, -1, 0};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    // Attenuation: constant, linear, quadratic
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0, -1, 0};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerCutoff = 12.5f;  // degrees
    float outerCutoff = 17.5f;  // degrees
};

class LightingSystem : public System {
public:
    void update(float deltaTime) override;
    
    void setAmbientLight(const glm::vec3& color, float intensity);
    
    // Gathered each frame from LightComponent entities
    const std::vector<DirectionalLight>& getDirectionalLights() const;
    const std::vector<PointLight>& getPointLights() const;
    const std::vector<SpotLight>& getSpotLights() const;

private:
    glm::vec3 m_ambientColor{0.1f};
    float m_ambientIntensity = 1.0f;
    
    std::vector<DirectionalLight> m_directionalLights;
    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
};

}  // namespace limbo
```

## Physics Architecture

### Physics3D Engine Wrapper

Recommended: **Jolt Physics** (modern, performant, MIT license) or **Bullet Physics** (mature, widely used)

```cpp
namespace limbo {

class Physics3D {
public:
    Physics3D(const glm::vec3& gravity = {0, -9.81f, 0});
    ~Physics3D();
    
    void step(float deltaTime);
    
    void setGravity(const glm::vec3& gravity);
    glm::vec3 getGravity() const;
    
    // Raycasting
    struct RaycastHit {
        Entity entity;
        glm::vec3 point;
        glm::vec3 normal;
        float distance;
    };
    
    bool raycast(const glm::vec3& origin, const glm::vec3& direction,
                 float maxDistance, RaycastHit& outHit);
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin,
                                        const glm::vec3& direction,
                                        float maxDistance);
    
    // Shape queries
    std::vector<Entity> overlapSphere(const glm::vec3& center, float radius);
    std::vector<Entity> overlapBox(const glm::vec3& center, 
                                   const glm::vec3& halfExtents);

private:
    // Jolt/Bullet physics world
    void* m_physicsSystem = nullptr;
};

class PhysicsSystem3D : public System {
public:
    PhysicsSystem3D(Physics3D& physics);
    
    void update(float deltaTime) override;
    void fixedUpdate(float fixedDeltaTime) override;

private:
    Physics3D& m_physics;
    
    void createBody(Entity entity);
    void destroyBody(Entity entity);
    void syncTransforms();
};

}  // namespace limbo
```

## Asset Loading

### Mesh Asset

```cpp
namespace limbo {

class MeshAsset : public Asset {
public:
    bool load(const std::string& filepath) override;
    void unload() override;
    
    Shared<Mesh> getMesh() const { return m_mesh; }
    
    // Supported formats: .obj, .gltf, .glb, .fbx
    static std::vector<std::string> getSupportedExtensions();

private:
    Shared<Mesh> m_mesh;
    
    bool loadOBJ(const std::string& filepath);
    bool loadGLTF(const std::string& filepath);
};

}  // namespace limbo
```

### Recommended Libraries for 3D

| Feature | Library | License |
|---------|---------|---------|
| Mesh Loading | **Assimp** or **tinygltf** | BSD / MIT |
| Physics | **Jolt Physics** | MIT |
| Image Loading | stb_image (existing) | Public Domain |
| Math | GLM (existing) | MIT |

## Shaders

### Standard 3D Shaders

```
assets/shaders/
├── 3d/
│   ├── pbr.vert              # PBR vertex shader
│   ├── pbr.frag              # PBR fragment shader
│   ├── phong.vert            # Classic Phong
│   ├── phong.frag
│   ├── unlit.vert            # No lighting
│   ├── unlit.frag
│   ├── skybox.vert           # Skybox rendering
│   ├── skybox.frag
│   ├── shadow_depth.vert     # Shadow map generation
│   ├── shadow_depth.frag
│   └── debug_line.vert/.frag # Debug visualization
└── 2d/
    ├── sprite.vert           # Existing 2D shaders
    └── sprite.frag
```

## Render Pipeline

### Forward Rendering (Initial Implementation)

```
1. Clear buffers
2. Update camera uniforms
3. Render shadow maps (optional)
4. For each light:
   - Bind light uniforms
   - For each mesh in frustum:
     - Bind material
     - Bind mesh VAO
     - Draw
5. Render skybox
6. Render transparent objects (back-to-front)
7. Post-processing (future)
8. Render 2D overlay/UI
```

### Deferred Rendering (Future Enhancement)

```
1. Geometry pass → G-Buffer (position, normal, albedo, metallic/roughness)
2. Lighting pass → Combine with lights
3. Forward pass for transparent objects
4. Post-processing
```

## Migration Path

### Phase 1: Restructure (Current)
- Move 2D code to render/2d/, physics/2d/
- Create empty 3d/ directories
- Update all includes
- Verify 2D still works

### Phase 2: 3D Foundation
- Implement Mesh, Material, Renderer3D basics
- Add MeshAsset loader (OBJ format first)
- Basic forward rendering with single light
- MeshRendererComponent and MeshRenderSystem

### Phase 3: Lighting
- Multiple light types (directional, point, spot)
- LightComponent and LightingSystem
- Basic shadow mapping

### Phase 4: Physics
- Integrate Jolt Physics
- 3D rigidbody and collider components
- PhysicsSystem3D

### Phase 5: Advanced Features
- PBR materials
- Skeletal animation
- Deferred rendering
- Post-processing effects

## Example Usage

### 2D Game (unchanged)
```cpp
class My2DGame : public Application {
    void onInit() override {
        // Create sprite entity
        auto player = m_world.createEntity("Player");
        player.addComponent<TransformComponent>();
        player.addComponent<SpriteRendererComponent>();
        player.addComponent<Rigidbody2DComponent>();
        
        // Add 2D systems
        m_systemManager.addSystem<SpriteRenderSystem>();
        m_systemManager.addSystem<PhysicsSystem2D>(m_physics2D);
    }
};
```

### 3D Game
```cpp
class My3DGame : public Application {
    void onInit() override {
        // Create 3D entity
        auto cube = m_world.createEntity("Cube");
        cube.addComponent<TransformComponent>();
        cube.addComponent<MeshRendererComponent>(meshId, materialId);
        cube.addComponent<Rigidbody3DComponent>();
        cube.addComponent<BoxCollider3DComponent>();
        
        // Create light
        auto sun = m_world.createEntity("Sun");
        sun.addComponent<TransformComponent>();
        sun.addComponent<LightComponent>(LightComponent::Type::Directional);
        
        // Add 3D systems
        m_systemManager.addSystem<MeshRenderSystem>();
        m_systemManager.addSystem<LightingSystem>();
        m_systemManager.addSystem<PhysicsSystem3D>(m_physics3D);
    }
};
```

### Mixed 2D/3D
```cpp
class MyMixedGame : public Application {
    void onInit() override {
        // 3D world
        auto mesh = m_world.createEntity("3DMesh");
        mesh.addComponent<MeshRendererComponent>();
        
        // 2D UI overlay
        auto uiSprite = m_world.createEntity("UISprite");
        uiSprite.addComponent<SpriteRendererComponent>();
        
        // Both systems
        m_systemManager.addSystem<MeshRenderSystem>();
        m_systemManager.addSystem<SpriteRenderSystem>();  // Renders last, as overlay
    }
};
```

## Conclusion

This architecture allows Limbo to grow into a full 2D+3D engine while:
- Preserving all existing 2D functionality
- Sharing common infrastructure (ECS, assets, platform)
- Enabling clean separation of 2D and 3D code
- Supporting incremental feature development
- Allowing games to use either or both paradigms
