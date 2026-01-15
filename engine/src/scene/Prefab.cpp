#include "limbo/scene/Prefab.hpp"

#include "limbo/ecs/Components.hpp"
#include "limbo/ecs/Hierarchy.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/scripting/ScriptComponent.hpp"
#include "limbo/debug/Log.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace limbo {

namespace {

// ============================================================================
// JSON Serialization Helpers
// ============================================================================

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

// ============================================================================
// Component Serializers
// ============================================================================

json serializeTransform(const TransformComponent& transform) {
    json j;
    j["position"] = serializeVec3(transform.position);
    j["rotation"] = serializeVec3(transform.rotation);
    j["scale"] = serializeVec3(transform.scale);
    return j;
}

void deserializeTransform(TransformComponent& transform, const json& j) {
    if (j.contains("position")) {
        transform.position = deserializeVec3(j["position"]);
    }
    if (j.contains("rotation")) {
        transform.rotation = deserializeVec3(j["rotation"]);
    }
    if (j.contains("scale")) {
        transform.scale = deserializeVec3(j["scale"]);
    }
}

json serializeSpriteRenderer(const SpriteRendererComponent& sprite) {
    json j;
    j["color"] = serializeVec4(sprite.color);
    j["sortingLayer"] = sprite.sortingLayer;
    j["sortingOrder"] = sprite.sortingOrder;
    if (sprite.textureId.isValid()) {
        j["textureId"] = sprite.textureId.uuid().toString();
    }
    j["uvMin"] = serializeVec2(sprite.uvMin);
    j["uvMax"] = serializeVec2(sprite.uvMax);
    return j;
}

void deserializeSpriteRenderer(SpriteRendererComponent& sprite, const json& j) {
    if (j.contains("color")) {
        sprite.color = deserializeVec4(j["color"]);
    }
    if (j.contains("sortingLayer")) {
        sprite.sortingLayer = j["sortingLayer"].get<i32>();
    }
    if (j.contains("sortingOrder")) {
        sprite.sortingOrder = j["sortingOrder"].get<i32>();
    }
    if (j.contains("textureId")) {
        sprite.textureId = AssetId(UUID::fromString(j["textureId"].get<String>()));
    }
    if (j.contains("uvMin")) {
        sprite.uvMin = deserializeVec2(j["uvMin"]);
    }
    if (j.contains("uvMax")) {
        sprite.uvMax = deserializeVec2(j["uvMax"]);
    }
}

json serializeQuadRenderer(const QuadRendererComponent& quad) {
    json j;
    j["color"] = serializeVec4(quad.color);
    j["size"] = serializeVec2(quad.size);
    j["sortingLayer"] = quad.sortingLayer;
    j["sortingOrder"] = quad.sortingOrder;
    return j;
}

void deserializeQuadRenderer(QuadRendererComponent& quad, const json& j) {
    if (j.contains("color")) {
        quad.color = deserializeVec4(j["color"]);
    }
    if (j.contains("size")) {
        quad.size = deserializeVec2(j["size"]);
    }
    if (j.contains("sortingLayer")) {
        quad.sortingLayer = j["sortingLayer"].get<i32>();
    }
    if (j.contains("sortingOrder")) {
        quad.sortingOrder = j["sortingOrder"].get<i32>();
    }
}

json serializeCircleRenderer(const CircleRendererComponent& circle) {
    json j;
    j["color"] = serializeVec4(circle.color);
    j["radius"] = circle.radius;
    j["segments"] = circle.segments;
    j["sortingLayer"] = circle.sortingLayer;
    j["sortingOrder"] = circle.sortingOrder;
    return j;
}

void deserializeCircleRenderer(CircleRendererComponent& circle, const json& j) {
    if (j.contains("color")) {
        circle.color = deserializeVec4(j["color"]);
    }
    if (j.contains("radius")) {
        circle.radius = j["radius"].get<f32>();
    }
    if (j.contains("segments")) {
        circle.segments = j["segments"].get<i32>();
    }
    if (j.contains("sortingLayer")) {
        circle.sortingLayer = j["sortingLayer"].get<i32>();
    }
    if (j.contains("sortingOrder")) {
        circle.sortingOrder = j["sortingOrder"].get<i32>();
    }
}

json serializeCamera(const CameraComponent& camera) {
    json j;
    j["projectionType"] = camera.projectionType == CameraComponent::ProjectionType::Perspective
                              ? "perspective"
                              : "orthographic";
    j["fov"] = camera.fov;
    j["orthoSize"] = camera.orthoSize;
    j["nearClip"] = camera.nearClip;
    j["farClip"] = camera.farClip;
    j["primary"] = camera.primary;
    return j;
}

void deserializeCamera(CameraComponent& camera, const json& j) {
    if (j.contains("projectionType")) {
        String projType = j["projectionType"].get<String>();
        camera.projectionType = (projType == "orthographic")
                                    ? CameraComponent::ProjectionType::Orthographic
                                    : CameraComponent::ProjectionType::Perspective;
    }
    if (j.contains("fov")) {
        camera.fov = j["fov"].get<f32>();
    }
    if (j.contains("orthoSize")) {
        camera.orthoSize = j["orthoSize"].get<f32>();
    }
    if (j.contains("nearClip")) {
        camera.nearClip = j["nearClip"].get<f32>();
    }
    if (j.contains("farClip")) {
        camera.farClip = j["farClip"].get<f32>();
    }
    if (j.contains("primary")) {
        camera.primary = j["primary"].get<bool>();
    }
}

json serializeRigidbody2D(const Rigidbody2DComponent& rb) {
    json j;
    j["type"] = static_cast<int>(rb.type);
    j["gravityScale"] = rb.gravityScale;
    j["fixedRotation"] = rb.fixedRotation;
    j["linearVelocity"] = serializeVec2(rb.linearVelocity);
    j["angularVelocity"] = rb.angularVelocity;
    j["linearDamping"] = rb.linearDamping;
    j["angularDamping"] = rb.angularDamping;
    return j;
}

void deserializeRigidbody2D(Rigidbody2DComponent& rb, const json& j) {
    if (j.contains("type")) {
        rb.type = static_cast<BodyType>(j["type"].get<int>());
    }
    if (j.contains("gravityScale")) {
        rb.gravityScale = j["gravityScale"].get<f32>();
    }
    if (j.contains("fixedRotation")) {
        rb.fixedRotation = j["fixedRotation"].get<bool>();
    }
    if (j.contains("linearVelocity")) {
        rb.linearVelocity = deserializeVec2(j["linearVelocity"]);
    }
    if (j.contains("angularVelocity")) {
        rb.angularVelocity = j["angularVelocity"].get<f32>();
    }
    if (j.contains("linearDamping")) {
        rb.linearDamping = j["linearDamping"].get<f32>();
    }
    if (j.contains("angularDamping")) {
        rb.angularDamping = j["angularDamping"].get<f32>();
    }
}

json serializeBoxCollider2D(const BoxCollider2DComponent& box) {
    json j;
    j["size"] = serializeVec2(box.size);
    j["offset"] = serializeVec2(box.offset);
    j["density"] = box.density;
    j["friction"] = box.friction;
    j["restitution"] = box.restitution;
    j["restitutionThreshold"] = box.restitutionThreshold;
    j["isTrigger"] = box.isTrigger;
    return j;
}

void deserializeBoxCollider2D(BoxCollider2DComponent& box, const json& j) {
    if (j.contains("size")) {
        box.size = deserializeVec2(j["size"]);
    }
    if (j.contains("offset")) {
        box.offset = deserializeVec2(j["offset"]);
    }
    if (j.contains("density")) {
        box.density = j["density"].get<f32>();
    }
    if (j.contains("friction")) {
        box.friction = j["friction"].get<f32>();
    }
    if (j.contains("restitution")) {
        box.restitution = j["restitution"].get<f32>();
    }
    if (j.contains("restitutionThreshold")) {
        box.restitutionThreshold = j["restitutionThreshold"].get<f32>();
    }
    if (j.contains("isTrigger")) {
        box.isTrigger = j["isTrigger"].get<bool>();
    }
}

json serializeCircleCollider2D(const CircleCollider2DComponent& circle) {
    json j;
    j["radius"] = circle.radius;
    j["offset"] = serializeVec2(circle.offset);
    j["density"] = circle.density;
    j["friction"] = circle.friction;
    j["restitution"] = circle.restitution;
    j["restitutionThreshold"] = circle.restitutionThreshold;
    j["isTrigger"] = circle.isTrigger;
    return j;
}

void deserializeCircleCollider2D(CircleCollider2DComponent& circle, const json& j) {
    if (j.contains("radius")) {
        circle.radius = j["radius"].get<f32>();
    }
    if (j.contains("offset")) {
        circle.offset = deserializeVec2(j["offset"]);
    }
    if (j.contains("density")) {
        circle.density = j["density"].get<f32>();
    }
    if (j.contains("friction")) {
        circle.friction = j["friction"].get<f32>();
    }
    if (j.contains("restitution")) {
        circle.restitution = j["restitution"].get<f32>();
    }
    if (j.contains("restitutionThreshold")) {
        circle.restitutionThreshold = j["restitutionThreshold"].get<f32>();
    }
    if (j.contains("isTrigger")) {
        circle.isTrigger = j["isTrigger"].get<bool>();
    }
}

json serializeScript(const ScriptComponent& script) {
    json j;
    j["scriptPath"] = script.scriptPath;
    j["enabled"] = script.enabled;
    return j;
}

void deserializeScript(ScriptComponent& script, const json& j) {
    if (j.contains("scriptPath")) {
        script.scriptPath = j["scriptPath"].get<String>();
    }
    if (j.contains("enabled")) {
        script.enabled = j["enabled"].get<bool>();
    }
}

}  // namespace

