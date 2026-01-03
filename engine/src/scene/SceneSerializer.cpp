#include "limbo/scene/SceneSerializer.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Entity.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

using json = nlohmann::json;

namespace limbo {

// ============================================================================
// JSON serialization for GLM types
// ============================================================================

namespace {

json serializeVec3(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

json serializeVec4(const glm::vec4& v) {
    return json::array({v.x, v.y, v.z, v.w});
}

glm::vec3 deserializeVec3(const json& j) {
    if (j.is_array() && j.size() >= 3) {
        return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }
    return glm::vec3(0.0f);
}

glm::vec4 deserializeVec4(const json& j) {
    if (j.is_array() && j.size() >= 4) {
        return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
    }
    return glm::vec4(1.0f);
}

} // anonymous namespace

// ============================================================================
// SceneSerializer implementation
// ============================================================================

SceneSerializer::SceneSerializer(World& world)
    : m_world(world) {
}

bool SceneSerializer::saveToFile(const std::filesystem::path& path) {
    try {
        String jsonStr = serialize();
        
        std::ofstream file(path);
        if (!file.is_open()) {
            m_error = "Failed to open file for writing: " + path.string();
            spdlog::error("{}", m_error);
            return false;
        }
        
        file << jsonStr;
        file.close();
        
        spdlog::info("Scene saved to: {}", path.string());
        return true;
    } catch (const std::exception& e) {
        m_error = "Failed to save scene: " + String(e.what());
        spdlog::error("{}", m_error);
        return false;
    }
}

bool SceneSerializer::loadFromFile(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            m_error = "Failed to open file for reading: " + path.string();
            spdlog::error("{}", m_error);
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        if (deserialize(buffer.str())) {
            spdlog::info("Scene loaded from: {}", path.string());
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        m_error = "Failed to load scene: " + String(e.what());
        spdlog::error("{}", m_error);
        return false;
    }
}

String SceneSerializer::serialize() {
    json root;
    root["version"] = "1.0";
    root["engine"] = "Limbo";
    
    json entitiesArray = json::array();
    
    // Iterate all entities with NameComponent
    auto view = m_world.view<NameComponent>();
    for (auto entityId : view) {
        json entityJson;
        
        // Name component (required for serialization)
        const auto& name = view.get<NameComponent>(entityId);
        entityJson["name"] = name.name;
        
        // Components array
        json componentsJson = json::object();
        
        // Transform component
        if (m_world.hasComponent<TransformComponent>(entityId)) {
            const auto& transform = m_world.getComponent<TransformComponent>(entityId);
            json transformJson;
            transformJson["position"] = serializeVec3(transform.position);
            transformJson["rotation"] = serializeVec3(transform.rotation);
            transformJson["scale"] = serializeVec3(transform.scale);
            componentsJson["Transform"] = transformJson;
        }
        
        // SpriteRenderer component
        if (m_world.hasComponent<SpriteRendererComponent>(entityId)) {
            const auto& sprite = m_world.getComponent<SpriteRendererComponent>(entityId);
            json spriteJson;
            spriteJson["color"] = serializeVec4(sprite.color);
            // TODO: Add texture path when TextureAsset reference is added
            componentsJson["SpriteRenderer"] = spriteJson;
        }
        
        // Static component (tag)
        if (m_world.hasComponent<StaticComponent>(entityId)) {
            componentsJson["Static"] = json::object();
        }
        
        entityJson["components"] = componentsJson;
        entitiesArray.push_back(entityJson);
    }
    
    root["entities"] = entitiesArray;
    
    return root.dump(2); // Pretty print with 2 spaces
}

bool SceneSerializer::deserialize(const String& jsonStr) {
    try {
        json root = json::parse(jsonStr);
        
        // Validate version
        if (!root.contains("version")) {
            m_error = "Invalid scene file: missing version";
            spdlog::error("{}", m_error);
            return false;
        }
        
        // Clear existing entities
        m_world.clear();
        
        // Load entities
        if (!root.contains("entities") || !root["entities"].is_array()) {
            m_error = "Invalid scene file: missing entities array";
            spdlog::error("{}", m_error);
            return false;
        }
        
        for (const auto& entityJson : root["entities"]) {
            // Get name
            String name = "Entity";
            if (entityJson.contains("name")) {
                name = entityJson["name"].get<String>();
            }
            
            // Create entity
            Entity entity = m_world.createEntity(name);
            
            // Load components
            if (entityJson.contains("components")) {
                const auto& components = entityJson["components"];
                
                // Transform
                if (components.contains("Transform")) {
                    const auto& transformJson = components["Transform"];
                    auto& transform = entity.addComponent<TransformComponent>();
                    
                    if (transformJson.contains("position")) {
                        transform.position = deserializeVec3(transformJson["position"]);
                    }
                    if (transformJson.contains("rotation")) {
                        transform.rotation = deserializeVec3(transformJson["rotation"]);
                    }
                    if (transformJson.contains("scale")) {
                        transform.scale = deserializeVec3(transformJson["scale"]);
                    }
                }
                
                // SpriteRenderer
                if (components.contains("SpriteRenderer")) {
                    const auto& spriteJson = components["SpriteRenderer"];
                    glm::vec4 color(1.0f);
                    
                    if (spriteJson.contains("color")) {
                        color = deserializeVec4(spriteJson["color"]);
                    }
                    
                    entity.addComponent<SpriteRendererComponent>(color);
                }
                
                // Static
                if (components.contains("Static")) {
                    entity.addComponent<StaticComponent>();
                }
            }
        }
        
        spdlog::info("Loaded {} entities", m_world.entityCount());
        return true;
    } catch (const json::exception& e) {
        m_error = "JSON parse error: " + String(e.what());
        spdlog::error("{}", m_error);
        return false;
    } catch (const std::exception& e) {
        m_error = "Deserialization error: " + String(e.what());
        spdlog::error("{}", m_error);
        return false;
    }
}

} // namespace limbo
