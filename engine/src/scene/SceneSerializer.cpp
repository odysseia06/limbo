#include "limbo/scene/SceneSerializer.hpp"
#include "limbo/scene/SchemaMigration.hpp"
#include "limbo/scene/Prefab.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Hierarchy.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/debug/Log.hpp"

#include <nlohmann/json.hpp>

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
    if (j.is_object() && j.contains("x") && j.contains("y")) {
        return glm::vec2(j["x"].get<float>(), j["y"].get<float>());
    }
    return glm::vec2(0.0f);
}

glm::vec3 deserializeVec3(const json& j) {
    if (j.is_array() && j.size() >= 3) {
        return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }
    if (j.is_object() && j.contains("x") && j.contains("y") && j.contains("z")) {
        return glm::vec3(j["x"].get<float>(), j["y"].get<float>(), j["z"].get<float>());
    }
    return glm::vec3(0.0f);
}

glm::vec4 deserializeVec4(const json& j) {
    if (j.is_array() && j.size() >= 4) {
        return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(),
                         j[3].get<float>());
    }
    if (j.is_object() && j.contains("r") && j.contains("g") && j.contains("b") && j.contains("a")) {
        return glm::vec4(j["r"].get<float>(), j["g"].get<float>(), j["b"].get<float>(),
                         j["a"].get<float>());
    }
    return glm::vec4(1.0f);
}

// Build a map from entity ID to index for hierarchy serialization
std::unordered_map<World::EntityId, i32>
buildEntityIndexMap(const std::vector<World::EntityId>& orderedEntities) {
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
            LIMBO_LOG_CORE_ERROR("{}", m_error);
            return false;
        }

        file << jsonStr;
        file.close();

        LIMBO_LOG_CORE_INFO("Scene saved to: {}", path.string());
        return true;
    } catch (const std::exception& e) {
        m_error = "Failed to save scene: " + String(e.what());
        LIMBO_LOG_CORE_ERROR("{}", m_error);
        return false;
    }
}