// ============================================================================
// Prefab Implementation
// ============================================================================

String Prefab::generateLocalId(const String& baseName) {
    if (m_localIdCounter == 0) {
        m_localIdCounter++;
        return "root";
    }
    return baseName + "_" + std::to_string(m_localIdCounter++);
}

const PrefabEntity* Prefab::findEntity(const String& localId) const {
    for (const auto& entity : m_entities) {
        if (entity.localId == localId) {
            return &entity;
        }
    }
    return nullptr;
}

PrefabEntity* Prefab::findEntity(const String& localId) {
    for (auto& entity : m_entities) {
        if (entity.localId == localId) {
            return &entity;
        }
    }
    return nullptr;
}

Prefab Prefab::createFromEntity(World& world, World::EntityId rootEntity) {
    Prefab prefab;

    if (!world.isValid(rootEntity)) {
        LIMBO_LOG_CORE_ERROR("Prefab::createFromEntity: Invalid root entity");
        return prefab;
    }

    // Get name from entity
    if (world.hasComponent<NameComponent>(rootEntity)) {
        prefab.m_name = world.getComponent<NameComponent>(rootEntity).name;
    }

    // Reset counter for generating local IDs
    prefab.m_localIdCounter = 0;

    // Map from entity ID to local ID
    std::unordered_map<World::EntityId, String> entityToLocalId;

    // Recursive function to collect entities
    std::function<void(World::EntityId, const String&)> collectEntities =
        [&](World::EntityId entityId, const String& parentLocalId) {
            // Generate local ID
            String baseName = "entity";
            if (world.hasComponent<NameComponent>(entityId)) {
                baseName = world.getComponent<NameComponent>(entityId).name;
                // Sanitize name for use as ID
                std::replace(baseName.begin(), baseName.end(), ' ', '_');
            }
            String localId = prefab.generateLocalId(baseName);
            entityToLocalId[entityId] = localId;

            // Create prefab entity
            PrefabEntity prefabEntity;
            prefabEntity.localId = localId;
            prefabEntity.parentLocalId = parentLocalId;

            if (world.hasComponent<NameComponent>(entityId)) {
                prefabEntity.name = world.getComponent<NameComponent>(entityId).name;
            } else {
                prefabEntity.name = "Entity";
            }

            // Serialize all components
            serializeEntityComponents(world, entityId, prefabEntity);

            prefab.m_entities.push_back(std::move(prefabEntity));

            // Process children
            Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
                collectEntities(childId, localId);
                return true;
            });
        };

    collectEntities(rootEntity, "");
    prefab.m_rootLocalId = "root";

    return prefab;
}

