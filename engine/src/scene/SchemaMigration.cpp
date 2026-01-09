#include "limbo/scene/SchemaMigration.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

void SchemaMigration::registerMigration(i32 fromVersion, MigrationFunc migration) {
    m_migrations[fromVersion] = std::move(migration);

    // Update current version if this migration goes to a higher version
    if (fromVersion + 1 > m_currentVersion) {
        m_currentVersion = fromVersion + 1;
    }
}

bool SchemaMigration::hasMigration(i32 fromVersion) const {
    return m_migrations.find(fromVersion) != m_migrations.end();
}

bool SchemaMigration::migrate(json& data, i32 fromVersion, i32 toVersion) {
    if (fromVersion == toVersion) {
        return true;  // Nothing to do
    }

    if (fromVersion > toVersion) {
        m_error = "Cannot downgrade schema version";
        spdlog::error("SchemaMigration: {}", m_error);
        return false;
    }

    // Apply migrations sequentially
    for (i32 version = fromVersion; version < toVersion; ++version) {
        auto it = m_migrations.find(version);
        if (it == m_migrations.end()) {
            m_error = "No migration found for version " + std::to_string(version);
            spdlog::error("SchemaMigration: {}", m_error);
            return false;
        }

        spdlog::info("SchemaMigration: Migrating from v{} to v{}", version, version + 1);

        if (!it->second(data)) {
            m_error = "Migration from v" + std::to_string(version) + " to v" +
                      std::to_string(version + 1) + " failed";
            spdlog::error("SchemaMigration: {}", m_error);
            return false;
        }

        // Update version in data
        data["version"] = version + 1;
    }

    spdlog::info("SchemaMigration: Successfully migrated from v{} to v{}", fromVersion, toVersion);
    return true;
}

SchemaMigration SchemaMigration::createSceneMigrationRegistry() {
    SchemaMigration registry;
    registry.setCurrentVersion(2);

    // Register scene migrations
    registry.registerMigration(1, migrations::sceneV1ToV2);

    return registry;
}

namespace migrations {

bool sceneV1ToV2(json& data) {
    // V1 had version as string "1.0", V2 uses integer
    // The version field is updated by the migrate() function

    // V1 scenes had minimal component support
    // V2 adds: Hierarchy, Camera, more SpriteRenderer fields

    // No structural changes needed for existing data - v1 scenes
    // are a subset of v2. The main change is the version format
    // and support for new components which weren't in v1.

    // Ensure entities array exists
    if (!data.contains("entities")) {
        data["entities"] = json::array();
    }

    // Ensure each entity has a components object
    for (auto& entity : data["entities"]) {
        if (!entity.contains("components")) {
            entity["components"] = json::object();
        }

        // Ensure Transform has all fields (v1 might have partial data)
        if (entity["components"].contains("Transform")) {
            auto& transform = entity["components"]["Transform"];
            if (!transform.contains("position")) {
                transform["position"] = json::array({0.0f, 0.0f, 0.0f});
            }
            if (!transform.contains("rotation")) {
                transform["rotation"] = json::array({0.0f, 0.0f, 0.0f});
            }
            if (!transform.contains("scale")) {
                transform["scale"] = json::array({1.0f, 1.0f, 1.0f});
            }
        }

        // Ensure SpriteRenderer has all v2 fields
        if (entity["components"].contains("SpriteRenderer")) {
            auto& sprite = entity["components"]["SpriteRenderer"];
            if (!sprite.contains("color")) {
                sprite["color"] = json::array({1.0f, 1.0f, 1.0f, 1.0f});
            }
            if (!sprite.contains("sortingOrder")) {
                sprite["sortingOrder"] = 0;
            }
            if (!sprite.contains("uvMin")) {
                sprite["uvMin"] = json::array({0.0f, 0.0f});
            }
            if (!sprite.contains("uvMax")) {
                sprite["uvMax"] = json::array({1.0f, 1.0f});
            }
        }
    }

    return true;
}

}  // namespace migrations

}  // namespace limbo
