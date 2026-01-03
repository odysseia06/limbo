#include <limbo/Limbo.hpp>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>

// A simple velocity component for demo purposes
struct VelocityComponent {
    glm::vec3 velocity{0.0f};

    VelocityComponent() = default;
    explicit VelocityComponent(const glm::vec3& vel) : velocity(vel) {}
};

// A simple movement system that updates positions based on velocity
class MovementSystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 deltaTime) override {
        // Iterate over all entities with Transform and Velocity components
        world.each<limbo::TransformComponent, VelocityComponent>(
            [deltaTime](limbo::World::EntityId, limbo::TransformComponent& transform,
                        VelocityComponent& velocity) {
                transform.position += velocity.velocity * deltaTime;
            });
    }
};

// A simple bounce system that reverses velocity at boundaries
class BounceSystem : public limbo::System {
public:
    BounceSystem() {
        // Run after movement system
        setPriority(10);
    }

    void update(limbo::World& world, limbo::f32 /*deltaTime*/) override {
        world.each<limbo::TransformComponent, VelocityComponent>(
            [](limbo::World::EntityId, limbo::TransformComponent& transform,
               VelocityComponent& velocity) {
                // Bounce off horizontal boundaries
                if (transform.position.x > 1.5f || transform.position.x < -1.5f) {
                    velocity.velocity.x = -velocity.velocity.x;
                    transform.position.x = glm::clamp(transform.position.x, -1.5f, 1.5f);
                }
                // Bounce off vertical boundaries
                if (transform.position.y > 1.0f || transform.position.y < -1.0f) {
                    velocity.velocity.y = -velocity.velocity.y;
                    transform.position.y = glm::clamp(transform.position.y, -1.0f, 1.0f);
                }
            });
    }
};

// A rotation system that spins entities with a SpriteRendererComponent (non-physics only)
class RotationSystem : public limbo::System {
public:
    void update(limbo::World& world, limbo::f32 deltaTime) override {
        // Only rotate entities that don't have physics (physics bodies handle their own rotation)
        world.each<limbo::TransformComponent, limbo::SpriteRendererComponent>(
            [&world, deltaTime](limbo::World::EntityId entity, limbo::TransformComponent& transform,
                                limbo::SpriteRendererComponent&) {
                if (!world.hasComponent<limbo::Rigidbody2DComponent>(entity)) {
                    transform.rotation.z += deltaTime * 2.0f;
                }
            });
    }
};

class SandboxApp : public limbo::Application {
public:
    SandboxApp() = default;

protected:
    void onInit() override {
        spdlog::info("Sandbox initialized");

        // Create render context
        m_renderContext = limbo::RenderContext::create();
        if (!m_renderContext->init(getWindow())) {
            spdlog::critical("Failed to initialize render context");
            requestExit();
            return;
        }

        // Initialize Renderer2D
        limbo::Renderer2D::init();

        // Initialize ImGui
        if (!m_imguiLayer.init(getWindow().getNativeHandle())) {
            spdlog::error("Failed to initialize ImGui");
        }

        // Initialize camera
        float aspect = static_cast<float>(getWindow().getWidth()) /
                       static_cast<float>(getWindow().getHeight());
        m_camera = limbo::OrthographicCamera(-aspect * m_zoom, aspect * m_zoom, -m_zoom, m_zoom);

        // Initialize physics
        m_physics.init({0.0f, -9.81f});

        // Initialize audio
        if (!m_audioEngine.init()) {
            spdlog::error("Failed to initialize audio engine");
        }

        // Initialize scripting
        if (!m_scriptEngine.init()) {
            spdlog::error("Failed to initialize script engine");
        }

        // Setup AssetManager
        setupAssets();

        // Setup ECS systems
        setupSystems();

        // Create entities
        createEntities();

        spdlog::info("Rendering setup complete");
        spdlog::info("Controls:");
        spdlog::info("  WASD/Arrow keys - Move camera");
        spdlog::info("  Q/E - Rotate camera");
        spdlog::info("  Mouse scroll - Zoom in/out");
        spdlog::info("  Space - Reset camera");
        spdlog::info("  Escape - Exit");
        spdlog::info("");
        spdlog::info("ECS Demo: {} entities created", getWorld().entityCount());
        spdlog::info("Assets loaded: {}", m_assetManager.assetCount());
        spdlog::info("Press F1 to toggle ImGui panels");
    }

    void setupAssets() {
        // Set asset root to the sandbox assets directory
        // Try to find the assets folder relative to the executable
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path assetsPath = exePath / "apps" / "sandbox" / "assets";

        // Fallback paths for different build configurations
        if (!std::filesystem::exists(assetsPath)) {
            assetsPath = exePath / "assets";
        }
        if (!std::filesystem::exists(assetsPath)) {
            assetsPath = exePath.parent_path() / "apps" / "sandbox" / "assets";
        }
        if (!std::filesystem::exists(assetsPath)) {
            // Try relative to source directory (for development)
            assetsPath = "C:/dev/limbo/apps/sandbox/assets";
        }

        if (std::filesystem::exists(assetsPath)) {
            m_assetManager.setAssetRoot(assetsPath);
            spdlog::info("Asset root set to: {}", assetsPath.string());

            // Enable hot-reloading for development
            m_assetManager.setHotReloadEnabled(true);
            spdlog::info("Hot-reload enabled");

            // Load shader asset
            auto shaderAsset = m_assetManager.load<limbo::ShaderAsset>("shaders/sprite");
            if (shaderAsset && shaderAsset->getState() == limbo::AssetState::Loaded) {
                spdlog::info("Loaded shader asset: shaders/sprite");
            } else {
                spdlog::warn("Failed to load shader asset");
            }

            // Load texture asset
            m_checkerboardTexture =
                m_assetManager.load<limbo::TextureAsset>("textures/checkerboard.png");
            if (m_checkerboardTexture &&
                m_checkerboardTexture->getState() == limbo::AssetState::Loaded) {
                spdlog::info("Loaded texture asset: textures/checkerboard.png ({}x{})",
                             m_checkerboardTexture->getWidth(), m_checkerboardTexture->getHeight());
            } else {
                spdlog::warn("Failed to load texture asset");
            }
        } else {
            spdlog::warn("Assets directory not found, using default path");
        }
    }