void Prefab::serializeEntityComponents(World& world, World::EntityId entityId,
                                       PrefabEntity& prefabEntity) {
    // Transform
    if (world.hasComponent<TransformComponent>(entityId)) {
        prefabEntity.components["Transform"] =
            serializeTransform(world.getComponent<TransformComponent>(entityId));
    }

    // Renderers
    if (world.hasComponent<SpriteRendererComponent>(entityId)) {
        prefabEntity.components["SpriteRenderer"] =
            serializeSpriteRenderer(world.getComponent<SpriteRendererComponent>(entityId));
    }
    if (world.hasComponent<QuadRendererComponent>(entityId)) {
        prefabEntity.components["QuadRenderer"] =
            serializeQuadRenderer(world.getComponent<QuadRendererComponent>(entityId));
    }
    if (world.hasComponent<CircleRendererComponent>(entityId)) {
        prefabEntity.components["CircleRenderer"] =
            serializeCircleRenderer(world.getComponent<CircleRendererComponent>(entityId));
    }

    // Camera
    if (world.hasComponent<CameraComponent>(entityId)) {
        prefabEntity.components["Camera"] =
            serializeCamera(world.getComponent<CameraComponent>(entityId));
    }

    // Physics
    if (world.hasComponent<Rigidbody2DComponent>(entityId)) {
        prefabEntity.components["Rigidbody2D"] =
            serializeRigidbody2D(world.getComponent<Rigidbody2DComponent>(entityId));
    }
    if (world.hasComponent<BoxCollider2DComponent>(entityId)) {
        prefabEntity.components["BoxCollider2D"] =
            serializeBoxCollider2D(world.getComponent<BoxCollider2DComponent>(entityId));
    }
    if (world.hasComponent<CircleCollider2DComponent>(entityId)) {
        prefabEntity.components["CircleCollider2D"] =
            serializeCircleCollider2D(world.getComponent<CircleCollider2DComponent>(entityId));
    }

    // Scripting
    if (world.hasComponent<ScriptComponent>(entityId)) {
        prefabEntity.components["Script"] =
            serializeScript(world.getComponent<ScriptComponent>(entityId));
    }

    // Tag components (stored as empty objects to indicate presence)
    if (world.hasComponent<StaticComponent>(entityId)) {
        prefabEntity.components["Static"] = json::object();
    }
    if (world.hasComponent<ActiveComponent>(entityId)) {
        prefabEntity.components["Active"] = json::object();
    }
}

