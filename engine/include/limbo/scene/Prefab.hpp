#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/core/UUID.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace limbo {

/**
 * PrefabEntity - Serialized entity data within a prefab
 *
 * Stores all component data for a single entity in the prefab hierarchy.
 */
struct PrefabEntity {
    String name;
    i32 parentIndex = -1;  // Index in the entities array, -1 if root

    // Serialized component data (JSON strings)
    std::optional<String> transformData;
    std::optional<String> spriteRendererData;
    std::optional<String> cameraData;
    bool hasStaticComponent = false;
    bool hasActiveComponent = false;
};

/**
 * PrefabInstanceComponent - Marks an entity as an instance of a prefab
 *
 * Tracks which prefab this entity came from and any property overrides.
 */
struct PrefabInstanceComponent {
    UUID prefabId;        // UUID of the source prefab asset
    i32 entityIndex = 0;  // Index of this entity in the prefab's entity list
    bool isRoot = true;   // Is this the root entity of the prefab instance?

    // Override tracking - stores which properties have been modified
    // Key: component name + "." + property name, Value: true if overridden
    std::unordered_map<String, bool> overrides;

    PrefabInstanceComponent() = default;
    explicit PrefabInstanceComponent(const UUID& id, i32 index = 0, bool root = true)
        : prefabId(id), entityIndex(index), isRoot(root) {}

    [[nodiscard]] bool hasOverride(const String& property) const {
        auto it = overrides.find(property);
        return it != overrides.end() && it->second;
    }

    void setOverride(const String& property, bool value = true) { overrides[property] = value; }

    void clearOverride(const String& property) { overrides.erase(property); }

    void clearAllOverrides() { overrides.clear(); }
};

/**
 * Prefab - A reusable template for creating entities
 *
 * A prefab stores a hierarchy of entities with their components that can be
 * instantiated multiple times in a scene. Changes to the prefab can propagate
 * to all instances (unless overridden).
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
     * @param world The world containing the instance
     * @param instanceRoot The root entity of the instance
     */
    void applyInstanceChanges(World& world, World::EntityId instanceRoot);

    /**
     * Revert an instance to match the prefab
     * @param world The world containing the instance
     * @param instanceRoot The root entity of the instance
     */
    void revertInstance(World& world, World::EntityId instanceRoot) const;

    // Serialization
    bool saveToFile(const std::filesystem::path& path);
    bool loadFromFile(const std::filesystem::path& path);
    [[nodiscard]] String serialize() const;
    bool deserialize(const String& jsonStr);

    // Accessors
    [[nodiscard]] const String& getName() const { return m_name; }
    void setName(const String& name) { m_name = name; }

    [[nodiscard]] const UUID& getPrefabId() const { return m_prefabId; }

    [[nodiscard]] usize getEntityCount() const { return m_entities.size(); }
    [[nodiscard]] const std::vector<PrefabEntity>& getEntities() const { return m_entities; }

private:
    // Serialize a single entity to PrefabEntity
    static PrefabEntity serializeEntity(World& world, World::EntityId entityId, i32 parentIndex);

    // Deserialize a PrefabEntity to create a real entity
    World::EntityId deserializeEntity(World& world, const PrefabEntity& prefabEntity,
                                      const std::vector<World::EntityId>& createdEntities) const;

private:
    String m_name = "Prefab";
    UUID m_prefabId = UUID::generate();
    std::vector<PrefabEntity> m_entities;
};

}  // namespace limbo
