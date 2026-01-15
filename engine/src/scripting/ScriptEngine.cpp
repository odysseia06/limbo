#include "limbo/scripting/ScriptEngine.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/DeferredDestruction.hpp"
#include "limbo/platform/Input.hpp"
#include "limbo/physics/2d/Physics2D.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/debug/Log.hpp"

#include <glm/glm.hpp>

namespace limbo {

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::init() {
    if (m_initialized) {
        return true;
    }

    // Open safe Lua libraries only
    // NOTE: os and io libraries are intentionally excluded for security
    // They allow filesystem access and command execution which is dangerous
    // for untrusted scripts. Use setSandboxed(false) to enable them if needed.
    m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math,
                         sol::lib::table);

    // Register engine bindings
    registerBindings();

    m_initialized = true;
    LIMBO_LOG_SCRIPT_INFO("ScriptEngine initialized");
    return true;
}

void ScriptEngine::shutdown() {
    if (!m_initialized) {
        return;
    }

    m_boundWorld = nullptr;
    m_boundPhysics = nullptr;
    m_entityTypesRegistered = false;
    m_physicsTypesRegistered = false;
    m_sandboxed = true;
    m_initialized = false;
    LIMBO_LOG_SCRIPT_INFO("ScriptEngine shutdown");
}

void ScriptEngine::setSandboxed(bool sandboxed) {
    if (!m_initialized) {
        LIMBO_LOG_SCRIPT_WARN("ScriptEngine::setSandboxed called before init()");
        return;
    }

    if (sandboxed == m_sandboxed) {
        return;  // No change
    }

    if (!sandboxed) {
        // Enable os and io libraries (dangerous for untrusted scripts!)
        LIMBO_LOG_SCRIPT_WARN(
            "ScriptEngine: Sandbox disabled - os/io libraries enabled. "
            "Only use with trusted scripts!");
        m_lua.open_libraries(sol::lib::os, sol::lib::io);
    } else {
        // Remove os and io libraries by setting them to nil
        m_lua["os"] = sol::nil;
        m_lua["io"] = sol::nil;
        LIMBO_LOG_SCRIPT_INFO("ScriptEngine: Sandbox enabled - os/io libraries disabled");
    }

    m_sandboxed = sandboxed;
}

