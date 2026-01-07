#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/ecs/World.hpp"

#include <filesystem>

namespace limbo {

// Current scene format version
inline constexpr i32 kSceneFormatVersion = 2;

/**
 * SceneSerializer - Saves and loads scenes to/from JSON files
 *
 * Handles serialization of entities and their components to a human-readable
 * JSON format. Supports all built-in components and can be extended for
 * custom components.
 *
 * Features:
 * - Schema versioning with automatic migration
 * - Parent/child hierarchy support
 * - Component serialization (Transform, SpriteRenderer, Camera, etc.)
 *
 * Usage:
 *   SceneSerializer serializer(world);
 *   serializer.saveToFile("scenes/level1.json");
 *
 *   // Later...
 *   serializer.loadFromFile("scenes/level1.json");
 */
class LIMBO_API SceneSerializer {
public:
    /**
     * Create a serializer for the given world
     * @param world The ECS world to serialize/deserialize
     */
    explicit SceneSerializer(World& world);

    /**
     * Save the current world state to a JSON file
     * @param path Path to save the file
     * @return true on success, false on failure
     */
    bool saveToFile(const std::filesystem::path& path);

    /**
     * Load world state from a JSON file
     * This will clear the current world and load the saved entities
     * @param path Path to the file to load
     * @return true on success, false on failure
     */
    bool loadFromFile(const std::filesystem::path& path);

    /**
     * Serialize the world to a JSON string
     * @return JSON string representation of the world
     */
    [[nodiscard]] String serialize();

    /**
     * Deserialize a JSON string into the world
     * This will clear the current world and load the entities
     * @param jsonStr JSON string to deserialize
     * @return true on success, false on failure
     */
    bool deserialize(const String& jsonStr);

    /**
     * Get the last error message
     */
    [[nodiscard]] const String& getError() const { return m_error; }

private:
    World& m_world;
    String m_error;
};

}  // namespace limbo