    void setupSystems() {
        // Add systems - they will be initialized by Application after onInit
        // These are logic systems that run during update phase
        getSystems().addSystem<MovementSystem>();
        getSystems().addSystem<BounceSystem>();
        getSystems().addSystem<RotationSystem>();

        // Physics system - runs after other systems
        getSystems().addSystem<limbo::PhysicsSystem2D>(m_physics)->setPriority(100);

        // Audio system
        getSystems().addSystem<limbo::AudioSystem>(m_audioEngine)->setPriority(50);

        // Animation system - runs before rendering to update sprite UVs
        getSystems().addSystem<limbo::AnimationSystem>()->setPriority(90);

        // Scripting system - runs Lua scripts
        getSystems().addSystem<limbo::ScriptSystem>(m_scriptEngine)->setPriority(5);

        // Particle system - updates and manages particles
        m_particleSystem = getSystems().addSystem<limbo::ParticleRenderSystem>(5000);
        m_particleSystem->setPriority(95);

        // Tilemap system - renders tilemaps
        m_tilemapSystem = getSystems().addSystem<limbo::TilemapRenderSystem>();
        m_tilemapSystem->setCamera(&m_camera);
        m_tilemapSystem->setPriority(85);

        // UI system - renders in-game UI
        m_uiSystem = getSystems().addSystem<limbo::UISystem>();
        m_uiSystem->setPriority(200);  // Render after everything else
        m_uiSystem->setScreenSize(glm::vec2(static_cast<float>(getWindow().getWidth()),
                                            static_cast<float>(getWindow().getHeight())));

        // Note: SpriteRenderSystem is NOT added here because rendering
        // needs to happen in onRender() after the screen is cleared
    }