bool ScriptEngine::loadScript(const std::filesystem::path& path) {
    if (!m_initialized) {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    try {
        auto result = m_lua.safe_script_file(path.string());
        if (!result.valid()) {
            sol::error const err = result;
            m_lastError = err.what();
            LIMBO_LOG_SCRIPT_ERROR("Lua script error in {}: {}", path.string(), m_lastError);
            return false;
        }
        LIMBO_LOG_SCRIPT_DEBUG("Loaded script: {}", path.string());
        return true;
    } catch (const sol::error& e) {
        m_lastError = e.what();
        LIMBO_LOG_SCRIPT_ERROR("Lua exception in {}: {}", path.string(), m_lastError);
        return false;
    }
}

bool ScriptEngine::executeString(const String& code) {
    if (!m_initialized) {
        m_lastError = "ScriptEngine not initialized";
        return false;
    }

    try {
        auto result = m_lua.safe_script(code);
        if (!result.valid()) {
            sol::error const err = result;
            m_lastError = err.what();
            LIMBO_LOG_SCRIPT_ERROR("Lua error: {}", m_lastError);
            return false;
        }
        return true;
    } catch (const sol::error& e) {
        m_lastError = e.what();
        LIMBO_LOG_SCRIPT_ERROR("Lua exception: {}", m_lastError);
        return false;
    }
}

bool ScriptEngine::hasFunction(const String& name) const {
    sol::object const obj = m_lua[name];
    return obj.is<sol::function>();
}

void ScriptEngine::registerBindings() {
    bindMathTypes();
    bindInputTypes();
    bindUtilityFunctions();
}

void ScriptEngine::bindWorld(World* world) {
    m_boundWorld = world;
    // Only register usertypes once - re-registration corrupts sol2 usertypes
    if (!m_entityTypesRegistered) {
        bindEntityTypes();
        bindComponentTypes();
        m_entityTypesRegistered = true;
    }
}

void ScriptEngine::bindMathTypes() {
    // Vec2
    m_lua.new_usertype<glm::vec2>(
        "Vec2", sol::call_constructor,
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(), "x",
        &glm::vec2::x, "y", &glm::vec2::y, sol::meta_function::addition,
        [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        sol::meta_function::subtraction,
        [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        sol::meta_function::multiplication,
        sol::overload([](const glm::vec2& a, float b) { return a * b; },
                      [](float a, const glm::vec2& b) { return a * b; }),
        "length", [](const glm::vec2& v) { return glm::length(v); }, "normalize",
        [](const glm::vec2& v) { return glm::normalize(v); }, "dot",
        [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); });

    // Vec3
    m_lua.new_usertype<glm::vec3>(
        "Vec3", sol::call_constructor,
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(), "x",
        &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z, sol::meta_function::addition,
        [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction,
        [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication,
        sol::overload([](const glm::vec3& a, float b) { return a * b; },
                      [](float a, const glm::vec3& b) { return a * b; }),
        "length", [](const glm::vec3& v) { return glm::length(v); }, "normalize",
        [](const glm::vec3& v) { return glm::normalize(v); }, "dot",
        [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); }, "cross",
        [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); });

    // Vec4 / Color
    m_lua.new_usertype<glm::vec4>(
        "Vec4", sol::call_constructor,
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x, "y", &glm::vec4::y, "z", &glm::vec4::z, "w", &glm::vec4::w, "r",
        &glm::vec4::r, "g", &glm::vec4::g, "b", &glm::vec4::b, "a", &glm::vec4::a);

    // Color alias
    m_lua["Color"] = m_lua["Vec4"];
}

// Helper to convert string key names to Key enum
static Key stringToKey(const String& keyName) {
    // Letters
    if (keyName == "A" || keyName == "a")
        return Key::A;
    if (keyName == "B" || keyName == "b")
        return Key::B;
    if (keyName == "C" || keyName == "c")
        return Key::C;
    if (keyName == "D" || keyName == "d")
        return Key::D;
    if (keyName == "E" || keyName == "e")
        return Key::E;
    if (keyName == "F" || keyName == "f")
        return Key::F;
    if (keyName == "G" || keyName == "g")
        return Key::G;
    if (keyName == "H" || keyName == "h")
        return Key::H;
    if (keyName == "I" || keyName == "i")
        return Key::I;
    if (keyName == "J" || keyName == "j")
        return Key::J;
    if (keyName == "K" || keyName == "k")
        return Key::K;
    if (keyName == "L" || keyName == "l")
        return Key::L;
    if (keyName == "M" || keyName == "m")
        return Key::M;
    if (keyName == "N" || keyName == "n")
        return Key::N;
    if (keyName == "O" || keyName == "o")
        return Key::O;
    if (keyName == "P" || keyName == "p")
        return Key::P;
    if (keyName == "Q" || keyName == "q")
        return Key::Q;
    if (keyName == "R" || keyName == "r")
        return Key::R;
    if (keyName == "S" || keyName == "s")
        return Key::S;
    if (keyName == "T" || keyName == "t")
        return Key::T;
    if (keyName == "U" || keyName == "u")
        return Key::U;
    if (keyName == "V" || keyName == "v")
        return Key::V;
    if (keyName == "W" || keyName == "w")
        return Key::W;
    if (keyName == "X" || keyName == "x")
        return Key::X;
    if (keyName == "Y" || keyName == "y")
        return Key::Y;
    if (keyName == "Z" || keyName == "z")
        return Key::Z;

    // Special keys
    if (keyName == "Space" || keyName == "space")
        return Key::Space;
    if (keyName == "Enter" || keyName == "enter" || keyName == "Return")
        return Key::Enter;
    if (keyName == "Escape" || keyName == "escape" || keyName == "Esc")
        return Key::Escape;
    if (keyName == "Tab" || keyName == "tab")
        return Key::Tab;
    if (keyName == "Backspace" || keyName == "backspace")
        return Key::Backspace;

    // Arrow keys
    if (keyName == "Up" || keyName == "up")
        return Key::Up;
    if (keyName == "Down" || keyName == "down")
        return Key::Down;
    if (keyName == "Left" || keyName == "left")
        return Key::Left;
    if (keyName == "Right" || keyName == "right")
        return Key::Right;

    // Modifiers
    if (keyName == "Shift" || keyName == "shift" || keyName == "LeftShift")
        return Key::LeftShift;
    if (keyName == "RightShift")
        return Key::RightShift;
    if (keyName == "Control" || keyName == "control" || keyName == "Ctrl" ||
        keyName == "LeftControl")
        return Key::LeftControl;
    if (keyName == "RightControl")
        return Key::RightControl;
    if (keyName == "Alt" || keyName == "alt" || keyName == "LeftAlt")
        return Key::LeftAlt;
    if (keyName == "RightAlt")
        return Key::RightAlt;

    // Numbers
    if (keyName == "0")
        return Key::Num0;
    if (keyName == "1")
        return Key::Num1;
    if (keyName == "2")
        return Key::Num2;
    if (keyName == "3")
        return Key::Num3;
    if (keyName == "4")
        return Key::Num4;
    if (keyName == "5")
        return Key::Num5;
    if (keyName == "6")
        return Key::Num6;
    if (keyName == "7")
        return Key::Num7;
    if (keyName == "8")
        return Key::Num8;
    if (keyName == "9")
        return Key::Num9;

    // Default - unknown key
    return Key::Unknown;
}

void ScriptEngine::bindInputTypes() {
    // Input table
    auto input = m_lua.create_named_table("Input");

    // Key checks - support both int and string
    input["isKeyDown"] =
        sol::overload([](int key) { return Input::isKeyDown(static_cast<Key>(key)); },
                      [](const String& keyName) { return Input::isKeyDown(stringToKey(keyName)); });
    input["isKeyPressed"] = sol::overload(
        [](int key) { return Input::isKeyPressed(static_cast<Key>(key)); },
        [](const String& keyName) { return Input::isKeyPressed(stringToKey(keyName)); });
    input["isKeyReleased"] = sol::overload(
        [](int key) { return Input::isKeyReleased(static_cast<Key>(key)); },
        [](const String& keyName) { return Input::isKeyReleased(stringToKey(keyName)); });

    // Mouse checks
    input["isMouseButtonDown"] = [](int button) {
        return Input::isMouseButtonDown(static_cast<MouseButton>(button));
    };
    input["isMouseButtonPressed"] = [](int button) {
        return Input::isMouseButtonPressed(static_cast<MouseButton>(button));
    };
    input["getMousePosition"] = []() { return glm::vec2(Input::getMouseX(), Input::getMouseY()); };
    input["getMouseX"] = []() { return Input::getMouseX(); };
    input["getMouseY"] = []() { return Input::getMouseY(); };

    // Key constants
    auto keys = m_lua.create_named_table("Key");
    keys["W"] = static_cast<int>(Key::W);
    keys["A"] = static_cast<int>(Key::A);
    keys["S"] = static_cast<int>(Key::S);
    keys["D"] = static_cast<int>(Key::D);
    keys["Space"] = static_cast<int>(Key::Space);
    keys["Escape"] = static_cast<int>(Key::Escape);
    keys["Up"] = static_cast<int>(Key::Up);
    keys["Down"] = static_cast<int>(Key::Down);
    keys["Left"] = static_cast<int>(Key::Left);
    keys["Right"] = static_cast<int>(Key::Right);
    keys["Enter"] = static_cast<int>(Key::Enter);
    keys["Tab"] = static_cast<int>(Key::Tab);
    keys["Shift"] = static_cast<int>(Key::LeftShift);
    keys["Control"] = static_cast<int>(Key::LeftControl);
    keys["I"] = static_cast<int>(Key::I);
    keys["J"] = static_cast<int>(Key::J);
    keys["K"] = static_cast<int>(Key::K);
    keys["L"] = static_cast<int>(Key::L);

    // Mouse button constants
    auto mouse = m_lua.create_named_table("Mouse");
    mouse["Left"] = static_cast<int>(MouseButton::Left);
    mouse["Right"] = static_cast<int>(MouseButton::Right);
    mouse["Middle"] = static_cast<int>(MouseButton::Middle);
}

static uint32_t entityGetId(Entity& e) {
    return static_cast<uint32_t>(e.id());
}

static String entityGetName(Entity& e) {
    if (e.hasComponent<NameComponent>()) {
        return e.getComponent<NameComponent>().name;
    }
    return "";
}

// Static helper functions for physics access (moved here so RigidbodyHandle can be defined before
// bindEntityTypes)
static glm::vec2 entityGetVelocity(Entity& e) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            b2Vec2 const vel = rb.runtimeBody->GetLinearVelocity();
            return {vel.x, vel.y};
        }
    }
    return {0.0f, 0.0f};
}

