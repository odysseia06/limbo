#include "limbo/scene/Prefab.hpp"

#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Hierarchy.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

using json = nlohmann::json;

namespace limbo {

namespace {

// JSON helpers for GLM types
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

}  // namespace

Prefab Prefab::createFromEntity(World& world, World::EntityId rootEntity) {
    Prefab prefab;

    if (!world.isValid(rootEntity)) {
        spdlog::error("Prefab::createFromEntity: Invalid root entity");
        return prefab;
    }

    // Get name from entity
    if (world.hasComponent<NameComponent>(rootEntity)) {
        prefab.m_name = world.getComponent<NameComponent>(rootEntity).name;
    }

    // Build entity list in hierarchy order
    std::vector<World::EntityId> entityList;
    std::unordered_map<World::EntityId, i32> entityIndexMap;

    // Recursive function to collect entities
    std::function<void(World::EntityId, i32)> collectEntities = [&](World::EntityId entityId,
                                                                    i32 parentIndex) {
        i32 currentIndex = static_cast<i32>(entityList.size());
        entityList.push_back(entityId);
        entityIndexMap[entityId] = currentIndex;

        // Serialize this entity
        prefab.m_entities.push_back(serializeEntity(world, entityId, parentIndex));

        // Process children
        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            collectEntities(childId, currentIndex);
            return true;
        });
    };

    collectEntities(rootEntity, -1);

    return prefab;
}

PrefabEntity Prefab::serializeEntity(World& world, World::EntityId entityId, i32 parentIndex) {
    PrefabEntity prefabEntity;
    prefabEntity.parentIndex = parentIndex;

    // Name
    if (world.hasComponent<NameComponent>(entityId)) {
        prefabEntity.name = world.getComponent<NameComponent>(entityId).name;
    } else {
        prefabEntity.name = "Entity";
    }

    // Transform
    if (world.hasComponent<TransformComponent>(entityId)) {
        const auto& transform = world.getComponent<TransformComponent>(entityId);
        json transformJson;
        transformJson["position"] = serializeVec3(transform.position);
        transformJson["rotation"] = serializeVec3(transform.rotation);
        transformJson["scale"] = serializeVec3(transform.scale);
        prefabEntity.transformData = transformJson.dump();
    }

    // SpriteRenderer
    if (world.hasComponent<SpriteRendererComponent>(entityId)) {
        const auto& sprite = world.getComponent<SpriteRendererComponent>(entityId);
        json spriteJson;
        spriteJson["color"] = serializeVec4(sprite.color);
        spriteJson["sortingOrder"] = sprite.sortingOrder;
        if (sprite.textureId.isValid()) {
            spriteJson["textureId"] = sprite.textureId.uuid().toString();
        }
        spriteJson["uvMin"] = serializeVec2(sprite.uvMin);
        spriteJson["uvMax"] = serializeVec2(sprite.uvMax);
        prefabEntity.spriteRendererData = spriteJson.dump();
    }

    // Camera
    if (world.hasComponent<CameraComponent>(entityId)) {
        const auto& camera = world.getComponent<CameraComponent>(entityId);
        json cameraJson;
        cameraJson["projectionType"] =
            camera.projectionType == CameraComponent::ProjectionType::Perspective ? "perspective"
                                                                                   : "orthographic";
        cameraJson["fov"] = camera.fov;
        cameraJson["orthoSize"] = camera.orthoSize;
        cameraJson["nearClip"] = camera.nearClip;
        cameraJson["farClip"] = camera.farClip;
        cameraJson["primary"] = camera.primary;
        prefabEntity.cameraData = cameraJson.dump();
    }

    // Tags
    prefabEntity.hasStaticComponent = world.hasComponent<StaticComponent>(entityId);
    prefabEntity.hasActiveComponent = world.hasComponent<ActiveComponent>(entityId);

    return prefabEntity;
}

Entity Prefab::instantiate(World& world, const glm::vec3& position) const {
    if (m_entities.empty()) {
        spdlog::error("Prefab::instantiate: Prefab has no entities");
        return Entity();
    }

    std::vector<World::EntityId> createdEntities;
    createdEntities.reserve(m_entities.size());

    // Create all entities
    for (usize i = 0; i < m_entities.size(); ++i) {
        World::EntityId entityId = deserializeEntity(world, m_entities[i], createdEntities);
        createdEntities.push_back(entityId);

        // Add PrefabInstanceComponent
        world.addComponent<PrefabInstanceComponent>(entityId, m_prefabId, static_cast<i32>(i),
                                                    i == 0);
    }

    // Set up hierarchy
    for (usize i = 0; i < m_entities.size(); ++i) {
        if (m_entities[i].parentIndex >= 0) {
            Hierarchy::setParent(world, createdEntities[i],
                                 createdEntities[static_cast<usize>(m_entities[i].parentIndex)]);
        }
    }

    // Apply position offset to root
    if (!createdEntities.empty() && world.hasComponent<TransformComponent>(createdEntities[0])) {
        auto& transform = world.getComponent<TransformComponent>(createdEntities[0]);
        transform.position += position;
    }

    return Entity(createdEntities[0], &world);
}