    void createEntities() {
        auto& world = getWorld();

        // Create several bouncing sprites
        for (int i = 0; i < 20; ++i) {
            limbo::Entity entity = world.createEntity(limbo::String("Sprite_") + std::to_string(i));

            // Add transform with random-ish starting position
            auto& transform = entity.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3((static_cast<float>(i % 5) - 2.0f) * 0.6f,
                                           (static_cast<float>(i / 5) - 2.0f) * 0.4f, 0.0f);
            transform.scale = glm::vec3(0.15f);

            // Add velocity for movement
            float speedX = 0.2f + static_cast<float>(i % 7) * 0.05f;
            float speedY = 0.15f + static_cast<float>(i % 5) * 0.05f;
            if (i % 2 == 0)
                speedX = -speedX;
            if (i % 3 == 0)
                speedY = -speedY;
            entity.addComponent<VelocityComponent>(glm::vec3(speedX, speedY, 0.0f));

            // Add sprite renderer with different colors
            glm::vec4 colors[] = {
                {1.0f, 0.3f, 0.3f, 1.0f},  // Red
                {0.3f, 1.0f, 0.3f, 1.0f},  // Green
                {0.3f, 0.3f, 1.0f, 1.0f},  // Blue
                {1.0f, 1.0f, 0.3f, 1.0f},  // Yellow
                {1.0f, 0.3f, 1.0f, 1.0f},  // Magenta
                {0.3f, 1.0f, 1.0f, 1.0f},  // Cyan
                {1.0f, 0.6f, 0.2f, 1.0f}   // Orange
            };
            entity.addComponent<limbo::SpriteRendererComponent>(colors[i % 7]);
        }

        // Create a larger stationary entity in the center
        limbo::Entity staticEntity = world.createEntity("CenterSprite");
        auto& staticTransform = staticEntity.addComponent<limbo::TransformComponent>();
        staticTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        staticTransform.scale = glm::vec3(0.3f);
        staticEntity.addComponent<limbo::SpriteRendererComponent>(
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Create physics demo entities if physics mode is enabled
        if (m_physicsEnabled) {
            createPhysicsEntities();
        }

        // Create animated sprite demo
        createAnimatedEntities();

        // Create scripted entities demo
        createScriptedEntities();

        // Create particle emitter entities
        createParticleEntities();

        // Create tilemap demo
        createTilemapEntity();

        // Create UI demo
        createUIDemo();
    }

    void createTilemapEntity() {
        auto& world = getWorld();

        // Create a procedural tileset texture (4x4 grid of different tile types)
        createTilesetTexture();

        // Create the tilemap
        m_tilemap = std::make_shared<limbo::Tilemap>();
        m_tilemap->create(16, 8, 0.15f, 0.15f);  // 16x8 tiles, 0.15 units each
        m_tilemap->setTileset(m_tileset.get());

        // Add layers
        uint32_t groundLayer = m_tilemap->addLayer("ground", 0);
        uint32_t decorLayer = m_tilemap->addLayer("decorations", 1);

        // Fill ground layer with grass (tile 0)
        m_tilemap->fillLayer(groundLayer, 0);

        // Add some dirt patches (tile 1)
        m_tilemap->setTile(groundLayer, 3, 2, 1);
        m_tilemap->setTile(groundLayer, 4, 2, 1);
        m_tilemap->setTile(groundLayer, 5, 2, 1);
        m_tilemap->setTile(groundLayer, 8, 4, 1);
        m_tilemap->setTile(groundLayer, 9, 4, 1);

        // Add stone tiles (tile 2) as a platform
        for (uint32_t x = 6; x < 11; ++x) {
            m_tilemap->setTile(groundLayer, x, 6, 2);
        }

        // Add water tiles (tile 3)
        m_tilemap->setTile(groundLayer, 12, 1, 3);
        m_tilemap->setTile(groundLayer, 13, 1, 3);
        m_tilemap->setTile(groundLayer, 12, 2, 3);
        m_tilemap->setTile(groundLayer, 13, 2, 3);

        // Add some decorations on top layer (tile 4 = flowers)
        m_tilemap->setTile(decorLayer, 2, 3, 4);
        m_tilemap->setTile(decorLayer, 5, 5, 4);
        m_tilemap->setTile(decorLayer, 10, 3, 4);
        m_tilemap->setTile(decorLayer, 14, 5, 4);

        // Create tilemap entity
        limbo::Entity tilemapEntity = world.createEntity("Tilemap");
        auto& transform = tilemapEntity.addComponent<limbo::TransformComponent>();
        transform.position = glm::vec3(-1.2f, -0.6f, -0.1f);  // Behind other sprites

        auto& tilemapComp = tilemapEntity.addComponent<limbo::TilemapComponent>();
        tilemapComp.tilemap = m_tilemap;
        tilemapComp.tileset = m_tileset;

        spdlog::info("Created tilemap: {}x{} tiles, {} layers", m_tilemap->getWidth(),
                     m_tilemap->getHeight(), m_tilemap->getLayerCount());
    }

    void createUIDemo() {
        auto& world = getWorld();

        // Create UI canvas
        m_uiCanvas = std::make_shared<limbo::UICanvas>();

        // Create main panel in top-left corner
        auto panel = std::make_shared<limbo::Panel>();
        panel->setPosition(glm::vec2(10.0f, 10.0f));
        panel->setSize(glm::vec2(220.0f, 150.0f));
        panel->setAnchor(limbo::Anchor::TopLeft);

        // Add label
        auto titleLabel = std::make_shared<limbo::Label>("UI Demo");
        titleLabel->setPosition(glm::vec2(10.0f, 10.0f));
        titleLabel->setSize(glm::vec2(200.0f, 20.0f));
        titleLabel->setTextColor(glm::vec4(1.0f, 1.0f, 0.3f, 1.0f));
        panel->addChild(titleLabel);

        // Add click counter label
        auto clickLabel = std::make_shared<limbo::Label>("Clicks: 0");
        clickLabel->setPosition(glm::vec2(10.0f, 35.0f));
        clickLabel->setSize(glm::vec2(200.0f, 20.0f));
        panel->addChild(clickLabel);

        // Add button
        auto button = std::make_shared<limbo::Button>("Click Me!");
        button->setPosition(glm::vec2(10.0f, 60.0f));
        button->setSize(glm::vec2(100.0f, 30.0f));
        button->setOnClick([this, clickLabel]() {
            m_buttonClickCount++;
            clickLabel->setText("Clicks: " + std::to_string(m_buttonClickCount));
            spdlog::info("Button clicked! Count: {}", m_buttonClickCount);
        });
        panel->addChild(button);

        // Add progress bar
        auto progressBar = std::make_shared<limbo::ProgressBar>();
        progressBar->setPosition(glm::vec2(10.0f, 100.0f));
        progressBar->setSize(glm::vec2(200.0f, 20.0f));
        progressBar->setProgress(0.0f);
        progressBar->setFillColor(glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));
        panel->addChild(progressBar);

        // Store progress bar reference for animation
        m_progressBar = progressBar;

        // Add panel to canvas
        m_uiCanvas->addWidget(panel);

        // Create bottom-right status panel
        auto statusPanel = std::make_shared<limbo::Panel>();
        statusPanel->setPosition(glm::vec2(10.0f, 10.0f));
        statusPanel->setSize(glm::vec2(180.0f, 60.0f));
        statusPanel->setAnchor(limbo::Anchor::BottomRight);

        auto statusLabel = std::make_shared<limbo::Label>("M16: UI System");
        statusLabel->setPosition(glm::vec2(10.0f, 10.0f));
        statusLabel->setSize(glm::vec2(160.0f, 20.0f));
        statusLabel->setTextColor(glm::vec4(0.7f, 0.9f, 1.0f, 1.0f));
        statusPanel->addChild(statusLabel);

        auto fpsLabel = std::make_shared<limbo::Label>("FPS: --");
        fpsLabel->setPosition(glm::vec2(10.0f, 32.0f));
        fpsLabel->setSize(glm::vec2(160.0f, 20.0f));
        statusPanel->addChild(fpsLabel);

        m_fpsLabel = fpsLabel;
        m_uiCanvas->addWidget(statusPanel);

        // Create UI entity
        limbo::Entity uiEntity = world.createEntity("UICanvas");
        auto& uiComp = uiEntity.addComponent<limbo::UICanvasComponent>();
        uiComp.canvas = m_uiCanvas;
        uiComp.screenSpace = true;

        spdlog::info("Created UI demo with panels, buttons, and progress bar");
    }

