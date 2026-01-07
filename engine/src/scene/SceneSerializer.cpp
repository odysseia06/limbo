#include "limbo/scene/SceneSerializer.hpp"
#include "limbo/scene/SchemaMigration.hpp"
#include "limbo/scene/Prefab.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Hierarchy.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

namespace limbo {

// ============================================================================
// JSON serialization for GLM types
// ============================================================================

namespace {

json serializeVec2(const glm::vec2& v) {
    return json::array({v.x, v.y});
}

json serializeVec3(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

json serializeVec4(const glm::vec4& v) {
    return json::array({v.x, v.y, v.z, v.w});
}

glm::vec2 deserializeVec2(const json& j) {
    if (j.is_array() && j.size() >= 2) {
        return glm::vec2(j[0].get<float>(), j[1].get<float>());
    }
    return glm::vec2(0.0f);
}

glm::vec3 deserializeVec3(const json& j) {
    if (j.is_array() && j.size() >= 3) {
        return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }
    return glm::vec3(0.0f);
}

glm::vec4 deserializeVec4(const json& j) {
    if (j.is_array() && j.size() >= 4) {
        return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(),
                         j[3].get<float>());
    }
    return glm::vec4(1.0f);
}

// Build a map from entity ID to index for hierarchy serialization
<<<<<<< HEAD
std::unordered_map<World::EntityId, i32>
buildEntityIndexMap(World& world, const std::vector<World::EntityId>& orderedEntities) {
=======
std::unordered_map<World::EntityId, i32> buildEntityIndexMap(
    World& world, const std::vector<World::EntityId>& orderedEntities) {
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0
    std::unordered_map<World::EntityId, i32> map;
    for (i32 i = 0; i < static_cast<i32>(orderedEntities.size()); ++i) {
        map[orderedEntities[static_cast<usize>(i)]] = i;
    }
    return map;
}

// Get entities in hierarchy order (parents before children)
std::vector<World::EntityId> getEntitiesInHierarchyOrder(World& world) {
    std::vector<World::EntityId> result;
    std::vector<World::EntityId> roots;
    std::vector<World::EntityId> noHierarchy;

    // First pass: find roots and entities without hierarchy
    auto view = world.view<NameComponent>();
    for (auto entityId : view) {
        if (world.hasComponent<HierarchyComponent>(entityId)) {
            const auto& hierarchy = world.getComponent<HierarchyComponent>(entityId);
            if (hierarchy.isRoot()) {
                roots.push_back(entityId);
            }
        } else {
            noHierarchy.push_back(entityId);
        }
    }

    // Add entities without hierarchy first
    for (auto entityId : noHierarchy) {
        result.push_back(entityId);
    }

    // Add hierarchical entities in depth-first order
    std::function<void(World::EntityId)> addWithChildren = [&](World::EntityId entityId) {
        result.push_back(entityId);
        Hierarchy::forEachChild(world, entityId, [&](World::EntityId child) {
            addWithChildren(child);
            return true;
        });
    };

    for (auto root : roots) {
        addWithChildren(root);
    }

    return result;
}

}  // anonymous namespace

// ============================================================================
// SceneSerializer implementation
// ============================================================================

SceneSerializer::SceneSerializer(World& world) : m_world(world) {}

bool SceneSerializer::saveToFile(const std::filesystem::path& path) {
    try {
        String const jsonStr = serialize();

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
    root["version"] = kSceneFormatVersion;
    root["engine"] = "Limbo";

    json entitiesArray = json::array();

    // Get entities in hierarchy order (parents before children)
    auto orderedEntities = getEntitiesInHierarchyOrder(m_world);
    auto entityIndexMap = buildEntityIndexMap(m_world, orderedEntities);

    for (auto entityId : orderedEntities) {
        json entityJson;

        // Name component (required for serialization)
        if (!m_world.hasComponent<NameComponent>(entityId)) {
            continue;
        }
        const auto& name = m_world.getComponent<NameComponent>(entityId);
        entityJson["name"] = name.name;

        // Components object
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

        // Hierarchy component
        if (m_world.hasComponent<HierarchyComponent>(entityId)) {
            const auto& hierarchy = m_world.getComponent<HierarchyComponent>(entityId);
            if (hierarchy.hasParent()) {
                json hierarchyJson;
                auto it = entityIndexMap.find(hierarchy.parent);
                if (it != entityIndexMap.end()) {
                    hierarchyJson["parent"] = it->second;
                }
                componentsJson["Hierarchy"] = hierarchyJson;
            }
        }

        // SpriteRenderer component
        if (m_world.hasComponent<SpriteRendererComponent>(entityId)) {
            const auto& sprite = m_world.getComponent<SpriteRendererComponent>(entityId);
            json spriteJson;
            spriteJson["color"] = serializeVec4(sprite.color);
            spriteJson["sortingOrder"] = sprite.sortingOrder;
            if (sprite.textureId.isValid()) {
                spriteJson["textureId"] = sprite.textureId.uuid().toString();
            }
            spriteJson["uvMin"] = serializeVec2(sprite.uvMin);
            spriteJson["uvMax"] = serializeVec2(sprite.uvMax);
            componentsJson["SpriteRenderer"] = spriteJson;
        }

        // Camera component
        if (m_world.hasComponent<CameraComponent>(entityId)) {
            const auto& camera = m_world.getComponent<CameraComponent>(entityId);
            json cameraJson;
            cameraJson["projectionType"] =
                camera.projectionType == CameraComponent::ProjectionType::Perspective
                    ? "perspective"
                    : "orthographic";
            cameraJson["fov"] = camera.fov;
            cameraJson["orthoSize"] = camera.orthoSize;
            cameraJson["nearClip"] = camera.nearClip;
            cameraJson["farClip"] = camera.farClip;
            cameraJson["primary"] = camera.primary;
            componentsJson["Camera"] = cameraJson;
        }

        // Tag components
        if (m_world.hasComponent<StaticComponent>(entityId)) {
            componentsJson["Static"] = json::object();
        }
        if (m_world.hasComponent<ActiveComponent>(entityId)) {
            componentsJson["Active"] = json::object();
        }

        // PrefabInstance component
        if (m_world.hasComponent<PrefabInstanceComponent>(entityId)) {
            const auto& prefabInstance = m_world.getComponent<PrefabInstanceComponent>(entityId);
            json prefabJson;
            prefabJson["prefabId"] = prefabInstance.prefabId.toString();
            prefabJson["entityIndex"] = prefabInstance.entityIndex;
            prefabJson["isRoot"] = prefabInstance.isRoot;

            // Serialize overrides
            if (!prefabInstance.overrides.empty()) {
                json overridesJson = json::object();
                for (const auto& [key, value] : prefabInstance.overrides) {
                    if (value) {
                        overridesJson[key] = true;
                    }
                }
                if (!overridesJson.empty()) {
                    prefabJson["overrides"] = overridesJson;
                }
            }

            componentsJson["PrefabInstance"] = prefabJson;
        }

        entityJson["components"] = componentsJson;
        entitiesArray.push_back(entityJson);
    }

    root["entities"] = entitiesArray;

    return root.dump(2);  // Pretty print with 2 spaces
}

bool SceneSerializer::deserialize(const String& jsonStr) {
    try {
        json root = json::parse(jsonStr);

        // Validate and get version
        i32 version = 1;
        if (root.contains("version")) {
            if (root["version"].is_number()) {
                version = root["version"].get<i32>();
            } else if (root["version"].is_string()) {
                // Legacy string version like "1.0"
                version = 1;
            }
        }

        // Migrate if needed
        if (version < kSceneFormatVersion) {
            spdlog::info("Scene version {} is older than current {}, migrating...", version,
                         kSceneFormatVersion);
            SchemaMigration migration = SchemaMigration::createSceneMigrationRegistry();
            if (!migration.migrate(root, version, kSceneFormatVersion)) {
                m_error = "Failed to migrate scene: " + migration.getError();
                spdlog::error("{}", m_error);
                return false;
            }
            version = kSceneFormatVersion;
        }

        // Clear existing entities
        m_world.clear();

        // Load entities
        if (!root.contains("entities") || !root["entities"].is_array()) {
            m_error = "Invalid scene file: missing entities array";
            spdlog::error("{}", m_error);
            return false;
        }

        // First pass: create all entities
        std::vector<World::EntityId> loadedEntities;
        for (const auto& entityJson : root["entities"]) {
            String name = "Entity";
            if (entityJson.contains("name")) {
                name = entityJson["name"].get<String>();
            }
            Entity entity = m_world.createEntity(name);
            loadedEntities.push_back(entity.id());
        }

        // Second pass: load components (hierarchy needs all entities to exist first)
        usize entityIndex = 0;
        for (const auto& entityJson : root["entities"]) {
            World::EntityId entityId = loadedEntities[entityIndex++];

            if (!entityJson.contains("components")) {
                continue;
            }

            const auto& components = entityJson["components"];

            // Transform
            if (components.contains("Transform")) {
                const auto& transformJson = components["Transform"];
                auto& transform = m_world.addComponent<TransformComponent>(entityId);

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

            // Hierarchy (version 2+)
            if (version >= 2 && components.contains("Hierarchy")) {
                const auto& hierarchyJson = components["Hierarchy"];
                if (hierarchyJson.contains("parent")) {
                    i32 parentIndex = hierarchyJson["parent"].get<i32>();
                    if (parentIndex >= 0 &&
                        static_cast<usize>(parentIndex) < loadedEntities.size()) {
<<<<<<< HEAD
                        Hierarchy::setParent(m_world, entityId,
                                             loadedEntities[static_cast<usize>(parentIndex)]);
=======
                        Hierarchy::setParent(
                            m_world, entityId,
                            loadedEntities[static_cast<usize>(parentIndex)]);
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0
                    }
                }
            }

            // SpriteRenderer
            if (components.contains("SpriteRenderer")) {
                const auto& spriteJson = components["SpriteRenderer"];
                auto& sprite = m_world.addComponent<SpriteRendererComponent>(entityId);

                if (spriteJson.contains("color")) {
                    sprite.color = deserializeVec4(spriteJson["color"]);
                }
                if (spriteJson.contains("sortingOrder")) {
                    sprite.sortingOrder = spriteJson["sortingOrder"].get<i32>();
                }
                if (spriteJson.contains("textureId")) {
                    String uuidStr = spriteJson["textureId"].get<String>();
                    sprite.textureId = AssetId(UUID::fromString(uuidStr));
                }
                if (spriteJson.contains("uvMin")) {
                    sprite.uvMin = deserializeVec2(spriteJson["uvMin"]);
                }
                if (spriteJson.contains("uvMax")) {
                    sprite.uvMax = deserializeVec2(spriteJson["uvMax"]);
                }
            }

            // Camera
            if (components.contains("Camera")) {
                const auto& cameraJson = components["Camera"];
                auto& camera = m_world.addComponent<CameraComponent>(entityId);

                if (cameraJson.contains("projectionType")) {
                    String projType = cameraJson["projectionType"].get<String>();
<<<<<<< HEAD
                    camera.projectionType = (projType == "orthographic")
                                                ? CameraComponent::ProjectionType::Orthographic
                                                : CameraComponent::ProjectionType::Perspective;
=======
                    camera.projectionType =
                        (projType == "orthographic")
                            ? CameraComponent::ProjectionType::Orthographic
                            : CameraComponent::ProjectionType::Perspective;
>>>>>>> 06875892ed8995d879d0cd1681cf1409670aa9f0
                }
                if (cameraJson.contains("fov")) {
                    camera.fov = cameraJson["fov"].get<f32>();
                }
                if (cameraJson.contains("orthoSize")) {
                    camera.orthoSize = cameraJson["orthoSize"].get<f32>();
                }
                if (cameraJson.contains("nearClip")) {
                    camera.nearClip = cameraJson["nearClip"].get<f32>();
                }
                if (cameraJson.contains("farClip")) {
                    camera.farClip = cameraJson["farClip"].get<f32>();
                }
                if (cameraJson.contains("primary")) {
                    camera.primary = cameraJson["primary"].get<bool>();
                }
            }

            // Tag components
            if (components.contains("Static")) {
                m_world.addComponent<StaticComponent>(entityId);
            }
            if (components.contains("Active")) {
                m_world.addComponent<ActiveComponent>(entityId);
            }

            // PrefabInstance
            if (components.contains("PrefabInstance")) {
                const auto& prefabJson = components["PrefabInstance"];
                auto& prefabInstance = m_world.addComponent<PrefabInstanceComponent>(entityId);

                if (prefabJson.contains("prefabId")) {
                    prefabInstance.prefabId =
                        UUID::fromString(prefabJson["prefabId"].get<String>());
                }
                if (prefabJson.contains("entityIndex")) {
                    prefabInstance.entityIndex = prefabJson["entityIndex"].get<i32>();
                }
                if (prefabJson.contains("isRoot")) {
                    prefabInstance.isRoot = prefabJson["isRoot"].get<bool>();
                }
                if (prefabJson.contains("overrides")) {
                    for (auto& [key, value] : prefabJson["overrides"].items()) {
                        if (value.is_boolean() && value.get<bool>()) {
                            prefabInstance.overrides[key] = true;
                        }
                    }
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

}  // namespace limbo