static void entitySetVelocity(Entity& e, const glm::vec2& vel) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            rb.runtimeBody->SetLinearVelocity(b2Vec2(vel.x, vel.y));
        }
    }
}

static float entityGetAngularVelocity(Entity& e) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            return rb.runtimeBody->GetAngularVelocity();
        }
    }
    return 0.0f;
}

static void entitySetAngularVelocity(Entity& e, float angVel) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            rb.runtimeBody->SetAngularVelocity(angVel);
        }
    }
}

static void entityApplyForce(Entity& e, const glm::vec2& force) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            rb.runtimeBody->ApplyForceToCenter(b2Vec2(force.x, force.y), true);
        }
    }
}

static void entityApplyImpulse(Entity& e, const glm::vec2& impulse) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            rb.runtimeBody->ApplyLinearImpulseToCenter(b2Vec2(impulse.x, impulse.y), true);
        }
    }
}

static void entityApplyTorque(Entity& e, float torque) {
    if (e.hasComponent<Rigidbody2DComponent>()) {
        auto& rb = e.getComponent<Rigidbody2DComponent>();
        if (rb.runtimeBody) {
            rb.runtimeBody->ApplyTorque(torque, true);
        }
    }
}

// Rigidbody wrapper for more intuitive API (self:getRigidbody():setVelocity(x, y))
// This is a lightweight wrapper that stores a reference to the entity
// Defined before bindEntityTypes so it can be used there
struct RigidbodyHandle {
    mutable Entity entity;

