#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/core/UUID.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace limbo {

/**
 * PrefabEntity - Serialized entity data within a prefab
 *
 * Stores all component data for a single entity in the prefab hierarchy.
 * Uses stable string IDs instead of indices for robust references.
 */
struct PrefabEntity {
    String localId;        // Stable ID within this prefab (e.g., "root", "child_1")
    String name;           // Display name
    String parentLocalId;  // Parent's localId, empty string if root

    // Generic component storage: component type name -> JSON data
    std::unordered_map<String, nlohmann::json> components;

    [[nodiscard]] bool isRoot() const { return parentLocalId.empty(); }

    [[nodiscard]] bool hasComponent(const String& typeName) const {
        return components.find(typeName) != components.end();
    }
};

/**
 * PrefabOverride - A single property override on a prefab instance
 *
 * Stores the actual overridden value, not just a flag.
 */
struct PrefabOverride {
    enum class Kind { Property, AddComponent, RemoveComponent };

    Kind kind = Kind::Property;
    String targetLocalId;  // Which entity in the prefab
    String component;      // Component type name
    String property;       // Property path (e.g., "position", "color.r")
    nlohmann::json value;  // The overridden value (for Property kind)

    PrefabOverride() = default;
    PrefabOverride(Kind k, const String& target, const String& comp, const String& prop,
                   const nlohmann::json& val)
        : kind(k), targetLocalId(target), component(comp), property(prop), value(val) {}

    // Convenience factory for property overrides
    static PrefabOverride makeProperty(const String& target, const String& comp, const String& prop,
                                       const nlohmann::json& val) {
        return PrefabOverride(Kind::Property, target, comp, prop, val);
    }
};

/**
 * PrefabInstanceComponent - Marks an entity as an instance of a prefab
 *
 * Tracks which prefab this entity came from and stores actual override values.
 */
struct PrefabInstanceComponent {
    UUID prefabId;       // UUID of the source prefab asset
    UUID instanceId;     // Unique ID for this instance
    String localId;      // This entity's localId within the prefab
    bool isRoot = true;  // Is this the root entity of the prefab instance?

    // Override storage - actual values, not just flags
    std::vector<PrefabOverride> overrides;

    PrefabInstanceComponent() : instanceId(UUID::generate()) {}

    explicit PrefabInstanceComponent(const UUID& prefab, const String& local, bool root = true)
        : prefabId(prefab), instanceId(UUID::generate()), localId(local), isRoot(root) {}

    [[nodiscard]] bool hasOverride(const String& component, const String& property) const {
        for (const auto& ov : overrides) {
            if (ov.kind == PrefabOverride::Kind::Property && ov.targetLocalId == localId &&
                ov.component == component && ov.property == property) {
                return true;
            }
        }
        return false;
    }

    void setOverride(const String& component, const String& property, const nlohmann::json& value) {
        // Update existing or add new
        for (auto& ov : overrides) {
            if (ov.kind == PrefabOverride::Kind::Property && ov.targetLocalId == localId &&
                ov.component == component && ov.property == property) {
                ov.value = value;
                return;
            }
        }
        overrides.push_back(PrefabOverride::makeProperty(localId, component, property, value));
    }

    void clearOverride(const String& component, const String& property) {
        overrides.erase(std::remove_if(overrides.begin(), overrides.end(),
                                       [&](const PrefabOverride& ov) {
                                           return ov.kind == PrefabOverride::Kind::Property &&
                                                  ov.targetLocalId == localId &&
                                                  ov.component == component &&
                                                  ov.property == property;
                                       }),
                        overrides.end());
    }

    void clearAllOverrides() { overrides.clear(); }

    // Legacy compatibility - check by "Component.property" string
    [[nodiscard]] bool hasOverride(const String& propertyPath) const {
        auto dotPos = propertyPath.find('.');
        if (dotPos == String::npos) {
            return false;
        }
        return hasOverride(propertyPath.substr(0, dotPos), propertyPath.substr(dotPos + 1));
    }
};