Entity Prefab::instantiate(World& world, const glm::vec3& position) const {
    if (m_entities.empty()) {
        LIMBO_LOG_CORE_ERROR("Prefab::instantiate: Prefab has no entities");
        return Entity();
    }

    // Map from local ID to created entity ID
    std::unordered_map<String, World::EntityId> localIdToEntity;

    // First pass: create all entities
    for (const auto& prefabEntity : m_entities) {
        Entity entity = world.createEntity(prefabEntity.name);
        localIdToEntity[prefabEntity.localId] = entity.id();

        // Add PrefabInstanceComponent
        world.addComponent<PrefabInstanceComponent>(entity.id(), m_prefabId, prefabEntity.localId,
                                                    prefabEntity.isRoot());

        // Deserialize components
        deserializeEntityComponents(world, entity.id(), prefabEntity);
    }

    // Second pass: set up hierarchy
    for (const auto& prefabEntity : m_entities) {
        if (!prefabEntity.parentLocalId.empty()) {
            auto parentIt = localIdToEntity.find(prefabEntity.parentLocalId);
            auto childIt = localIdToEntity.find(prefabEntity.localId);
            if (parentIt != localIdToEntity.end() && childIt != localIdToEntity.end()) {
                Hierarchy::setParent(world, childIt->second, parentIt->second);
            }
        }
    }

    // Apply position offset to root
    auto rootIt = localIdToEntity.find(m_rootLocalId);
    if (rootIt != localIdToEntity.end() && world.hasComponent<TransformComponent>(rootIt->second)) {
        auto& transform = world.getComponent<TransformComponent>(rootIt->second);
        transform.position += position;
    }

    return Entity(rootIt != localIdToEntity.end() ? rootIt->second : World::kNullEntity, &world);
}