    explicit RigidbodyHandle(Entity e) : entity(e) {}

    glm::vec2 getVelocity() const { return entityGetVelocity(entity); }

    void setVelocity(float x, float y) { entitySetVelocity(entity, glm::vec2(x, y)); }

    float getVelocityX() const { return getVelocity().x; }

    float getVelocityY() const { return getVelocity().y; }

    void setVelocityX(float x) {
        glm::vec2 vel = getVelocity();
        vel.x = x;
        entitySetVelocity(entity, vel);
    }

    void setVelocityY(float y) {
        glm::vec2 vel = getVelocity();
        vel.y = y;
        entitySetVelocity(entity, vel);
    }

    float getAngularVelocity() const { return entityGetAngularVelocity(entity); }

    void setAngularVelocity(float angVel) { entitySetAngularVelocity(entity, angVel); }

    void applyForce(float x, float y) { entityApplyForce(entity, glm::vec2(x, y)); }

    void applyImpulse(float x, float y) { entityApplyImpulse(entity, glm::vec2(x, y)); }

    void applyTorque(float torque) { entityApplyTorque(entity, torque); }
};

// Static helper functions for component access (forward declarations for bindEntityTypes)
static glm::vec3 entityGetPosition(Entity& e) {
    if (e.hasComponent<TransformComponent>()) {
        return e.getComponent<TransformComponent>().position;
    }
    return glm::vec3(0.0f);
}