World::EntityId Prefab::deserializeEntity(
    World& world, const PrefabEntity& prefabEntity,
    [[maybe_unused]] const std::vector<World::EntityId>& createdEntities) const {
    Entity entity = world.createEntity(prefabEntity.name);

    // Transform
    if (prefabEntity.transformData.has_value()) {
        json transformJson = json::parse(prefabEntity.transformData.value());
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
    if (prefabEntity.spriteRendererData.has_value()) {
        json spriteJson = json::parse(prefabEntity.spriteRendererData.value());
        auto& sprite = entity.addComponent<SpriteRendererComponent>();
        if (spriteJson.contains("color")) {
            sprite.color = deserializeVec4(spriteJson["color"]);
        }
        if (spriteJson.contains("sortingOrder")) {
            sprite.sortingOrder = spriteJson["sortingOrder"].get<i32>();
        }
        if (spriteJson.contains("textureId")) {
            sprite.textureId = AssetId(UUID::fromString(spriteJson["textureId"].get<String>()));
        }
        if (spriteJson.contains("uvMin")) {
            sprite.uvMin = deserializeVec2(spriteJson["uvMin"]);
        }
        if (spriteJson.contains("uvMax")) {
            sprite.uvMax = deserializeVec2(spriteJson["uvMax"]);
        }
    }

    // Camera
    if (prefabEntity.cameraData.has_value()) {
        json cameraJson = json::parse(prefabEntity.cameraData.value());
        auto& camera = entity.addComponent<CameraComponent>();
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

    // Tags
    if (prefabEntity.hasStaticComponent) {
        entity.addComponent<StaticComponent>();
    }
    if (prefabEntity.hasActiveComponent) {
        entity.addComponent<ActiveComponent>();
    }

    return entity.id();
}

void Prefab::updateInstances(World& world, bool respectOverrides) const {
    // Find all instances of this prefab
    auto view = world.view<PrefabInstanceComponent>();

    for (auto entityId : view) {
        auto& instance = view.get<PrefabInstanceComponent>(entityId);

        // Only update if this is from our prefab and it's in range
        if (instance.prefabId != m_prefabId) {
            continue;
        }
        if (instance.entityIndex < 0 ||
            static_cast<usize>(instance.entityIndex) >= m_entities.size()) {
            continue;
        }

        const PrefabEntity& prefabEntity = m_entities[static_cast<usize>(instance.entityIndex)];

        // Update transform (if not overridden)
        if (prefabEntity.transformData.has_value() &&
            world.hasComponent<TransformComponent>(entityId)) {
            auto& transform = world.getComponent<TransformComponent>(entityId);
            json transformJson = json::parse(prefabEntity.transformData.value());

            if (!respectOverrides || !instance.hasOverride("Transform.position")) {
                if (transformJson.contains("position")) {
                    transform.position = deserializeVec3(transformJson["position"]);
                }
            }
            if (!respectOverrides || !instance.hasOverride("Transform.rotation")) {
                if (transformJson.contains("rotation")) {
                    transform.rotation = deserializeVec3(transformJson["rotation"]);
                }
            }
            if (!respectOverrides || !instance.hasOverride("Transform.scale")) {
                if (transformJson.contains("scale")) {
                    transform.scale = deserializeVec3(transformJson["scale"]);
                }
            }
        }

        // Update sprite (if not overridden)
        if (prefabEntity.spriteRendererData.has_value() &&
            world.hasComponent<SpriteRendererComponent>(entityId)) {
            auto& sprite = world.getComponent<SpriteRendererComponent>(entityId);
            json spriteJson = json::parse(prefabEntity.spriteRendererData.value());

            if (!respectOverrides || !instance.hasOverride("SpriteRenderer.color")) {
                if (spriteJson.contains("color")) {
                    sprite.color = deserializeVec4(spriteJson["color"]);
                }
            }
        }
    }
}

void Prefab::applyInstanceChanges(World& world, World::EntityId instanceRoot) {
    if (!world.isValid(instanceRoot)) {
        return;
    }

    // Rebuild the prefab from the instance
    *this = createFromEntity(world, instanceRoot);
}

void Prefab::revertInstance(World& world, World::EntityId instanceRoot) const {
    if (!world.isValid(instanceRoot)) {
        return;
    }

    // Clear overrides and update from prefab
    auto* instance = world.tryGetComponent<PrefabInstanceComponent>(instanceRoot);
    if (instance != nullptr) {
        instance->clearAllOverrides();
    }

    // Recursively revert all entities in the instance
    std::function<void(World::EntityId)> revertEntity = [&](World::EntityId entityId) {
        auto* inst = world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (inst != nullptr && inst->prefabId == m_prefabId) {
            inst->clearAllOverrides();
        }

        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            revertEntity(childId);
            return true;
        });
    };

    revertEntity(instanceRoot);
    updateInstances(world, false);
}