void Prefab::deserializeEntityComponents(World& world, World::EntityId entityId,
                                         const PrefabEntity& prefabEntity) const {
    for (const auto& [typeName, data] : prefabEntity.components) {
        if (typeName == "Transform") {
            auto& transform = world.hasComponent<TransformComponent>(entityId)
                                  ? world.getComponent<TransformComponent>(entityId)
                                  : world.addComponent<TransformComponent>(entityId);
            deserializeTransform(transform, data);
        } else if (typeName == "SpriteRenderer") {
            auto& sprite = world.addComponent<SpriteRendererComponent>(entityId);
            deserializeSpriteRenderer(sprite, data);
        } else if (typeName == "QuadRenderer") {
            auto& quad = world.addComponent<QuadRendererComponent>(entityId);
            deserializeQuadRenderer(quad, data);
        } else if (typeName == "CircleRenderer") {
            auto& circle = world.addComponent<CircleRendererComponent>(entityId);
            deserializeCircleRenderer(circle, data);
        } else if (typeName == "Camera") {
            auto& camera = world.addComponent<CameraComponent>(entityId);
            deserializeCamera(camera, data);
        } else if (typeName == "Rigidbody2D") {
            auto& rb = world.addComponent<Rigidbody2DComponent>(entityId);
            deserializeRigidbody2D(rb, data);
        } else if (typeName == "BoxCollider2D") {
            auto& box = world.addComponent<BoxCollider2DComponent>(entityId);
            deserializeBoxCollider2D(box, data);
        } else if (typeName == "CircleCollider2D") {
            auto& circle = world.addComponent<CircleCollider2DComponent>(entityId);
            deserializeCircleCollider2D(circle, data);
        } else if (typeName == "Script") {
            auto& script = world.addComponent<ScriptComponent>(entityId);
            deserializeScript(script, data);
        } else if (typeName == "Static") {
            world.addComponent<StaticComponent>(entityId);
        } else if (typeName == "Active") {
            world.addComponent<ActiveComponent>(entityId);
        }
    }
}

void Prefab::updateInstances(World& world, bool respectOverrides) const {
    // Find all instances of this prefab
    auto view = world.view<PrefabInstanceComponent>();

    for (auto entityId : view) {
        auto& instance = view.get<PrefabInstanceComponent>(entityId);

        // Only update if this is from our prefab
        if (instance.prefabId != m_prefabId) {
            continue;
        }

        // Find the corresponding prefab entity
        const PrefabEntity* prefabEntity = findEntity(instance.localId);
        if (prefabEntity == nullptr) {
            continue;
        }

        // Update each component
        for (const auto& [typeName, data] : prefabEntity->components) {
            // For now, simplified update - just update Transform as example
            // Full implementation would check each property against overrides
            if (typeName == "Transform" && world.hasComponent<TransformComponent>(entityId)) {
                auto& transform = world.getComponent<TransformComponent>(entityId);

                if (!respectOverrides || !instance.hasOverride("Transform", "position")) {
                    if (data.contains("position")) {
                        transform.position = deserializeVec3(data["position"]);
                    }
                }
                if (!respectOverrides || !instance.hasOverride("Transform", "rotation")) {
                    if (data.contains("rotation")) {
                        transform.rotation = deserializeVec3(data["rotation"]);
                    }
                }
                if (!respectOverrides || !instance.hasOverride("Transform", "scale")) {
                    if (data.contains("scale")) {
                        transform.scale = deserializeVec3(data["scale"]);
                    }
                }
            }
            // Add more component types as needed...
        }
    }
}

bool Prefab::applyInstanceChanges(World& world, World::EntityId instanceRoot) {
    if (!world.isValid(instanceRoot)) {
        LIMBO_LOG_CORE_ERROR("Prefab::applyInstanceChanges: Invalid instance root");
        return false;
    }

    // Collect all overrides from the instance hierarchy
    std::vector<PrefabOverride> allOverrides;

    std::function<void(World::EntityId)> collectOverrides = [&](World::EntityId entityId) {
        auto* instance = world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (instance != nullptr && instance->prefabId == m_prefabId) {
            for (const auto& override : instance->overrides) {
                allOverrides.push_back(override);
            }
        }

        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            collectOverrides(childId);
            return true;
        });
    };

    collectOverrides(instanceRoot);

    if (allOverrides.empty()) {
        LIMBO_LOG_CORE_INFO("Prefab::applyInstanceChanges: No overrides to apply");
        return true;
    }

    // Apply each override to the prefab
    usize appliedCount = 0;
    for (const auto& override : allOverrides) {
        if (applyOverride(override)) {
            appliedCount++;
        }
    }

    // Clear the applied overrides from the instance
    std::function<void(World::EntityId)> clearAppliedOverrides = [&](World::EntityId entityId) {
        auto* instance = world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (instance != nullptr && instance->prefabId == m_prefabId) {
            instance->clearAllOverrides();
        }

        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            clearAppliedOverrides(childId);
            return true;
        });
    };

    clearAppliedOverrides(instanceRoot);

    LIMBO_LOG_CORE_INFO("Prefab::applyInstanceChanges: Applied {} overrides", appliedCount);
    return appliedCount > 0;
}

