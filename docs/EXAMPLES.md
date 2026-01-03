# Limbo Engine Examples

This document provides code examples for common game development tasks.

## Table of Contents

1. [Player Movement](#player-movement)
2. [Platformer Physics](#platformer-physics)
3. [Top-Down Shooter](#top-down-shooter)
4. [Tilemap Level](#tilemap-level)
5. [UI Health Bar](#ui-health-bar)
6. [Particle Effects](#particle-effects)
7. [Scene Management](#scene-management)
8. [Custom Components](#custom-components)

---

## Player Movement

### Basic 4-Direction Movement

```cpp
struct PlayerComponent {
    float speed = 5.0f;
};

class PlayerMovementSystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 dt) override {
        world.each<limbo::TransformComponent, PlayerComponent>(
            [dt](auto, auto& transform, auto& player) {
                glm::vec3 velocity{0.0f};
                
                if (limbo::Input::isKeyDown(limbo::Key::W)) velocity.y += 1.0f;
                if (limbo::Input::isKeyDown(limbo::Key::S)) velocity.y -= 1.0f;
                if (limbo::Input::isKeyDown(limbo::Key::A)) velocity.x -= 1.0f;
                if (limbo::Input::isKeyDown(limbo::Key::D)) velocity.x += 1.0f;
                
                // Normalize for consistent diagonal speed
                if (glm::length(velocity) > 0.0f) {
                    velocity = glm::normalize(velocity);
                }
                
                transform.position += velocity * player.speed * dt;
            }
        );
    }
};
```

### Smooth Camera Follow

```cpp
class CameraFollowSystem : public limbo::System {
public:
    CameraFollowSystem(limbo::OrthographicCamera& camera) : m_camera(camera) {}
    
    void update(limbo::World& world, limbo::f32 dt) override {
        world.each<limbo::TransformComponent, PlayerComponent>(
            [this, dt](auto, auto& transform, auto&) {
                glm::vec3 target = transform.position;
                glm::vec3 current = m_camera.getPosition();
                
                // Smooth interpolation
                float smoothness = 5.0f;
                glm::vec3 newPos = glm::mix(current, target, smoothness * dt);
                newPos.z = 0.0f;  // Keep camera at z=0
                
                m_camera.setPosition(newPos);
            }
        );
    }

private:
    limbo::OrthographicCamera& m_camera;
};
```

---

## Platformer Physics

### Jump with Ground Check

```cpp
struct PlatformerComponent {
    float jumpForce = 8.0f;
    float moveSpeed = 5.0f;
    bool isGrounded = false;
    int jumpCount = 0;
    int maxJumps = 2;  // Double jump
};

class PlatformerSystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 dt) override {
        world.each<limbo::TransformComponent, limbo::Rigidbody2DComponent, 
                   PlatformerComponent>(
            [](auto, auto& transform, auto& rb, auto& plat) {
                if (!rb.body) return;
                
                b2Vec2 vel = rb.body->GetLinearVelocity();
                
                // Horizontal movement
                float moveInput = 0.0f;
                if (limbo::Input::isKeyDown(limbo::Key::A)) moveInput -= 1.0f;
                if (limbo::Input::isKeyDown(limbo::Key::D)) moveInput += 1.0f;
                
                vel.x = moveInput * plat.moveSpeed;
                
                // Ground check (simple: if velocity.y is nearly zero)
                plat.isGrounded = std::abs(vel.y) < 0.1f;
                
                if (plat.isGrounded) {
                    plat.jumpCount = 0;
                }
                
                // Jump
                if (limbo::Input::isKeyPressed(limbo::Key::Space)) {
                    if (plat.jumpCount < plat.maxJumps) {
                        vel.y = plat.jumpForce;
                        plat.jumpCount++;
                    }
                }
                
                rb.body->SetLinearVelocity(vel);
            }
        );
    }
};
```

---

## Top-Down Shooter

### Mouse Aiming and Shooting

```cpp
struct WeaponComponent {
    float fireRate = 0.2f;      // Seconds between shots
    float bulletSpeed = 15.0f;
    float bulletLifetime = 2.0f;
    float cooldown = 0.0f;
};

struct BulletComponent {
    float lifetime = 2.0f;
    glm::vec2 velocity{0.0f};
};

class ShootingSystem : public limbo::System {
public:
    ShootingSystem(limbo::OrthographicCamera& camera) : m_camera(camera) {}
    
    void update(limbo::World& world, limbo::f32 dt) override {
        // Update bullets
        world.each<limbo::TransformComponent, BulletComponent>(
            [&world, dt](limbo::World::EntityId id, auto& transform, auto& bullet) {
                bullet.lifetime -= dt;
                if (bullet.lifetime <= 0.0f) {
                    world.destroyEntity(id);
                    return;
                }
                
                transform.position.x += bullet.velocity.x * dt;
                transform.position.y += bullet.velocity.y * dt;
            }
        );
        
        // Handle shooting
        world.each<limbo::TransformComponent, WeaponComponent>(
            [&world, this, dt](auto, auto& transform, auto& weapon) {
                weapon.cooldown -= dt;
                
                if (limbo::Input::isMouseButtonDown(limbo::MouseButton::Left) 
                    && weapon.cooldown <= 0.0f) {
                    
                    // Get mouse position in world space
                    glm::vec2 mouseScreen = limbo::Input::getMousePosition();
                    glm::vec2 mouseWorld = screenToWorld(mouseScreen);
                    
                    // Calculate direction
                    glm::vec2 playerPos{transform.position.x, transform.position.y};
                    glm::vec2 dir = glm::normalize(mouseWorld - playerPos);
                    
                    // Spawn bullet
                    auto bullet = world.createEntity("Bullet");
                    auto& bt = bullet.addComponent<limbo::TransformComponent>();
                    bt.position = transform.position;
                    bt.scale = glm::vec3(0.1f, 0.3f, 1.0f);
                    bt.rotation.z = std::atan2(dir.y, dir.x);
                    
                    bullet.addComponent<limbo::SpriteRendererComponent>(
                        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)
                    );
                    
                    auto& bc = bullet.addComponent<BulletComponent>();
                    bc.velocity = dir * weapon.bulletSpeed;
                    bc.lifetime = weapon.bulletLifetime;
                    
                    weapon.cooldown = weapon.fireRate;
                }
            }
        );
    }

private:
    glm::vec2 screenToWorld(glm::vec2 screen) {
        // Simplified conversion - adjust based on your camera setup
        float halfWidth = 1280.0f / 2.0f;
        float halfHeight = 720.0f / 2.0f;
        
        glm::vec3 camPos = m_camera.getPosition();
        float zoom = 1.0f;  // Get from camera
        
        float worldX = (screen.x - halfWidth) / halfWidth * zoom + camPos.x;
        float worldY = (halfHeight - screen.y) / halfHeight * zoom + camPos.y;
        
        return {worldX, worldY};
    }
    
    limbo::OrthographicCamera& m_camera;
};
```

---

## Tilemap Level

### Creating a Tilemap from Code

```cpp
void createLevel() {
    // Create tileset texture (or load from file)
    m_tilesetTexture = limbo::Texture2D::load("assets/textures/tileset.png");
    
    // Setup tileset (assuming 16x16 tiles in a 64x64 texture)
    m_tileset = std::make_shared<limbo::Tileset>();
    m_tileset->create(m_tilesetTexture.get(), 16, 16);
    
    // Mark solid tiles
    m_tileset->setTileFlags(1, limbo::TileFlags::Solid);  // Wall
    m_tileset->setTileFlags(2, limbo::TileFlags::Solid);  // Ground
    
    // Create tilemap
    m_tilemap = std::make_shared<limbo::Tilemap>();
    m_tilemap->create(20, 15, 1.0f, 1.0f);  // 20x15 tiles, 1 unit each
    m_tilemap->setTileset(m_tileset.get());
    
    // Add layers
    uint32_t bgLayer = m_tilemap->addLayer("background", 0);
    uint32_t fgLayer = m_tilemap->addLayer("foreground", 1);
    
    // Fill background with grass
    m_tilemap->fillLayer(bgLayer, 0);
    
    // Create ground
    for (uint32_t x = 0; x < 20; ++x) {
        m_tilemap->setTile(fgLayer, x, 0, 2);  // Ground tiles at bottom
    }
    
    // Add some walls
    for (uint32_t y = 1; y < 5; ++y) {
        m_tilemap->setTile(fgLayer, 5, y, 1);
        m_tilemap->setTile(fgLayer, 15, y, 1);
    }
    
    // Create entity
    auto tilemapEntity = getWorld().createEntity("Level");
    tilemapEntity.addComponent<limbo::TransformComponent>();
    auto& tmComp = tilemapEntity.addComponent<limbo::TilemapComponent>();
    tmComp.tilemap = m_tilemap;
    tmComp.tileset = m_tileset;
}
```

### Tile Collision Detection

```cpp
bool isTileSolid(const limbo::Tilemap& tilemap, const limbo::Tileset& tileset,
                  float worldX, float worldY) {
    // Convert world position to tile coordinates
    int tileX = static_cast<int>(worldX / tilemap.getTileWidth());
    int tileY = static_cast<int>(worldY / tilemap.getTileHeight());
    
    // Check bounds
    if (tileX < 0 || tileX >= tilemap.getWidth() ||
        tileY < 0 || tileY >= tilemap.getHeight()) {
        return true;  // Out of bounds = solid
    }
    
    // Check foreground layer (layer 1)
    uint32_t tileId = tilemap.getTile(1, tileX, tileY);
    
    return (tileset.getTileFlags(tileId) & limbo::TileFlags::Solid) != 
           limbo::TileFlags::None;
}
```

---

## UI Health Bar

### Health Component and UI

```cpp
struct HealthComponent {
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;
    
    float getPercent() const { return currentHealth / maxHealth; }
};

class HealthUISystem : public limbo::System {
public:
    HealthUISystem(std::shared_ptr<limbo::ProgressBar> healthBar)
        : m_healthBar(healthBar) {}
    
    void update(limbo::World& world, limbo::f32 dt) override {
        // Find player health
        world.each<HealthComponent, PlayerComponent>(
            [this](auto, auto& health, auto&) {
                m_healthBar->setProgress(health.getPercent());
                
                // Change color based on health
                if (health.getPercent() > 0.5f) {
                    m_healthBar->setFillColor({0.2f, 0.8f, 0.2f, 1.0f});
                } else if (health.getPercent() > 0.25f) {
                    m_healthBar->setFillColor({0.8f, 0.8f, 0.2f, 1.0f});
                } else {
                    m_healthBar->setFillColor({0.8f, 0.2f, 0.2f, 1.0f});
                }
            }
        );
    }

private:
    std::shared_ptr<limbo::ProgressBar> m_healthBar;
};

// Setup in onInit:
void setupHealthUI() {
    m_uiCanvas = std::make_shared<limbo::UICanvas>();
    
    auto healthBar = std::make_shared<limbo::ProgressBar>();
    healthBar->setPosition({20.0f, 20.0f});
    healthBar->setSize({200.0f, 25.0f});
    healthBar->setAnchor(limbo::Anchor::TopLeft);
    healthBar->setProgress(1.0f);
    healthBar->setFillColor({0.2f, 0.8f, 0.2f, 1.0f});
    
    m_uiCanvas->addWidget(healthBar);
    
    getSystems().addSystem<HealthUISystem>(healthBar);
}
```

---

## Particle Effects

### Explosion Effect

```cpp
void spawnExplosion(limbo::World& world, const glm::vec3& position) {
    auto emitter = world.createEntity("Explosion");
    
    auto& transform = emitter.addComponent<limbo::TransformComponent>();
    transform.position = position;
    
    auto& particles = emitter.addComponent<limbo::ParticleEmitterComponent>();
    particles.props.velocity = glm::vec3(0.0f);
    particles.props.velocityVariance = glm::vec3(5.0f, 5.0f, 0.0f);
    particles.props.colorStart = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    particles.props.colorEnd = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
    particles.props.sizeStart = 0.3f;
    particles.props.sizeEnd = 0.0f;
    particles.props.lifetime = 0.5f;
    particles.props.lifetimeVariance = 0.2f;
    particles.props.emissionRate = 100.0f;
    
    // Add a self-destruct component
    auto& selfDestruct = emitter.addComponent<TimedDestroyComponent>();
    selfDestruct.timeRemaining = 0.6f;  // Slightly longer than particle lifetime
}
```

### Trail Effect

```cpp
void setupTrailEmitter(limbo::Entity entity) {
    auto& particles = entity.addComponent<limbo::ParticleEmitterComponent>();
    
    particles.props.positionVariance = glm::vec3(0.1f, 0.1f, 0.0f);
    particles.props.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    particles.props.velocityVariance = glm::vec3(0.2f, 0.2f, 0.0f);
    particles.props.colorStart = glm::vec4(0.5f, 0.8f, 1.0f, 0.8f);
    particles.props.colorEnd = glm::vec4(0.2f, 0.4f, 1.0f, 0.0f);
    particles.props.sizeStart = 0.1f;
    particles.props.sizeEnd = 0.02f;
    particles.props.lifetime = 0.3f;
    particles.props.emissionRate = 30.0f;
}
```

---

## Scene Management

### Scene Transition

```cpp
class SceneManager {
public:
    void loadScene(const std::string& scenePath, limbo::World& world) {
        // Clear current scene
        world.clear();
        
        // Load new scene
        limbo::SceneSerializer serializer(world);
        if (!serializer.loadFromFile(scenePath)) {
            spdlog::error("Failed to load scene: {}", serializer.getError());
            return;
        }
        
        m_currentScene = scenePath;
        spdlog::info("Loaded scene: {}", scenePath);
    }
    
    void saveCurrentScene(limbo::World& world) {
        if (m_currentScene.empty()) {
            spdlog::warn("No current scene to save");
            return;
        }
        
        limbo::SceneSerializer serializer(world);
        if (serializer.saveToFile(m_currentScene)) {
            spdlog::info("Saved scene: {}", m_currentScene);
        } else {
            spdlog::error("Failed to save: {}", serializer.getError());
        }
    }

private:
    std::string m_currentScene;
};

// Usage:
SceneManager sceneManager;
sceneManager.loadScene("assets/scenes/level1.json", getWorld());
```

---

## Custom Components

### Complete Enemy AI Example

```cpp
// Components
struct EnemyComponent {
    float detectionRange = 5.0f;
    float attackRange = 1.5f;
    float speed = 2.0f;
    
    enum class State { Idle, Chase, Attack } state = State::Idle;
};

struct HealthComponent {
    float max = 50.0f;
    float current = 50.0f;
    
    void takeDamage(float amount) {
        current = std::max(0.0f, current - amount);
    }
    
    bool isDead() const { return current <= 0.0f; }
};

// System
class EnemyAISystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 dt) override {
        // Find player position
        glm::vec3 playerPos{0.0f};
        bool playerFound = false;
        
        world.each<limbo::TransformComponent, PlayerComponent>(
            [&playerPos, &playerFound](auto, auto& transform, auto&) {
                playerPos = transform.position;
                playerFound = true;
            }
        );
        
        if (!playerFound) return;
        
        // Update enemies
        world.each<limbo::TransformComponent, EnemyComponent, HealthComponent>(
            [&world, &playerPos, dt](limbo::World::EntityId id, 
                                      auto& transform, auto& enemy, auto& health) {
                if (health.isDead()) {
                    world.destroyEntity(id);
                    return;
                }
                
                float distance = glm::distance(
                    glm::vec2(transform.position), 
                    glm::vec2(playerPos)
                );
                
                // State machine
                switch (enemy.state) {
                    case EnemyComponent::State::Idle:
                        if (distance < enemy.detectionRange) {
                            enemy.state = EnemyComponent::State::Chase;
                        }
                        break;
                        
                    case EnemyComponent::State::Chase:
                        if (distance > enemy.detectionRange * 1.5f) {
                            enemy.state = EnemyComponent::State::Idle;
                        } else if (distance < enemy.attackRange) {
                            enemy.state = EnemyComponent::State::Attack;
                        } else {
                            // Move towards player
                            glm::vec2 dir = glm::normalize(
                                glm::vec2(playerPos) - glm::vec2(transform.position)
                            );
                            transform.position.x += dir.x * enemy.speed * dt;
                            transform.position.y += dir.y * enemy.speed * dt;
                        }
                        break;
                        
                    case EnemyComponent::State::Attack:
                        if (distance > enemy.attackRange) {
                            enemy.state = EnemyComponent::State::Chase;
                        }
                        // Attack logic here (damage player, play animation, etc.)
                        break;
                }
            }
        );
    }
};

// Spawn an enemy
void spawnEnemy(limbo::World& world, const glm::vec3& position) {
    auto enemy = world.createEntity("Enemy");
    
    auto& transform = enemy.addComponent<limbo::TransformComponent>();
    transform.position = position;
    transform.scale = glm::vec3(0.8f);
    
    enemy.addComponent<limbo::SpriteRendererComponent>(
        glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)
    );
    
    enemy.addComponent<EnemyComponent>();
    enemy.addComponent<HealthComponent>();
}
```