    void createTilesetTexture() {
        // Create a simple 4x4 procedural tileset
        const uint32_t tileSize = 16;
        const uint32_t cols = 4;
        const uint32_t rows = 4;
        const uint32_t texWidth = tileSize * cols;
        const uint32_t texHeight = tileSize * rows;

        std::vector<uint8_t> pixels(texWidth * texHeight * 4);

        // Helper to set pixel color
        auto setPixel = [&](uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t a = 255) {
            uint32_t idx = (y * texWidth + x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        };

        // Fill each tile with a base color and pattern
        for (uint32_t tileY = 0; tileY < rows; ++tileY) {
            for (uint32_t tileX = 0; tileX < cols; ++tileX) {
                uint32_t tileIdx = tileY * cols + tileX;
                uint32_t baseX = tileX * tileSize;
                uint32_t baseY = tileY * tileSize;

                for (uint32_t y = 0; y < tileSize; ++y) {
                    for (uint32_t x = 0; x < tileSize; ++x) {
                        uint32_t px = baseX + x;
                        uint32_t py = baseY + y;

                        // Different tile types
                        switch (tileIdx) {
                        case 0:  // Grass - green with variation
                        {
                            uint8_t g = static_cast<uint8_t>(100 + (x * y) % 50);
                            setPixel(px, py, 40, g, 30);
                            break;
                        }
                        case 1:  // Dirt - brown
                        {
                            uint8_t r = static_cast<uint8_t>(120 + (x + y) % 30);
                            setPixel(px, py, r, 80, 40);
                            break;
                        }
                        case 2:  // Stone - gray
                        {
                            uint8_t v = static_cast<uint8_t>(100 + (x * 3 + y * 7) % 40);
                            setPixel(px, py, v, v, static_cast<uint8_t>(v + 10));
                            break;
                        }
                        case 3:  // Water - blue
                        {
                            uint8_t b = static_cast<uint8_t>(150 + (x + y * 2) % 50);
                            setPixel(px, py, 30, 80, b);
                            break;
                        }
                        case 4:  // Flowers - green with colored dots
                        {
                            setPixel(px, py, 40, 120, 30);
                            if ((x == 4 || x == 11) && (y == 4 || y == 11)) {
                                setPixel(px, py, 255, 100, 150);  // Pink flower
                            }
                            if ((x == 7) && (y == 7)) {
                                setPixel(px, py, 255, 255, 100);  // Yellow flower
                            }
                            break;
                        }
                        default:  // Empty/transparent
                            setPixel(px, py, 0, 0, 0, 0);
                            break;
                        }
                    }
                }
            }
        }

        // Create tileset texture
        m_tilesetTexture = std::make_unique<limbo::Texture2D>();
        limbo::TextureSpec spec;
        spec.width = texWidth;
        spec.height = texHeight;
        spec.format = limbo::TextureFormat::RGBA8;
        spec.minFilter = limbo::TextureFilter::Nearest;
        spec.magFilter = limbo::TextureFilter::Nearest;
        spec.generateMipmaps = false;
        m_tilesetTexture->create(spec, pixels.data());

        // Create tileset
        m_tileset = std::make_shared<limbo::Tileset>();
        m_tileset->create(m_tilesetTexture.get(), tileSize, tileSize);

        // Set tile properties
        m_tileset->setTileFlags(2, limbo::TileFlags::Solid);  // Stone is solid
        m_tileset->setTileFlags(3, limbo::TileFlags::Water);  // Water

        spdlog::info("Created tileset: {}x{} tiles", cols, rows);
    }

    void createParticleEntities() {
        auto& world = getWorld();

        // Fire/fountain emitter at bottom center
        {
            limbo::Entity emitter = world.createEntity("FireEmitter");
            auto& transform = emitter.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(0.0f, -0.9f, 0.0f);

            auto& particleEmitter = emitter.addComponent<limbo::ParticleEmitterComponent>();
            particleEmitter.props.velocity = glm::vec3(0.0f, 2.5f, 0.0f);
            particleEmitter.props.velocityVariance = glm::vec3(0.8f, 0.5f, 0.0f);
            particleEmitter.props.colorStart = glm::vec4(1.0f, 0.6f, 0.1f, 1.0f);  // Orange
            particleEmitter.props.colorEnd = glm::vec4(1.0f, 0.1f, 0.0f, 0.0f);    // Red, fade out
            particleEmitter.props.sizeStart = 0.08f;
            particleEmitter.props.sizeEnd = 0.02f;
            particleEmitter.props.lifetime = 1.2f;
            particleEmitter.props.lifetimeVariance = 0.3f;
            particleEmitter.props.emissionRate = 30.0f;
        }

        // Sparkle emitter on the left
        {
            limbo::Entity emitter = world.createEntity("SparkleEmitter");
            auto& transform = emitter.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(-1.0f, 0.0f, 0.0f);

            auto& particleEmitter = emitter.addComponent<limbo::ParticleEmitterComponent>();
            particleEmitter.props.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            particleEmitter.props.velocityVariance = glm::vec3(1.5f, 1.5f, 0.0f);
            particleEmitter.props.colorStart = glm::vec4(0.3f, 0.7f, 1.0f, 1.0f);  // Cyan
            particleEmitter.props.colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);  // White, fade out
            particleEmitter.props.sizeStart = 0.05f;
            particleEmitter.props.sizeEnd = 0.0f;
            particleEmitter.props.lifetime = 0.8f;
            particleEmitter.props.lifetimeVariance = 0.2f;
            particleEmitter.props.emissionRate = 20.0f;
            particleEmitter.props.rotationSpeed = 3.0f;
            particleEmitter.props.rotationSpeedVariance = 2.0f;
        }

        // Snow emitter at top
        {
            limbo::Entity emitter = world.createEntity("SnowEmitter");
            auto& transform = emitter.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(1.0f, 0.9f, 0.0f);

            auto& particleEmitter = emitter.addComponent<limbo::ParticleEmitterComponent>();
            particleEmitter.props.positionVariance = glm::vec3(0.5f, 0.0f, 0.0f);
            particleEmitter.props.velocity = glm::vec3(0.0f, -0.5f, 0.0f);
            particleEmitter.props.velocityVariance = glm::vec3(0.3f, 0.2f, 0.0f);
            particleEmitter.props.colorStart = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);  // White
            particleEmitter.props.colorEnd = glm::vec4(0.8f, 0.9f, 1.0f, 0.0f);  // Light blue, fade
            particleEmitter.props.sizeStart = 0.03f;
            particleEmitter.props.sizeEnd = 0.02f;
            particleEmitter.props.sizeVariance = 0.01f;
            particleEmitter.props.lifetime = 2.0f;
            particleEmitter.props.lifetimeVariance = 0.5f;
            particleEmitter.props.emissionRate = 15.0f;
        }