bool Prefab::applyOverride(const PrefabOverride& override) {
    if (override.kind != PrefabOverride::Kind::Property) {
        // TODO: Handle AddComponent/RemoveComponent in v1.5
        LIMBO_LOG_CORE_WARN("Prefab::applyOverride: Only Property overrides supported currently");
        return false;
    }

    // Find the target entity in the prefab
    PrefabEntity* targetEntity = findEntity(override.targetLocalId);
    if (targetEntity == nullptr) {
        LIMBO_LOG_CORE_WARN("Prefab::applyOverride: Entity '{}' not found in prefab",
                            override.targetLocalId);
        return false;
    }

    // Find or create the component data
    auto compIt = targetEntity->components.find(override.component);
    if (compIt == targetEntity->components.end()) {
        // Component doesn't exist - create it with just this property
        targetEntity->components[override.component] = json::object();
        compIt = targetEntity->components.find(override.component);
    }

    // Apply the property value
    // Handle nested properties (e.g., "position" for vec3, or "color" for vec4)
    compIt->second[override.property] = override.value;

    LIMBO_LOG_CORE_DEBUG("Prefab::applyOverride: Applied {}.{} = {}", override.component,
                         override.property, override.value.dump());
    return true;
}

void Prefab::revertInstance(World& world, World::EntityId instanceRoot) const {
    if (!world.isValid(instanceRoot)) {
        return;
    }

    // Clear overrides on all entities in the instance
    std::function<void(World::EntityId)> clearOverrides = [&](World::EntityId entityId) {
        auto* instance = world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (instance != nullptr && instance->prefabId == m_prefabId) {
            instance->clearAllOverrides();
        }

        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            clearOverrides(childId);
            return true;
        });
    };

    clearOverrides(instanceRoot);
    updateInstances(world, false);
}

void Prefab::unpack(World& world, World::EntityId instanceRoot, bool completely) {
    if (!world.isValid(instanceRoot)) {
        LIMBO_LOG_CORE_ERROR("Prefab::unpack: Invalid instance root");
        return;
    }

    // Collect all entities that are part of this prefab instance
    std::vector<World::EntityId> entitiesToUnpack;

    // Get the prefab ID from the root to identify which entities belong to this instance
    UUID instancePrefabId;
    UUID instanceInstanceId;
    if (world.hasComponent<PrefabInstanceComponent>(instanceRoot)) {
        auto& rootInst = world.getComponent<PrefabInstanceComponent>(instanceRoot);
        instancePrefabId = rootInst.prefabId;
        instanceInstanceId = rootInst.instanceId;
    } else {
        LIMBO_LOG_CORE_WARN("Prefab::unpack: Root entity is not a prefab instance");
        return;
    }

    std::function<void(World::EntityId)> collectEntities = [&](World::EntityId entityId) {
        auto* instance = world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (instance != nullptr && instance->prefabId == instancePrefabId &&
            instance->instanceId == instanceInstanceId) {
            entitiesToUnpack.push_back(entityId);
        }

        // Recurse to children
        Hierarchy::forEachChild(world, entityId, [&](World::EntityId childId) {
            // Check if child is part of the same instance or a nested prefab
            auto* childInstance = world.tryGetComponent<PrefabInstanceComponent>(childId);
            if (childInstance != nullptr) {
                if (childInstance->prefabId == instancePrefabId &&
                    childInstance->instanceId == instanceInstanceId) {
                    // Same instance - include it
                    collectEntities(childId);
                } else if (completely) {
                    // Different prefab instance - unpack recursively if completely=true
                    unpack(world, childId, true);
                }
                // If not completely, leave nested prefab instances alone
            } else {
                // Regular entity child (shouldn't happen for proper prefab instances)
                collectEntities(childId);
            }
            return true;
        });
    };

    collectEntities(instanceRoot);

    // Remove PrefabInstanceComponent from all collected entities
    // This keeps all current component values but removes the prefab link
    for (World::EntityId entityId : entitiesToUnpack) {
        if (world.hasComponent<PrefabInstanceComponent>(entityId)) {
            world.removeComponent<PrefabInstanceComponent>(entityId);
        }
    }

    LIMBO_LOG_CORE_INFO("Prefab::unpack: Unpacked {} entities from prefab instance",
                        entitiesToUnpack.size());
}

