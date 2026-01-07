#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <map>

namespace limbo {

using json = nlohmann::json;

/**
 * Migration function signature
 * Takes a JSON document and migrates it from version N to version N+1
 * Returns true if migration succeeded, false if failed
 */
using MigrationFunc = std::function<bool(json&)>;

/**
 * SchemaMigration - Manages schema versioning and migration for serialized data
 *
 * This class handles the migration of serialized JSON data between different
 * schema versions. Each migration function transforms data from version N
 * to version N+1.
 *
 * Usage:
 * 1. Register migrations: registry.registerMigration(1, migrateV1ToV2);
 * 2. Migrate data: registry.migrate(data, fromVersion, toVersion);
 *
 * Example migration function:
 * bool migrateV1ToV2(json& data) {
 *     // Rename "position" to "translation" in Transform components
 *     for (auto& entity : data["entities"]) {
 *         if (entity["components"].contains("Transform")) {
 *             auto& transform = entity["components"]["Transform"];
 *             if (transform.contains("position")) {
 *                 transform["translation"] = transform["position"];
 *                 transform.erase("position");
 *             }
 *         }
 *     }
 *     return true;
 * }
 */
class LIMBO_API SchemaMigration {
public:
    /**
     * Register a migration function for a specific version
     * @param fromVersion The version this migration upgrades FROM
     * @param migration The migration function
     */
    void registerMigration(i32 fromVersion, MigrationFunc migration);

    /**
     * Check if a migration exists for a specific version
     * @param fromVersion The version to check
     * @return True if a migration exists
     */
    [[nodiscard]] bool hasMigration(i32 fromVersion) const;

    /**
     * Migrate data from one version to another
     * @param data The JSON data to migrate (modified in place)
     * @param fromVersion The current version of the data
     * @param toVersion The target version
     * @return True if migration succeeded, false if failed
     */
    bool migrate(json& data, i32 fromVersion, i32 toVersion);

    /**
     * Get the current (latest) schema version
     */
    [[nodiscard]] i32 getCurrentVersion() const { return m_currentVersion; }

    /**
     * Set the current schema version
     */
    void setCurrentVersion(i32 version) { m_currentVersion = version; }

    /**
     * Get the error message from the last failed migration
     */
    [[nodiscard]] const String& getError() const { return m_error; }

    /**
     * Create a default scene migration registry with built-in migrations
     */
    static SchemaMigration createSceneMigrationRegistry();

private:
    std::map<i32, MigrationFunc> m_migrations;
    i32 m_currentVersion = 1;
    String m_error;
};

// ============================================================================
// Built-in migration functions for scenes
// ============================================================================

namespace migrations {

/**
 * Migrate scene from v1 to v2
 * - Version field changed from string "1.0" to integer 2
 * - Added Hierarchy component support
 * - Added Camera component support
 * - Added more SpriteRenderer fields
 */
bool sceneV1ToV2(json& data);

}  // namespace migrations

}  // namespace limbo