bool SceneSerializer::loadFromFile(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            m_error = "Failed to open file for reading: " + path.string();
            LIMBO_LOG_CORE_ERROR("{}", m_error);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        if (deserialize(buffer.str())) {
            LIMBO_LOG_CORE_INFO("Scene loaded from: {}", path.string());
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        m_error = "Failed to load scene: " + String(e.what());
        LIMBO_LOG_CORE_ERROR("{}", m_error);
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
    auto entityIndexMap = buildEntityIndexMap(orderedEntities);

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
            spriteJson["sortingLayer"] = sprite.sortingLayer;
            spriteJson["sortingOrder"] = sprite.sortingOrder;
            if (sprite.textureId.isValid()) {
                spriteJson["textureId"] = sprite.textureId.uuid().toString();
            }
            spriteJson["uvMin"] = serializeVec2(sprite.uvMin);
            spriteJson["uvMax"] = serializeVec2(sprite.uvMax);
            componentsJson["SpriteRenderer"] = spriteJson;
        }

        // QuadRenderer component
        if (m_world.hasComponent<QuadRendererComponent>(entityId)) {
            const auto& quad = m_world.getComponent<QuadRendererComponent>(entityId);
            json quadJson;
            quadJson["color"] = serializeVec4(quad.color);
            quadJson["size"] = serializeVec2(quad.size);
            quadJson["sortingLayer"] = quad.sortingLayer;
            quadJson["sortingOrder"] = quad.sortingOrder;
            componentsJson["QuadRenderer"] = quadJson;
        }

        // CircleRenderer component
        if (m_world.hasComponent<CircleRendererComponent>(entityId)) {
            const auto& circle = m_world.getComponent<CircleRendererComponent>(entityId);
            json circleJson;
            circleJson["color"] = serializeVec4(circle.color);
            circleJson["radius"] = circle.radius;
            circleJson["segments"] = circle.segments;
            circleJson["sortingLayer"] = circle.sortingLayer;
            circleJson["sortingOrder"] = circle.sortingOrder;
            componentsJson["CircleRenderer"] = circleJson;
        }

        // TextRenderer component
        if (m_world.hasComponent<TextRendererComponent>(entityId)) {
            const auto& textComp = m_world.getComponent<TextRendererComponent>(entityId);
            json textJson;
            textJson["text"] = textComp.text;
            if (textComp.fontId.isValid()) {
                textJson["fontId"] = textComp.fontId.uuid().toString();
            }
            textJson["scale"] = textComp.scale;
            textJson["color"] = serializeVec4(textComp.color);
            textJson["sortingLayer"] = textComp.sortingLayer;
            textJson["sortingOrder"] = textComp.sortingOrder;
            componentsJson["TextRenderer"] = textJson;
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
            prefabJson["instanceId"] = prefabInstance.instanceId.toString();
            prefabJson["localId"] = prefabInstance.localId;
            prefabJson["isRoot"] = prefabInstance.isRoot;

            // Serialize overrides with actual values
            if (!prefabInstance.overrides.empty()) {
                json overridesArray = json::array();
                for (const auto& ov : prefabInstance.overrides) {
                    json ovJson;
                    ovJson["kind"] = static_cast<int>(ov.kind);
                    ovJson["targetLocalId"] = ov.targetLocalId;
                    ovJson["component"] = ov.component;
                    ovJson["property"] = ov.property;
                    ovJson["value"] = ov.value;
                    overridesArray.push_back(ovJson);
                }
                prefabJson["overrides"] = overridesArray;
            }

            componentsJson["PrefabInstance"] = prefabJson;
        }

        // Rigidbody2D component
        if (m_world.hasComponent<Rigidbody2DComponent>(entityId)) {
            const auto& rb = m_world.getComponent<Rigidbody2DComponent>(entityId);
            json rbJson;
            rbJson["type"] = static_cast<int>(rb.type);
            rbJson["gravityScale"] = rb.gravityScale;
            rbJson["fixedRotation"] = rb.fixedRotation;
            rbJson["linearVelocity"] = serializeVec2(rb.linearVelocity);
            rbJson["angularVelocity"] = rb.angularVelocity;
            rbJson["linearDamping"] = rb.linearDamping;
            rbJson["angularDamping"] = rb.angularDamping;
            componentsJson["Rigidbody2D"] = rbJson;
        }

        // BoxCollider2D component
        if (m_world.hasComponent<BoxCollider2DComponent>(entityId)) {
            const auto& box = m_world.getComponent<BoxCollider2DComponent>(entityId);
            json boxJson;
            boxJson["size"] = serializeVec2(box.size);
            boxJson["offset"] = serializeVec2(box.offset);
            boxJson["density"] = box.density;
            boxJson["friction"] = box.friction;
            boxJson["restitution"] = box.restitution;
            boxJson["restitutionThreshold"] = box.restitutionThreshold;
            boxJson["isTrigger"] = box.isTrigger;
            componentsJson["BoxCollider2D"] = boxJson;
        }

        // CircleCollider2D component
        if (m_world.hasComponent<CircleCollider2DComponent>(entityId)) {
            const auto& circle = m_world.getComponent<CircleCollider2DComponent>(entityId);
            json circleJson;
            circleJson["radius"] = circle.radius;
            circleJson["offset"] = serializeVec2(circle.offset);
            circleJson["density"] = circle.density;
            circleJson["friction"] = circle.friction;
            circleJson["restitution"] = circle.restitution;
            circleJson["restitutionThreshold"] = circle.restitutionThreshold;
            circleJson["isTrigger"] = circle.isTrigger;
            componentsJson["CircleCollider2D"] = circleJson;
        }

        // Script component
        if (m_world.hasComponent<ScriptComponent>(entityId)) {
            const auto& script = m_world.getComponent<ScriptComponent>(entityId);
            json scriptJson;
            scriptJson["scriptPath"] = script.scriptPath.string();
            scriptJson["enabled"] = script.enabled;
            componentsJson["Script"] = scriptJson;
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
            LIMBO_LOG_CORE_INFO("Scene version {} is older than current {}, migrating...", version,
                                kSceneFormatVersion);
            SchemaMigration migration = SchemaMigration::createSceneMigrationRegistry();
            if (!migration.migrate(root, version, kSceneFormatVersion)) {
                m_error = "Failed to migrate scene: " + migration.getError();
                LIMBO_LOG_CORE_ERROR("{}", m_error);
                return false;
            }
            version = kSceneFormatVersion;
        }

        // Clear existing entities
        m_world.clear();

        // Load entities
        if (!root.contains("entities") || !root["entities"].is_array()) {
            m_error = "Invalid scene file: missing entities array";
            LIMBO_LOG_CORE_ERROR("{}", m_error);
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
                        Hierarchy::setParent(m_world, entityId,
                                             loadedEntities[static_cast<usize>(parentIndex)]);
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
                if (spriteJson.contains("sortingLayer")) {
                    sprite.sortingLayer = spriteJson["sortingLayer"].get<i32>();
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

            // QuadRenderer
            if (components.contains("QuadRenderer")) {
                const auto& quadJson = components["QuadRenderer"];
                auto& quad = m_world.addComponent<QuadRendererComponent>(entityId);

                if (quadJson.contains("color")) {
                    quad.color = deserializeVec4(quadJson["color"]);
                }
                if (quadJson.contains("size")) {
                    quad.size = deserializeVec2(quadJson["size"]);
                }
                if (quadJson.contains("sortingLayer")) {
                    quad.sortingLayer = quadJson["sortingLayer"].get<i32>();
                }
                if (quadJson.contains("sortingOrder")) {
                    quad.sortingOrder = quadJson["sortingOrder"].get<i32>();
                }
            }

            // CircleRenderer
            if (components.contains("CircleRenderer")) {
                const auto& circleJson = components["CircleRenderer"];
                auto& circle = m_world.addComponent<CircleRendererComponent>(entityId);

                if (circleJson.contains("color")) {
                    circle.color = deserializeVec4(circleJson["color"]);
                }
                if (circleJson.contains("radius")) {
                    circle.radius = circleJson["radius"].get<f32>();
                }
                if (circleJson.contains("segments")) {
                    circle.segments = circleJson["segments"].get<i32>();
                }
                if (circleJson.contains("sortingLayer")) {
                    circle.sortingLayer = circleJson["sortingLayer"].get<i32>();
                }
                if (circleJson.contains("sortingOrder")) {
                    circle.sortingOrder = circleJson["sortingOrder"].get<i32>();
                }
            }

            // TextRenderer
            if (components.contains("TextRenderer")) {
                const auto& textJson = components["TextRenderer"];
                auto& textComp = m_world.addComponent<TextRendererComponent>(entityId);

                if (textJson.contains("text")) {
                    textComp.text = textJson["text"].get<String>();
                }
                if (textJson.contains("fontId")) {
                    String uuidStr = textJson["fontId"].get<String>();
                    textComp.fontId = AssetId(UUID::fromString(uuidStr));
                }
                if (textJson.contains("scale")) {
                    textComp.scale = textJson["scale"].get<f32>();
                }
                if (textJson.contains("color")) {
                    textComp.color = deserializeVec4(textJson["color"]);
                }
                if (textJson.contains("sortingLayer")) {
                    textComp.sortingLayer = textJson["sortingLayer"].get<i32>();
                }
                if (textJson.contains("sortingOrder")) {
                    textComp.sortingOrder = textJson["sortingOrder"].get<i32>();
                }
            }

            // Camera
            if (components.contains("Camera")) {
                const auto& cameraJson = components["Camera"];
                auto& camera = m_world.addComponent<CameraComponent>(entityId);

                if (cameraJson.contains("projectionType")) {
                    String projType = cameraJson["projectionType"].get<String>();
                    camera.projectionType = (projType == "orthographic")
                                                ? CameraComponent::ProjectionType::Orthographic
                                                : CameraComponent::ProjectionType::Perspective;
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
                if (prefabJson.contains("instanceId")) {
                    prefabInstance.instanceId =
                        UUID::fromString(prefabJson["instanceId"].get<String>());
                }
                if (prefabJson.contains("localId")) {
                    prefabInstance.localId = prefabJson["localId"].get<String>();
                }
                if (prefabJson.contains("isRoot")) {
                    prefabInstance.isRoot = prefabJson["isRoot"].get<bool>();
                }
                if (prefabJson.contains("overrides")) {
                    const auto& overridesJson = prefabJson["overrides"];
                    if (overridesJson.is_array()) {
                        // New format: array of override objects
                        for (const auto& ovJson : overridesJson) {
                            PrefabOverride ov;
                            if (ovJson.contains("kind")) {
                                ov.kind =
                                    static_cast<PrefabOverride::Kind>(ovJson["kind"].get<int>());
                            }
                            if (ovJson.contains("targetLocalId")) {
                                ov.targetLocalId = ovJson["targetLocalId"].get<String>();
                            }
                            if (ovJson.contains("component")) {
                                ov.component = ovJson["component"].get<String>();
                            }
                            if (ovJson.contains("property")) {
                                ov.property = ovJson["property"].get<String>();
                            }
                            if (ovJson.contains("value")) {
                                ov.value = ovJson["value"];
                            }
                            prefabInstance.overrides.push_back(ov);
                        }
                    } else if (overridesJson.is_object()) {
                        // Legacy format: object with "Component.property": true
                        for (auto& [key, value] : overridesJson.items()) {
                            if (value.is_boolean() && value.get<bool>()) {
                                // Parse "Component.property" format
                                auto dotPos = key.find('.');
                                if (dotPos != String::npos) {
                                    PrefabOverride ov;
                                    ov.kind = PrefabOverride::Kind::Property;
                                    ov.targetLocalId = prefabInstance.localId;
                                    ov.component = key.substr(0, dotPos);
                                    ov.property = key.substr(dotPos + 1);
                                    // Value not available in legacy format
                                    prefabInstance.overrides.push_back(ov);
                                }
                            }
                        }
                    }
                }
            }

            // Rigidbody2D component
            if (components.contains("Rigidbody2D")) {
                const auto& rbJson = components["Rigidbody2D"];
                auto& rb = m_world.addComponent<Rigidbody2DComponent>(entityId);

                if (rbJson.contains("type")) {
                    rb.type = static_cast<BodyType>(rbJson["type"].get<int>());
                }
                if (rbJson.contains("fixedRotation")) {
                    rb.fixedRotation = rbJson["fixedRotation"].get<bool>();
                }
                if (rbJson.contains("gravityScale")) {
                    rb.gravityScale = rbJson["gravityScale"].get<float>();
                }
                if (rbJson.contains("linearDamping")) {
                    rb.linearDamping = rbJson["linearDamping"].get<float>();
                }
                if (rbJson.contains("angularDamping")) {
                    rb.angularDamping = rbJson["angularDamping"].get<float>();
                }
            }

            // BoxCollider2D component
            if (components.contains("BoxCollider2D")) {
                const auto& boxJson = components["BoxCollider2D"];
                auto& box = m_world.addComponent<BoxCollider2DComponent>(entityId);

                if (boxJson.contains("size")) {
                    box.size = deserializeVec2(boxJson["size"]);
                }
                if (boxJson.contains("offset")) {
                    box.offset = deserializeVec2(boxJson["offset"]);
                }
                if (boxJson.contains("density")) {
                    box.density = boxJson["density"].get<float>();
                }
                if (boxJson.contains("friction")) {
                    box.friction = boxJson["friction"].get<float>();
                }
                if (boxJson.contains("restitution")) {
                    box.restitution = boxJson["restitution"].get<float>();
                }
                if (boxJson.contains("isTrigger")) {
                    box.isTrigger = boxJson["isTrigger"].get<bool>();
                }
            }

            // CircleCollider2D component
            if (components.contains("CircleCollider2D")) {
                const auto& circleJson = components["CircleCollider2D"];
                auto& circle = m_world.addComponent<CircleCollider2DComponent>(entityId);

                if (circleJson.contains("radius")) {
                    circle.radius = circleJson["radius"].get<float>();
                }
                if (circleJson.contains("offset")) {
                    circle.offset = deserializeVec2(circleJson["offset"]);
                }
                if (circleJson.contains("density")) {
                    circle.density = circleJson["density"].get<float>();
                }
                if (circleJson.contains("friction")) {
                    circle.friction = circleJson["friction"].get<float>();
                }
                if (circleJson.contains("restitution")) {
                    circle.restitution = circleJson["restitution"].get<float>();
                }
                if (circleJson.contains("isTrigger")) {
                    circle.isTrigger = circleJson["isTrigger"].get<bool>();
                }
            }

            // Script component
            if (components.contains("Script")) {
                const auto& scriptJson = components["Script"];
                auto& script = m_world.addComponent<ScriptComponent>(entityId);

                if (scriptJson.contains("scriptPath")) {
                    script.scriptPath = scriptJson["scriptPath"].get<String>();
                }
                if (scriptJson.contains("enabled")) {
                    script.enabled = scriptJson["enabled"].get<bool>();
                }
            }
        }

        LIMBO_LOG_CORE_INFO("Loaded {} entities", m_world.entityCount());
        return true;
    } catch (const json::exception& e) {
        m_error = "JSON parse error: " + String(e.what());
        LIMBO_LOG_CORE_ERROR("{}", m_error);
        return false;
    } catch (const std::exception& e) {
        m_error = "Deserialization error: " + String(e.what());
        LIMBO_LOG_CORE_ERROR("{}", m_error);
        return false;
    }
}

}  // namespace limbo
