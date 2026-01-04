#include "limbo/scripting/ScriptEngine.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/platform/Input.hpp"

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

namespace limbo {

ScriptEngine::~ScriptEngine() {
    shutdown();
}

bool ScriptEngine::init() {
    if (m_initialized) {
        return true;
    }

    // Open standard Lua libraries
    m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math,
                         sol::lib::table, sol::lib::os, sol::lib::io);

    // Register engine bindings
    registerBindings();

    m_initialized = true;
    spdlog::info("ScriptEngine initialized");
    return true;
}

void ScriptEngine::shutdown() {
    if (!m_initialized) {
        return;
    }

    m_boundWorld = nullptr;
    m_initialized = false;
    spdlog::info("ScriptEngine shutdown");
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
            spdlog::error("Lua script error in {}: {}", path.string(), m_lastError);
            return false;
        }
        spdlog::debug("Loaded script: {}", path.string());
        return true;
    } catch (const sol::error& e) {
        m_lastError = e.what();
        spdlog::error("Lua exception in {}: {}", path.string(), m_lastError);
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
            spdlog::error("Lua error: {}", m_lastError);
            return false;
        }
        return true;
    } catch (const sol::error& e) {
        m_lastError = e.what();
        spdlog::error("Lua exception: {}", m_lastError);
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
    bindEntityTypes();
    bindComponentTypes();
}

void ScriptEngine::bindMathTypes() {
    // Vec2
    m_lua.new_usertype<glm::vec2>(
        "Vec2", sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(), "x",
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
        "Vec3", sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z, sol::meta_function::addition,
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
        "Vec4",
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x, "y", &glm::vec4::y, "z", &glm::vec4::z, "w", &glm::vec4::w, "r",
        &glm::vec4::r, "g", &glm::vec4::g, "b", &glm::vec4::b, "a", &glm::vec4::a);

    // Color alias
    m_lua["Color"] = m_lua["Vec4"];
}

void ScriptEngine::bindInputTypes() {
    // Input table
    auto input = m_lua.create_named_table("Input");

    // Key checks
    input["isKeyDown"] = [](int key) { return Input::isKeyDown(static_cast<Key>(key)); };
    input["isKeyPressed"] = [](int key) { return Input::isKeyPressed(static_cast<Key>(key)); };
    input["isKeyReleased"] = [](int key) { return Input::isKeyReleased(static_cast<Key>(key)); };

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

void ScriptEngine::bindEntityTypes() {
    if (!m_boundWorld) {
        return;
    }

    // Entity handle for Lua
    m_lua.new_usertype<Entity>("Entity", sol::no_constructor, "isValid", &Entity::isValid, "getId",
                               &entityGetId, "getName", &entityGetName, "destroy",
                               &Entity::destroy);

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

// Static helper functions for component access
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

void ScriptEngine::bindComponentTypes() {
    if (!m_boundWorld) {
        return;
    }

    // TransformComponent access via Entity
    m_lua["Entity"]["getPosition"] = &entityGetPosition;
    m_lua["Entity"]["setPosition"] = &entitySetPosition;
    m_lua["Entity"]["getRotation"] = &entityGetRotation;
    m_lua["Entity"]["setRotation"] = &entitySetRotation;
    m_lua["Entity"]["getScale"] = &entityGetScale;
    m_lua["Entity"]["setScale"] = &entitySetScale;

    // SpriteRendererComponent access
    m_lua["Entity"]["getColor"] = &entityGetColor;
    m_lua["Entity"]["setColor"] = &entitySetColor;

    // Add components
    m_lua["Entity"]["addTransform"] = &entityAddTransform;
    m_lua["Entity"]["addSprite"] = &entityAddSprite;
}

void ScriptEngine::bindUtilityFunctions() {
    // Logging
    m_lua["print"] = [](const String& msg) { spdlog::info("[Lua] {}", msg); };
    m_lua["log"] = m_lua.create_table_with(
        "info", [](const String& msg) { spdlog::info("[Lua] {}", msg); }, "warn",
        [](const String& msg) { spdlog::warn("[Lua] {}", msg); }, "error",
        [](const String& msg) { spdlog::error("[Lua] {}", msg); }, "debug",
        [](const String& msg) { spdlog::debug("[Lua] {}", msg); });

    // Time (will be set each frame by ScriptSystem)
    m_lua["Time"] = m_lua.create_table_with("deltaTime", 0.0f, "totalTime", 0.0f);

    // Math utilities
    m_lua["math"]["clamp"] = [](float value, float min, float max) {
        return glm::clamp(value, min, max);
    };
    m_lua["math"]["lerp"] = [](float a, float b, float t) { return glm::mix(a, b, t); };
}

}  // namespace limbo