/**
 * Prefab - A reusable template for creating entities
 *
 * A prefab stores a hierarchy of entities with their components that can be
 * instantiated multiple times in a scene. Changes to the prefab can propagate
 * to all instances (unless overridden).
 *
 * Data model follows Unity-style prefabs:
 * - Entities identified by stable local_id strings
 * - Components stored generically as JSON
 * - Overrides store actual values, enabling Apply/Revert
 *
 * Usage:
 * 1. Create a prefab from an existing entity: Prefab::createFromEntity()
 * 2. Save the prefab to a file: prefab.saveToFile()
 * 3. Instantiate the prefab: prefab.instantiate()
 */
class LIMBO_API Prefab {
public:
    Prefab() = default;
    ~Prefab() = default;

    // Copyable and movable
    Prefab(const Prefab&) = default;
    Prefab& operator=(const Prefab&) = default;
    Prefab(Prefab&&) = default;
    Prefab& operator=(Prefab&&) = default;

    /**
     * Create a prefab from an existing entity (and its children)
     * @param world The world containing the entity
     * @param rootEntity The root entity to create the prefab from
     * @return The created prefab
     */
    static Prefab createFromEntity(World& world, World::EntityId rootEntity);

    /**
     * Instantiate this prefab in a world
     * @param world The world to create the instance in
     * @param position Optional position offset for the root entity
     * @return The root entity of the instantiated prefab
     */
    Entity instantiate(World& world, const glm::vec3& position = glm::vec3(0.0f)) const;

    /**
     * Update all instances of this prefab in a world
     * @param world The world containing the instances
     * @param respectOverrides If true, don't update overridden properties
     */
    void updateInstances(World& world, bool respectOverrides = true) const;

    /**
     * Apply changes from an instance back to the prefab
     * This merges the instance's current state into the prefab baseline,
     * preserving the structure while updating changed values.
     * @param world The world containing the instance
     * @param instanceRoot The root entity of the instance
     * @return True if changes were applied successfully
     */
    bool applyInstanceChanges(World& world, World::EntityId instanceRoot);

    /**
     * Apply a single override from an instance to the prefab
     * @param override The override to apply
     * @return True if the override was applied
     */
    bool applyOverride(const PrefabOverride& override);

    /**
     * Revert an instance to match the prefab
     * @param world The world containing the instance
     * @param instanceRoot The root entity of the instance
     */
    void revertInstance(World& world, World::EntityId instanceRoot) const;

    /**
     * Unpack a prefab instance - removes the prefab link but keeps current values
     * @param world The world containing the instance
     * @param instanceRoot The root entity of the instance
     * @param completely If true, also unpack any nested prefab instances
     */
    static void unpack(World& world, World::EntityId instanceRoot, bool completely = false);

    // Serialization
    bool saveToFile(const std::filesystem::path& path);
    bool loadFromFile(const std::filesystem::path& path);
    [[nodiscard]] String serialize() const;
    bool deserialize(const String& jsonStr);

    // Accessors
    [[nodiscard]] const String& getName() const { return m_name; }
    void setName(const String& name) { m_name = name; }

    [[nodiscard]] const UUID& getPrefabId() const { return m_prefabId; }
    [[nodiscard]] const String& getRootLocalId() const { return m_rootLocalId; }

    [[nodiscard]] usize getEntityCount() const { return m_entities.size(); }
    [[nodiscard]] const std::vector<PrefabEntity>& getEntities() const { return m_entities; }

    // Find entity by local ID
    [[nodiscard]] const PrefabEntity* findEntity(const String& localId) const;
    [[nodiscard]] PrefabEntity* findEntity(const String& localId);

private:
    // Generate a unique local ID for an entity
    String generateLocalId(const String& baseName);

    // Serialize a single entity's components to JSON
    static void serializeEntityComponents(World& world, World::EntityId entityId,
                                          PrefabEntity& prefabEntity);

    // Deserialize components from a PrefabEntity and add to an entity
    void deserializeEntityComponents(World& world, World::EntityId entityId,
                                     const PrefabEntity& prefabEntity) const;

private:
    String m_name = "Prefab";
    UUID m_prefabId = UUID::generate();
    String m_rootLocalId = "root";
    std::vector<PrefabEntity> m_entities;

    // Counter for generating unique local IDs during creation
    mutable i32 m_localIdCounter = 0;
};

}  // namespace limbo