static void entitySetPosition(Entity& e, const glm::vec3& pos) {
    if (e.hasComponent<TransformComponent>()) {
        e.getComponent<TransformComponent>().position = pos;
    }
}

static glm::vec3 entityGetRotation(Entity& e) {
    if (e.hasComponent<TransformComponent>()) {
        return e.getComponent<TransformComponent>().rotation;
    }
    return glm::vec3(0.0f);
}

static void entitySetRotation(Entity& e, const glm::vec3& rot) {
    if (e.hasComponent<TransformComponent>()) {
        e.getComponent<TransformComponent>().rotation = rot;
    }
}

static glm::vec3 entityGetScale(Entity& e) {
    if (e.hasComponent<TransformComponent>()) {
        return e.getComponent<TransformComponent>().scale;
    }
    return glm::vec3(1.0f);
}

static void entitySetScale(Entity& e, const glm::vec3& scale) {
    if (e.hasComponent<TransformComponent>()) {
        e.getComponent<TransformComponent>().scale = scale;
    }
}

static glm::vec4 entityGetColor(Entity& e) {
    if (e.hasComponent<SpriteRendererComponent>()) {
        return e.getComponent<SpriteRendererComponent>().color;
    }
    return glm::vec4(1.0f);
}

static void entitySetColor(Entity& e, const glm::vec4& color) {
    if (e.hasComponent<SpriteRendererComponent>()) {
        e.getComponent<SpriteRendererComponent>().color = color;
    }
}

static void entityAddTransform(Entity& e) {
    if (!e.hasComponent<TransformComponent>()) {
        e.addComponent<TransformComponent>();
    }
}

static void entityAddSprite(Entity& e, const glm::vec4& color) {
    if (!e.hasComponent<SpriteRendererComponent>()) {
        e.addComponent<SpriteRendererComponent>(color);
    }
}