bool Prefab::saveToFile(const std::filesystem::path& path) {
    try {
        String jsonStr = serialize();

        std::ofstream file(path);
        if (!file.is_open()) {
            LIMBO_LOG_CORE_ERROR("Prefab::saveToFile: Failed to open file: {}", path.string());
            return false;
        }

        file << jsonStr;
        file.close();

        LIMBO_LOG_CORE_INFO("Prefab saved to: {}", path.string());
        return true;
    } catch (const std::exception& e) {
        LIMBO_LOG_CORE_ERROR("Prefab::saveToFile: {}", e.what());
        return false;
    }
}

bool Prefab::loadFromFile(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LIMBO_LOG_CORE_ERROR("Prefab::loadFromFile: Failed to open file: {}", path.string());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        if (deserialize(buffer.str())) {
            LIMBO_LOG_CORE_INFO("Prefab loaded from: {}", path.string());
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        LIMBO_LOG_CORE_ERROR("Prefab::loadFromFile: {}", e.what());
        return false;
    }
}

String Prefab::serialize() const {
    json root;
    root["type"] = "PrefabAsset";
    root["version"] = 1;
    root["prefab_id"] = m_prefabId.toString();
    root["name"] = m_name;
    root["root_local_id"] = m_rootLocalId;

    json entitiesArray = json::array();
    for (const auto& entity : m_entities) {
        json entityJson;
        entityJson["local_id"] = entity.localId;
        entityJson["name"] = entity.name;
        entityJson["parent_local_id"] =
            entity.parentLocalId.empty() ? nullptr : json(entity.parentLocalId);
        entityJson["components"] = entity.components;
        entitiesArray.push_back(entityJson);
    }

    root["entities"] = entitiesArray;

    return root.dump(2);
}

bool Prefab::deserialize(const String& jsonStr) {
    try {
        json root = json::parse(jsonStr);

        // Support both old and new format
        String type = root.value("type", "Prefab");
        if (type != "PrefabAsset" && type != "Prefab") {
            LIMBO_LOG_CORE_ERROR("Prefab::deserialize: Invalid prefab file type: {}", type);
            return false;
        }

        if (root.contains("prefab_id")) {
            m_prefabId = UUID::fromString(root["prefab_id"].get<String>());
        } else if (root.contains("id")) {
            // Legacy format
            m_prefabId = UUID::fromString(root["id"].get<String>());
        }

        if (root.contains("name")) {
            m_name = root["name"].get<String>();
        }

        if (root.contains("root_local_id")) {
            m_rootLocalId = root["root_local_id"].get<String>();
        } else {
            m_rootLocalId = "root";
        }

        m_entities.clear();

        if (root.contains("entities") && root["entities"].is_array()) {
            for (const auto& entityJson : root["entities"]) {
                PrefabEntity entity;

                if (entityJson.contains("local_id")) {
                    entity.localId = entityJson["local_id"].get<String>();
                }
                if (entityJson.contains("name")) {
                    entity.name = entityJson["name"].get<String>();
                }
                if (entityJson.contains("parent_local_id") &&
                    !entityJson["parent_local_id"].is_null()) {
                    entity.parentLocalId = entityJson["parent_local_id"].get<String>();
                }

                // New format: generic components map
                if (entityJson.contains("components") && entityJson["components"].is_object()) {
                    entity.components =
                        entityJson["components"].get<std::unordered_map<String, json>>();
                } else {
                    // Legacy format: individual component fields
                    if (entityJson.contains("transform")) {
                        entity.components["Transform"] = entityJson["transform"];
                    }
                    if (entityJson.contains("spriteRenderer")) {
                        entity.components["SpriteRenderer"] = entityJson["spriteRenderer"];
                    }
                    if (entityJson.contains("camera")) {
                        entity.components["Camera"] = entityJson["camera"];
                    }
                    if (entityJson.contains("static") && entityJson["static"].get<bool>()) {
                        entity.components["Static"] = json::object();
                    }
                    if (entityJson.contains("active") && entityJson["active"].get<bool>()) {
                        entity.components["Active"] = json::object();
                    }
                }

                m_entities.push_back(std::move(entity));
            }
        }

        return true;
    } catch (const json::exception& e) {
        LIMBO_LOG_CORE_ERROR("Prefab::deserialize: JSON error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LIMBO_LOG_CORE_ERROR("Prefab::deserialize: {}", e.what());
        return false;
    }
}

}  // namespace limbo