        spdlog::info("Created 3 particle emitter entities");
    }

    void createScriptedEntities() {
        auto& world = getWorld();

        // Find script path
        std::filesystem::path scriptPath = m_assetManager.getAssetRoot() / "scripts" / "player.lua";

        if (!std::filesystem::exists(scriptPath)) {
            spdlog::warn("Script not found: {}", scriptPath.string());
            spdlog::info("Create apps/sandbox/assets/scripts/player.lua to enable scripting demo");
            return;
        }

        // Create a scripted entity (player controlled with IJKL keys)
        limbo::Entity scriptedEntity = world.createEntity("ScriptedPlayer");
        auto& transform = scriptedEntity.addComponent<limbo::TransformComponent>();
        transform.position = glm::vec3(0.0f, -0.3f, 0.0f);
        transform.scale = glm::vec3(0.2f);

        // Bright green to distinguish from other sprites
        scriptedEntity.addComponent<limbo::SpriteRendererComponent>(
            glm::vec4(0.2f, 1.0f, 0.4f, 1.0f));

        // Add script component
        auto& script = scriptedEntity.addComponent<limbo::ScriptComponent>();
        script.scriptPath = scriptPath;

        spdlog::info("Created scripted entity with script: {}", scriptPath.string());
        spdlog::info("  Use I/J/K/L keys to move the green square");
    }

    void createAnimatedEntities() {
        auto& world = getWorld();

        // Create a procedural sprite sheet texture (4x2 grid with color variations)
        createAnimationSpriteSheet();

        // Create an animated entity
        limbo::Entity animEntity = world.createEntity("AnimatedSprite");
        auto& transform = animEntity.addComponent<limbo::TransformComponent>();
        transform.position = glm::vec3(-1.2f, 0.8f, 0.0f);
        transform.scale = glm::vec3(0.25f);

        auto& sprite = animEntity.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f));

        // Setup animator with our sprite sheet
        auto& animator = animEntity.addComponent<limbo::AnimatorComponent>();

        // Create animation clip
        auto clip = std::make_shared<limbo::AnimationClip>("pulse");
        clip->setSpriteSheet(&m_animSpriteSheet);
        clip->addFrameRange(0, 7, 0.1f);  // 8 frames at 0.1s each
        clip->setPlayMode(limbo::AnimationPlayMode::Loop);

        animator.addClip("pulse", clip);
        animator.defaultClip = "pulse";
        animator.playOnStart = true;

        // Create a second animated entity with ping-pong animation
        limbo::Entity animEntity2 = world.createEntity("AnimatedSprite2");
        auto& transform2 = animEntity2.addComponent<limbo::TransformComponent>();
        transform2.position = glm::vec3(1.2f, 0.8f, 0.0f);
        transform2.scale = glm::vec3(0.25f);

        animEntity2.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f));

        auto& animator2 = animEntity2.addComponent<limbo::AnimatorComponent>();

        auto clip2 = std::make_shared<limbo::AnimationClip>("bounce");
        clip2->setSpriteSheet(&m_animSpriteSheet);
        clip2->addFrameRange(0, 7, 0.15f);
        clip2->setPlayMode(limbo::AnimationPlayMode::PingPong);
        clip2->setSpeed(1.5f);

        animator2.addClip("bounce", clip2);
        animator2.defaultClip = "bounce";
        animator2.playOnStart = true;

        spdlog::info("Created {} animated entities", 2);
    }

    void createAnimationSpriteSheet() {
        // Create a simple 4x2 procedural sprite sheet texture
        // Each frame will have a different color intensity
        const uint32_t frameWidth = 32;
        const uint32_t frameHeight = 32;
        const uint32_t cols = 4;
        const uint32_t rows = 2;
        const uint32_t texWidth = frameWidth * cols;
        const uint32_t texHeight = frameHeight * rows;

        std::vector<uint8_t> pixels(texWidth * texHeight * 4);

        for (uint32_t frame = 0; frame < cols * rows; ++frame) {
            uint32_t frameCol = frame % cols;
            uint32_t frameRow = frame / cols;

            // Calculate color based on frame (pulsing effect)
            float t = static_cast<float>(frame) / static_cast<float>(cols * rows - 1);
            float intensity = 0.3f + 0.7f * (0.5f + 0.5f * std::sin(t * 6.28318f));

            uint8_t r = static_cast<uint8_t>(255 * intensity);
            uint8_t g = static_cast<uint8_t>(128 * (1.0f - t) + 255 * t * intensity);
            uint8_t b = static_cast<uint8_t>(255 * (1.0f - intensity * 0.5f));

            for (uint32_t y = 0; y < frameHeight; ++y) {
                for (uint32_t x = 0; x < frameWidth; ++x) {
                    uint32_t px = frameCol * frameWidth + x;
                    uint32_t py = frameRow * frameHeight + y;
                    uint32_t idx = (py * texWidth + px) * 4;

                    // Create a simple circular shape
                    float dx = static_cast<float>(x) - frameWidth * 0.5f;
                    float dy = static_cast<float>(y) - frameHeight * 0.5f;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float radius = frameWidth * 0.4f;

                    if (dist < radius) {
                        pixels[idx + 0] = r;
                        pixels[idx + 1] = g;
                        pixels[idx + 2] = b;
                        pixels[idx + 3] = 255;
                    } else {
                        pixels[idx + 0] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                        pixels[idx + 3] = 0;
                    }
                }
            }
        }

        // Create texture
        m_animTexture = std::make_unique<limbo::Texture2D>();
        limbo::TextureSpec spec;
        spec.width = texWidth;
        spec.height = texHeight;
        spec.format = limbo::TextureFormat::RGBA8;
        spec.minFilter = limbo::TextureFilter::Nearest;
        spec.magFilter = limbo::TextureFilter::Nearest;
        spec.generateMipmaps = false;
        m_animTexture->create(spec, pixels.data());

        // Setup sprite sheet
        m_animSpriteSheet.setTexture(m_animTexture.get());
        m_animSpriteSheet.createFromGrid(frameWidth, frameHeight);

        spdlog::info("Created animation sprite sheet ({}x{}, {} frames)", texWidth, texHeight,
                     m_animSpriteSheet.getFrameCount());
    }

    void createPhysicsEntities() {
        auto& world = getWorld();

        // Create ground (static body)
        {
            limbo::Entity ground = world.createEntity("Ground");
            auto& transform = ground.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(0.0f, -0.8f, 0.0f);
            transform.scale = glm::vec3(3.0f, 0.1f, 1.0f);
            ground.addComponent<limbo::SpriteRendererComponent>(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
            ground.addComponent<limbo::Rigidbody2DComponent>(limbo::BodyType::Static);
            ground.addComponent<limbo::BoxCollider2DComponent>(glm::vec2(0.5f, 0.5f));
        }

        // Create some falling boxes
        for (int i = 0; i < 5; ++i) {
            limbo::Entity box =
                world.createEntity(limbo::String("PhysicsBox_") + std::to_string(i));
            auto& transform = box.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(-0.4f + static_cast<float>(i) * 0.2f,
                                           0.5f + static_cast<float>(i) * 0.3f, 0.0f);
            transform.scale = glm::vec3(0.1f);

            // Different colors for physics objects
            glm::vec4 physicsColors[] = {{0.9f, 0.2f, 0.2f, 1.0f},
                                         {0.2f, 0.9f, 0.2f, 1.0f},
                                         {0.2f, 0.2f, 0.9f, 1.0f},
                                         {0.9f, 0.9f, 0.2f, 1.0f},
                                         {0.9f, 0.2f, 0.9f, 1.0f}};
            box.addComponent<limbo::SpriteRendererComponent>(physicsColors[i]);

            box.addComponent<limbo::Rigidbody2DComponent>(limbo::BodyType::Dynamic);

            auto& collider = box.addComponent<limbo::BoxCollider2DComponent>(glm::vec2(0.5f, 0.5f));
            collider.restitution = 0.3f;
        }

        // Create a falling circle
        {
            limbo::Entity circle = world.createEntity("PhysicsCircle");
            auto& transform = circle.addComponent<limbo::TransformComponent>();
            transform.position = glm::vec3(0.3f, 1.0f, 0.0f);
            transform.scale = glm::vec3(0.15f);
            circle.addComponent<limbo::SpriteRendererComponent>(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

            circle.addComponent<limbo::Rigidbody2DComponent>(limbo::BodyType::Dynamic);

            auto& collider = circle.addComponent<limbo::CircleCollider2DComponent>(0.5f);
            collider.restitution = 0.5f;
        }
    }

    void onUpdate(limbo::f32 deltaTime) override {
        m_deltaTime = deltaTime;

        // Toggle ImGui with F1
        if (limbo::Input::isKeyPressed(limbo::Key::F1)) {
            m_showImGui = !m_showImGui;
            m_imguiLayer.setEnabled(m_showImGui);
        }

        // Toggle demo window with F2
        if (limbo::Input::isKeyPressed(limbo::Key::F2)) {
            m_showDemoWindow = !m_showDemoWindow;
        }

        // Update camera based on input
        updateCamera(deltaTime);

        // Poll for asset hot-reload
        m_assetManager.pollHotReload();

        // Update UI demo elements
        updateUIDemo(deltaTime);

        // Reset stats each frame
        limbo::Renderer2D::resetStats();
    }

    void updateUIDemo(limbo::f32 deltaTime) {
        // Animate progress bar
        if (m_progressBar) {
            m_demoProgress += deltaTime * 0.2f;  // 5 seconds to fill
            if (m_demoProgress > 1.0f) {
                m_demoProgress = 0.0f;
            }
            m_progressBar->setProgress(m_demoProgress);
        }

        // Update FPS label
        if (m_fpsLabel && deltaTime > 0.0f) {
            int fps = static_cast<int>(1.0f / deltaTime);
            m_fpsLabel->setText("FPS: " + std::to_string(fps));
        }
    }

    void updateCamera(limbo::f32 deltaTime) {
        // Camera movement with keyboard
        glm::vec3 camPos = m_camera.getPosition();
        float camSpeed = 2.0f * deltaTime * m_zoom;

        if (limbo::Input::isKeyDown(limbo::Key::W) || limbo::Input::isKeyDown(limbo::Key::Up)) {
            camPos.y += camSpeed;
        }
        if (limbo::Input::isKeyDown(limbo::Key::S) || limbo::Input::isKeyDown(limbo::Key::Down)) {
            camPos.y -= camSpeed;
        }
        if (limbo::Input::isKeyDown(limbo::Key::A) || limbo::Input::isKeyDown(limbo::Key::Left)) {
            camPos.x -= camSpeed;
        }
        if (limbo::Input::isKeyDown(limbo::Key::D) || limbo::Input::isKeyDown(limbo::Key::Right)) {
            camPos.x += camSpeed;
        }

        m_camera.setPosition(camPos);

        // Q/E to rotate
        float camRot = m_camera.getRotation();
        float rotSpeed = 2.0f * deltaTime;

        if (limbo::Input::isKeyDown(limbo::Key::Q)) {
            camRot += rotSpeed;
        }
        if (limbo::Input::isKeyDown(limbo::Key::E)) {
            camRot -= rotSpeed;
        }

        m_camera.setRotation(camRot);

        // Mouse scroll to zoom
        float scroll = limbo::Input::getScrollY();
        if (scroll != 0.0f) {
            m_zoom -= scroll * 0.1f;
            m_zoom = glm::clamp(m_zoom, 0.1f, 10.0f);

            float aspect = static_cast<float>(getWindow().getWidth()) /
                           static_cast<float>(getWindow().getHeight());
            m_camera.setProjection(-aspect * m_zoom, aspect * m_zoom, -m_zoom, m_zoom);
        }

        // Space to reset camera
        if (limbo::Input::isKeyPressed(limbo::Key::Space)) {
            m_camera.setPosition(glm::vec3(0.0f));
            m_camera.setRotation(0.0f);
            m_zoom = 1.0f;

            float aspect = static_cast<float>(getWindow().getWidth()) /
                           static_cast<float>(getWindow().getHeight());
            m_camera.setProjection(-aspect * m_zoom, aspect * m_zoom, -m_zoom, m_zoom);

            spdlog::info("Camera reset");
        }
    }

    void onRender() override {
        // Clear to a nice dark blue color
        m_renderContext->clear(0.1f, 0.1f, 0.2f, 1.0f);

        // Render all sprites using Renderer2D
        renderSprites();

        // Render ImGui
        renderImGui();
    }

    void renderImGui() {
        m_imguiLayer.beginFrame();

        if (m_showImGui) {
            // Stats panel
            limbo::DebugPanels::showStatsPanel(m_deltaTime);

            // Entity inspector
            limbo::DebugPanels::showEntityInspector(getWorld());

            // Asset browser
            limbo::DebugPanels::showAssetBrowser(m_assetManager);

            // Scene panel
            showScenePanel();

            // Audio panel
            showAudioPanel();

            // Demo window (toggle with F2)
            if (m_showDemoWindow) {
                limbo::DebugPanels::showDemoWindow();
            }
        }

        m_imguiLayer.endFrame();
    }

    void showScenePanel() {
        ImGui::Begin("Scene");

        ImGui::Text("Scene Management");
        ImGui::Separator();

        // Save scene
        static char saveFilename[256] = "scene.json";
        ImGui::InputText("Filename", saveFilename, sizeof(saveFilename));

        if (ImGui::Button("Save Scene")) {
            std::filesystem::path savePath =
                m_assetManager.getAssetRoot() / "scenes" / saveFilename;

            // Create scenes directory if needed
            std::filesystem::create_directories(savePath.parent_path());

            limbo::SceneSerializer serializer(getWorld());
            if (serializer.saveToFile(savePath)) {
                m_statusMessage = "Scene saved: " + savePath.string();
                m_statusIsError = false;
            } else {
                m_statusMessage = "Failed to save: " + serializer.getError();
                m_statusIsError = true;
            }
            m_statusTimer = 3.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Scene")) {
            std::filesystem::path loadPath =
                m_assetManager.getAssetRoot() / "scenes" / saveFilename;

            limbo::SceneSerializer serializer(getWorld());
            if (serializer.loadFromFile(loadPath)) {
                m_statusMessage = "Scene loaded: " + loadPath.string();
                m_statusIsError = false;
            } else {
                m_statusMessage = "Failed to load: " + serializer.getError();
                m_statusIsError = true;
            }
            m_statusTimer = 3.0f;
        }

        ImGui::Separator();

        if (ImGui::Button("Reset to Default")) {
            getWorld().clear();
            createEntities();
            m_statusMessage = "Scene reset to default";
            m_statusIsError = false;
            m_statusTimer = 3.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear All")) {
            getWorld().clear();
            m_statusMessage = "Scene cleared";
            m_statusIsError = false;
            m_statusTimer = 3.0f;
        }

        // Status message
        if (m_statusTimer > 0.0f) {
            m_statusTimer -= m_deltaTime;
            ImGui::Separator();
            if (m_statusIsError) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_statusMessage.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", m_statusMessage.c_str());
            }
        }

        ImGui::End();
    }

    void showAudioPanel() {
        ImGui::Begin("Audio");

        ImGui::Text("Audio Engine");
        ImGui::Separator();

        // Master volume
        static float masterVolume = 1.0f;
        if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
            m_audioEngine.setMasterVolume(masterVolume);
        }

        ImGui::Text("Sample Rate: %u Hz", m_audioEngine.getSampleRate());
        ImGui::Text("Channels: %u", m_audioEngine.getChannels());

        ImGui::Separator();
        ImGui::Text("Test Tone");

        // Test tone controls
        static float frequency = 440.0f;
        static float duration = 0.5f;
        static float volume = 0.3f;

        ImGui::SliderFloat("Frequency", &frequency, 100.0f, 2000.0f, "%.0f Hz");
        ImGui::SliderFloat("Duration", &duration, 0.1f, 2.0f, "%.1f s");
        ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);

        if (ImGui::Button("Generate & Play Tone")) {
            m_testToneClip.generateTestTone(frequency, duration, volume);
            m_testToneSource.setClip(&m_testToneClip);
            m_audioEngine.registerSource(&m_testToneSource);
            m_testToneSource.play();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop")) {
            m_testToneSource.stop();
        }

        // Show playback state
        const char* stateStr = "Stopped";
        if (m_testToneSource.getState() == limbo::AudioState::Playing) {
            stateStr = "Playing";
        } else if (m_testToneSource.getState() == limbo::AudioState::Paused) {
            stateStr = "Paused";
        }
        ImGui::Text("State: %s", stateStr);

        if (m_testToneClip.isLoaded()) {
            ImGui::Text("Clip Duration: %.2f s", m_testToneClip.getDuration());
        }

        ImGui::End();
    }

    void renderSprites() {
        auto& world = getWorld();

        // Begin batch rendering
        limbo::Renderer2D::beginScene(m_camera);

        // Render all entities with Transform and SpriteRenderer
        auto view = world.view<limbo::TransformComponent, limbo::SpriteRendererComponent>();
        for (auto entity : view) {
            const auto& transform = view.get<limbo::TransformComponent>(entity);
            const auto& sprite = view.get<limbo::SpriteRendererComponent>(entity);

            // Check if entity has animator (uses sprite sheet with custom UVs)
            if (world.hasComponent<limbo::AnimatorComponent>(entity)) {
                const auto& animator = world.getComponent<limbo::AnimatorComponent>(entity);
                auto* clip = animator.currentState.getClip();
                if (clip && clip->getSpriteSheet() && clip->getSpriteSheet()->getTexture()) {
                    limbo::Renderer2D::drawQuad(transform.getMatrix(),
                                                *clip->getSpriteSheet()->getTexture(), sprite.uvMin,
                                                sprite.uvMax, sprite.color);
                    continue;
                }
            }

            // Regular sprite rendering
            if (m_checkerboardTexture && m_checkerboardTexture->isLoaded()) {
                // Use texture with tint color
                limbo::Renderer2D::drawQuad(transform.getMatrix(),
                                            *m_checkerboardTexture->getTexture(), 1.0f,
                                            sprite.color);
            } else {
                limbo::Renderer2D::drawQuad(transform.getMatrix(), sprite.color);
            }
        }

        // Render tilemaps
        if (m_tilemapSystem) {
            m_tilemapSystem->render(world);
        }

        // Render particles (they're added to the same batch)
        if (m_particleSystem) {
            m_particleSystem->render();
        }

        // End batch and flush
        limbo::Renderer2D::endScene();

        // Render UI (separate pass with screen-space camera)
        if (m_uiSystem) {
            m_uiSystem->render(getWorld());
        }
    }

    void onShutdown() override {
        m_scriptEngine.shutdown();
        m_audioEngine.shutdown();
        m_physics.shutdown();
        m_imguiLayer.shutdown();

        limbo::Renderer2D::shutdown();

        if (m_renderContext) {
            m_renderContext->shutdown();
            m_renderContext.reset();
        }
        spdlog::info("Sandbox shutdown");
    }