void ScriptEngine::bindEntityTypes() {
    if (!m_boundWorld) {
        return;
    }

    // Register RigidbodyHandle usertype first (used by Entity:getRigidbody())
    m_lua.new_usertype<RigidbodyHandle>(
        "RigidbodyHandle", sol::no_constructor, "getVelocity",
        [](RigidbodyHandle& h) { return h.getVelocity(); }, "setVelocity",
        [](RigidbodyHandle& h, float x, float y) { h.setVelocity(x, y); }, "getVelocityX",
        [](RigidbodyHandle& h) { return h.getVelocityX(); }, "getVelocityY",
        [](RigidbodyHandle& h) { return h.getVelocityY(); }, "setVelocityX",
        [](RigidbodyHandle& h, float x) { h.setVelocityX(x); }, "setVelocityY",
        [](RigidbodyHandle& h, float y) { h.setVelocityY(y); }, "getAngularVelocity",
        [](RigidbodyHandle& h) { return h.getAngularVelocity(); }, "setAngularVelocity",
        [](RigidbodyHandle& h, float v) { h.setAngularVelocity(v); }, "applyForce",
        [](RigidbodyHandle& h, float x, float y) { h.applyForce(x, y); }, "applyImpulse",
        [](RigidbodyHandle& h, float x, float y) { h.applyImpulse(x, y); }, "applyTorque",
        [](RigidbodyHandle& h, float t) { h.applyTorque(t); });

    // Entity handle for Lua
    // Use DeferredDestruction for destroy() to handle physics callback safety
    m_lua.new_usertype<Entity>(
        "Entity", sol::no_constructor,
        // Core methods
        "isValid", &Entity::isValid, "getId", &entityGetId, "getName", &entityGetName, "destroy",
        [](Entity& e) {
            if (e.isValid()) {
                DeferredDestruction::destroy(*e.world(), e.id());
            }
        },
        // Position property (read/write)
        "position",
        sol::property(
            [](Entity& e) -> glm::vec3 {
                if (e.hasComponent<TransformComponent>()) {
                    return e.getComponent<TransformComponent>().position;
                }
                return glm::vec3(0.0f);
            },
            [](Entity& e, const glm::vec3& pos) {
                if (e.hasComponent<TransformComponent>()) {
                    e.getComponent<TransformComponent>().position = pos;
                }
            }),
        // Rotation property (read/write)
        "rotation",
        sol::property(
            [](Entity& e) -> glm::vec3 {
                if (e.hasComponent<TransformComponent>()) {
                    return e.getComponent<TransformComponent>().rotation;
                }
                return glm::vec3(0.0f);
            },
            [](Entity& e, const glm::vec3& rot) {
                if (e.hasComponent<TransformComponent>()) {
                    e.getComponent<TransformComponent>().rotation = rot;
                }
            }),
        // Scale property (read/write)
        "scale",
        sol::property(
            [](Entity& e) -> glm::vec3 {
                if (e.hasComponent<TransformComponent>()) {
                    return e.getComponent<TransformComponent>().scale;
                }
                return glm::vec3(1.0f);
            },
            [](Entity& e, const glm::vec3& scale) {
                if (e.hasComponent<TransformComponent>()) {
                    e.getComponent<TransformComponent>().scale = scale;
                }
            }),
        // Transform methods
        "getPosition", &entityGetPosition, "setPosition", &entitySetPosition, "getRotation",
        &entityGetRotation, "setRotation", &entitySetRotation, "getScale", &entityGetScale,
        "setScale", &entitySetScale,
        // Sprite methods
        "getColor", &entityGetColor, "setColor", &entitySetColor, "addTransform",
        &entityAddTransform, "addSprite", &entityAddSprite,
        // Physics methods (getRigidbody returns a handle for more methods)
        "getRigidbody",
        [this](Entity& e) -> sol::object {
            if (e.isValid() && e.hasComponent<Rigidbody2DComponent>()) {
                return sol::make_object(m_lua, RigidbodyHandle(e));
            }
            return sol::nil;
        });

    // Capture world pointer for closures
    World* world = m_boundWorld;
    sol::state& lua = m_lua;

    // World access
    m_lua["World"] = m_lua.create_table_with(
        "createEntity",
        [world](const String& name) -> Entity {
            if (world) {
                return world->createEntity(name);
            }
            return Entity();
        },
        "getEntityByName",
        [world, &lua](const String& name) -> sol::object {
            if (!world) {
                return sol::nil;
            }
            // Search for entity by name
            Entity found;
            world->each<NameComponent>([&](World::EntityId id, NameComponent& nc) {
                if (nc.name == name && !found.isValid()) {
                    found = Entity(id, world);
                }
            });
            if (found.isValid()) {
                return sol::make_object(lua, found);
            }
            return sol::nil;
        },
        "entityCount", [world]() -> size_t { return world ? world->entityCount() : 0; });
}

void ScriptEngine::bindComponentTypes() {
    // All component methods are now added directly in bindEntityTypes
    // This function is kept for API compatibility but does nothing
}

void ScriptEngine::bindUtilityFunctions() {
    // Logging
    m_lua["print"] = [](const String& msg) { LIMBO_LOG_SCRIPT_INFO("[Lua] {}", msg); };
    m_lua["log"] = m_lua.create_table_with(
        "info", [](const String& msg) { LIMBO_LOG_SCRIPT_INFO("[Lua] {}", msg); }, "warn",
        [](const String& msg) { LIMBO_LOG_SCRIPT_WARN("[Lua] {}", msg); }, "error",
        [](const String& msg) { LIMBO_LOG_SCRIPT_ERROR("[Lua] {}", msg); }, "debug",
        [](const String& msg) { LIMBO_LOG_SCRIPT_DEBUG("[Lua] {}", msg); });

    // Time (will be set each frame by ScriptSystem)
    m_lua["Time"] = m_lua.create_table_with("deltaTime", 0.0f, "totalTime", 0.0f);

    // Math utilities
    m_lua["math"]["clamp"] = [](float value, float min, float max) {
        return glm::clamp(value, min, max);
    };
    m_lua["math"]["lerp"] = [](float a, float b, float t) { return glm::mix(a, b, t); };
}