bool Prefab::saveToFile(const std::filesystem::path& path) {
    try {
        String jsonStr = serialize();

        std::ofstream file(path);
        if (!file.is_open()) {
            spdlog::error("Prefab::saveToFile: Failed to open file: {}", path.string());
            return false;
        }

        file << jsonStr;
        file.close();

        spdlog::info("Prefab saved to: {}", path.string());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Prefab::saveToFile: {}", e.what());
        return false;
    }
}

bool Prefab::loadFromFile(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("Prefab::loadFromFile: Failed to open file: {}", path.string());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        if (deserialize(buffer.str())) {
            spdlog::info("Prefab loaded from: {}", path.string());
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Prefab::loadFromFile: {}", e.what());
        return false;
    }
}

String Prefab::serialize() const {
    json root;
    root["version"] = 1;
    root["type"] = "Prefab";
    root["name"] = m_name;
    root["id"] = m_prefabId.toString();

    json entitiesArray = json::array();
    for (const auto& entity : m_entities) {
        json entityJson;
        entityJson["name"] = entity.name;
        entityJson["parentIndex"] = entity.parentIndex;

        if (entity.transformData.has_value()) {
            entityJson["transform"] = json::parse(entity.transformData.value());
        }
        if (entity.spriteRendererData.has_value()) {
            entityJson["spriteRenderer"] = json::parse(entity.spriteRendererData.value());
        }
        if (entity.cameraData.has_value()) {
            entityJson["camera"] = json::parse(entity.cameraData.value());
        }
        if (entity.hasStaticComponent) {
            entityJson["static"] = true;
        }
        if (entity.hasActiveComponent) {
            entityJson["active"] = true;
        }

        entitiesArray.push_back(entityJson);
    }

    root["entities"] = entitiesArray;

    return root.dump(2);
}

bool Prefab::deserialize(const String& jsonStr) {
    try {
        json root = json::parse(jsonStr);

        if (!root.contains("type") || root["type"] != "Prefab") {
            spdlog::error("Prefab::deserialize: Invalid prefab file");
            return false;
        }

        if (root.contains("name")) {
            m_name = root["name"].get<String>();
        }
        if (root.contains("id")) {
            m_prefabId = UUID::fromString(root["id"].get<String>());
        }

        m_entities.clear();

        if (root.contains("entities") && root["entities"].is_array()) {
            for (const auto& entityJson : root["entities"]) {
                PrefabEntity entity;

                if (entityJson.contains("name")) {
                    entity.name = entityJson["name"].get<String>();
                }
                if (entityJson.contains("parentIndex")) {
                    entity.parentIndex = entityJson["parentIndex"].get<i32>();
                }
                if (entityJson.contains("transform")) {
                    entity.transformData = entityJson["transform"].dump();
                }
                if (entityJson.contains("spriteRenderer")) {
                    entity.spriteRendererData = entityJson["spriteRenderer"].dump();
                }
                if (entityJson.contains("camera")) {
                    entity.cameraData = entityJson["camera"].dump();
                }
                if (entityJson.contains("static")) {
                    entity.hasStaticComponent = entityJson["static"].get<bool>();
                }
                if (entityJson.contains("active")) {
                    entity.hasActiveComponent = entityJson["active"].get<bool>();
                }

                m_entities.push_back(entity);
            }
        }

        return true;
    } catch (const json::exception& e) {
        spdlog::error("Prefab::deserialize: JSON error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Prefab::deserialize: {}", e.what());
        return false;
    }
}

}  // namespace limbo