private:
    limbo::Unique<limbo::RenderContext> m_renderContext;
    limbo::AssetManager m_assetManager;
    limbo::ImGuiLayer m_imguiLayer;

    // Loaded assets
    limbo::Shared<limbo::TextureAsset> m_checkerboardTexture;

    // Camera
    limbo::OrthographicCamera m_camera;
    limbo::f32 m_zoom = 1.0f;

    // ImGui state
    limbo::f32 m_deltaTime = 0.0f;
    bool m_showImGui = true;
    bool m_showDemoWindow = false;

    // Scene panel state
    limbo::String m_statusMessage;
    limbo::f32 m_statusTimer = 0.0f;
    bool m_statusIsError = false;

    // Physics
    limbo::Physics2D m_physics;
    bool m_physicsEnabled = true;

    // Audio
    limbo::AudioEngine m_audioEngine;
    limbo::AudioClip m_testToneClip;
    limbo::AudioSource m_testToneSource;

    // Animation
    limbo::Unique<limbo::Texture2D> m_animTexture;
    limbo::SpriteSheet m_animSpriteSheet;

    // Scripting
    limbo::ScriptEngine m_scriptEngine;

    // Particles
    limbo::ParticleRenderSystem* m_particleSystem = nullptr;

    // Tilemap
    limbo::TilemapRenderSystem* m_tilemapSystem = nullptr;
    limbo::Unique<limbo::Texture2D> m_tilesetTexture;
    limbo::Shared<limbo::Tileset> m_tileset;
    limbo::Shared<limbo::Tilemap> m_tilemap;

    // UI
    limbo::UISystem* m_uiSystem = nullptr;
    limbo::Shared<limbo::UICanvas> m_uiCanvas;
    std::shared_ptr<limbo::ProgressBar> m_progressBar;
    std::shared_ptr<limbo::Label> m_fpsLabel;
    float m_demoProgress = 0.0f;
    int m_buttonClickCount = 0;
};

int main() {
    // Initialize debug/logging
    limbo::debug::init();

    SandboxApp app;

    limbo::ApplicationConfig config;
    config.appName = "Limbo Sandbox";
    config.window.title = "Limbo Engine - 2D Renderer Demo";
    config.window.width = 1280;
    config.window.height = 720;

    auto result = app.init(config);
    if (!result) {
        spdlog::critical("Failed to initialize application: {}", result.error());
        limbo::debug::shutdown();
        return 1;
    }

    app.run();
    app.shutdown();

    limbo::debug::shutdown();
    return 0;
}