void ScriptEngine::bindPhysics(Physics2D* physics) {
    m_boundPhysics = physics;
    // Only register physics types once - re-registration corrupts sol2 usertypes
    if (!m_physicsTypesRegistered) {
        bindPhysicsTypes();
        m_physicsTypesRegistered = true;
    }
}

void ScriptEngine::bindPhysicsTypes() {
    if (!m_boundPhysics || !m_boundWorld) {
        return;
    }

    Physics2D* physics = m_boundPhysics;
    World* world = m_boundWorld;
    sol::state& lua = m_lua;

    // RaycastHit2D result type
    m_lua.new_usertype<RaycastHit2D>(
        "RaycastHit2D", sol::no_constructor, "hit", &RaycastHit2D::hit, "point",
        &RaycastHit2D::point, "normal", &RaycastHit2D::normal, "distance", &RaycastHit2D::distance,
        "getEntity", [world, &lua](const RaycastHit2D& hit) -> sol::object {
            if (!hit.hit || !hit.body) {
                return sol::nil;
            }
            auto entityId = static_cast<World::EntityId>(hit.body->GetUserData().pointer);
            if (world->isValid(entityId)) {
                return sol::make_object(lua, Entity(entityId, world));
            }
            return sol::nil;
        });

    // Physics table with query functions
    m_lua["Physics"] = m_lua.create_table_with(
        "raycast",
        [physics, world, &lua](const glm::vec2& origin, const glm::vec2& direction,
                               float maxDistance,
                               sol::optional<bool> includeTriggers) -> sol::object {
            if (!physics->isInitialized()) {
                return sol::nil;
            }
            bool const triggers = includeTriggers.value_or(false);
            RaycastHit2D hit = physics->raycast(origin, direction, maxDistance, triggers);
            if (hit.hit) {
                return sol::make_object(lua, hit);
            }
            return sol::nil;
        },
        "raycastAll",
        [physics, &lua](const glm::vec2& origin, const glm::vec2& direction, float maxDistance,
                        sol::optional<bool> includeTriggers) -> sol::table {
            sol::table results = lua.create_table();
            if (!physics->isInitialized()) {
                return results;
            }
            bool const triggers = includeTriggers.value_or(false);
            auto hits = physics->raycastAll(origin, direction, maxDistance, triggers);
            for (size_t i = 0; i < hits.size(); ++i) {
                results[i + 1] = hits[i];  // Lua arrays are 1-indexed
            }
            return results;
        },
        "overlapCircle",
        [physics, world, &lua](const glm::vec2& center, float radius,
                               sol::optional<bool> includeTriggers) -> sol::table {
            sol::table results = lua.create_table();
            if (!physics->isInitialized()) {
                return results;
            }
            bool const triggers = includeTriggers.value_or(false);
            auto bodies = physics->overlapCircle(center, radius, triggers);
            size_t index = 1;
            for (b2Body* body : bodies) {
                auto entityId = static_cast<World::EntityId>(body->GetUserData().pointer);
                if (world->isValid(entityId)) {
                    results[index++] = Entity(entityId, world);
                }
            }
            return results;
        },
        "overlapBox",
        [physics, world, &lua](const glm::vec2& center, const glm::vec2& halfExtents,
                               sol::optional<bool> includeTriggers) -> sol::table {
            sol::table results = lua.create_table();
            if (!physics->isInitialized()) {
                return results;
            }
            bool const triggers = includeTriggers.value_or(false);
            auto bodies = physics->overlapBox(center, halfExtents, triggers);
            size_t index = 1;
            for (b2Body* body : bodies) {
                auto entityId = static_cast<World::EntityId>(body->GetUserData().pointer);
                if (world->isValid(entityId)) {
                    results[index++] = Entity(entityId, world);
                }
            }
            return results;
        });
}

}  // namespace limbo
