#include "PrefabStage.hpp"
#include "EditorApp.hpp"

#include <limbo/debug/Log.hpp>
#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/scene/SceneSerializer.hpp>

namespace limbo::editor {

PrefabStage::PrefabStage(EditorApp& editor) : m_editor(editor) {}

bool PrefabStage::open(const std::filesystem::path& prefabPath) {
    if (m_isOpen) {
        LIMBO_LOG_EDITOR_WARN("PrefabStage: Already editing a prefab, close it first");
        return false;
    }

    // Load the prefab
    if (!m_prefab.loadFromFile(prefabPath)) {
        LIMBO_LOG_EDITOR_ERROR("PrefabStage: Failed to load prefab: {}", prefabPath.string());
        return false;
    }

    m_prefabPath = prefabPath;
    m_prefabName = m_prefab.getName();

    // Save current scene state
    SceneSerializer sceneSerializer(m_editor.getWorld());
    m_savedSceneState = sceneSerializer.serialize();
    m_savedSelection = m_editor.getSelectedEntity();

    // Clear the prefab world and instantiate the prefab into it
    m_prefabWorld.clear();
    Entity prefabRoot = m_prefab.instantiate(m_prefabWorld, glm::vec3(0.0f));

    if (!prefabRoot.isValid()) {
        LIMBO_LOG_EDITOR_ERROR("PrefabStage: Failed to instantiate prefab for editing");
        return false;
    }

    // Swap the editor's world with the prefab world
    // The editor will now work with the prefab world
    std::swap(m_editor.getWorld(), m_prefabWorld);

    // Select the root entity
    m_editor.selectEntity(Entity(prefabRoot.id(), &m_editor.getWorld()));

    m_isOpen = true;
    m_hasUnsavedChanges = false;

    LIMBO_LOG_EDITOR_INFO("PrefabStage: Opened prefab '{}' for editing", m_prefabName);
    return true;
}

bool PrefabStage::openFromInstance(const UUID& prefabId) {
    // Search for the prefab file in assets/prefabs/
    std::filesystem::path prefabsDir = std::filesystem::current_path() / "assets" / "prefabs";

    if (!std::filesystem::exists(prefabsDir)) {
        LIMBO_LOG_EDITOR_ERROR("PrefabStage: Prefabs directory not found");
        return false;
    }

    // Search for a prefab file with matching ID
    for (const auto& entry : std::filesystem::recursive_directory_iterator(prefabsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".prefab") {
            Prefab tempPrefab;
            if (tempPrefab.loadFromFile(entry.path())) {
                if (tempPrefab.getPrefabId() == prefabId) {
                    return open(entry.path());
                }
            }
        }
    }

    LIMBO_LOG_EDITOR_ERROR("PrefabStage: Could not find prefab with ID: {}", prefabId.toString());
    return false;
}

bool PrefabStage::save() {
    if (!m_isOpen) {
        LIMBO_LOG_EDITOR_WARN("PrefabStage: No prefab is open for editing");
        return false;
    }

    // Find the root entity in the prefab world (the one we're editing)
    World::EntityId rootId = World::kNullEntity;
    m_editor.getWorld().each<NameComponent>([&](World::EntityId id, NameComponent&) {
        // Find an entity without a parent (root)
        if (!m_editor.getWorld().hasComponent<HierarchyComponent>(id) ||
            !m_editor.getWorld().getComponent<HierarchyComponent>(id).hasParent()) {
            if (rootId == World::kNullEntity) {
                rootId = id;
            }
        }
    });

    if (rootId == World::kNullEntity) {
        LIMBO_LOG_EDITOR_ERROR("PrefabStage: Could not find root entity in prefab world");
        return false;
    }

    // Create a new prefab from the current state
    Prefab newPrefab = Prefab::createFromEntity(m_editor.getWorld(), rootId);
    newPrefab.setName(m_prefabName);

    // Save to file
    if (!newPrefab.saveToFile(m_prefabPath)) {
        LIMBO_LOG_EDITOR_ERROR("PrefabStage: Failed to save prefab: {}", m_prefabPath.string());
        return false;
    }

    m_prefab = newPrefab;
    m_hasUnsavedChanges = false;

    LIMBO_LOG_EDITOR_INFO("PrefabStage: Saved prefab '{}'", m_prefabName);
    return true;
}

void PrefabStage::close(bool saveChanges) {
    if (!m_isOpen) {
        return;
    }

    if (saveChanges && m_hasUnsavedChanges) {
        save();
    }

    // Swap back to the original scene world
    std::swap(m_editor.getWorld(), m_prefabWorld);

    // Restore the scene state
    if (!m_savedSceneState.empty()) {
        m_editor.getWorld().clear();
        SceneSerializer sceneSerializer(m_editor.getWorld());
        if (!sceneSerializer.deserialize(m_savedSceneState)) {
            LIMBO_LOG_EDITOR_ERROR("PrefabStage: Failed to restore scene state");
        }
    }

    // Restore selection if the entity still exists
    if (m_savedSelection.isValid()) {
        m_editor.selectEntity(m_savedSelection);
    } else {
        m_editor.deselectAll();
    }

    // Update instances of this prefab in the scene
    if (saveChanges) {
        updateSceneInstances();
    }

    // Clear prefab world
    m_prefabWorld.clear();

    m_isOpen = false;
    m_hasUnsavedChanges = false;
    m_savedSceneState.clear();
    m_prefabPath.clear();
    m_prefabName.clear();

    LIMBO_LOG_EDITOR_INFO("PrefabStage: Closed prefab editor");
}

void PrefabStage::updateSceneInstances() {
    // Find all instances of this prefab in the scene and update them
    UUID prefabId = m_prefab.getPrefabId();

    // Collect all root instances first (can't modify registry while iterating)
    std::vector<World::EntityId> instanceRoots;
    m_editor.getWorld().each<PrefabInstanceComponent>(
        [&prefabId, &instanceRoots](World::EntityId id, PrefabInstanceComponent& inst) {
            if (inst.prefabId == prefabId && inst.isRoot) {
                instanceRoots.push_back(id);
            }
        });

    if (instanceRoots.empty()) {
        LIMBO_LOG_EDITOR_INFO("PrefabStage: No instances to update");
        return;
    }

    LIMBO_LOG_EDITOR_INFO("PrefabStage: Updating {} instances of prefab '{}'", instanceRoots.size(),
                          m_prefabName);

    for (World::EntityId rootId : instanceRoots) {
        if (!m_editor.getWorld().isValid(rootId)) {
            continue;
        }

        // Step 1: Save current overrides from this instance
        std::vector<PrefabOverride> savedOverrides;
        glm::vec3 savedPosition{0.0f};

        // Save root position (which is often an override)
        if (m_editor.getWorld().hasComponent<TransformComponent>(rootId)) {
            savedPosition = m_editor.getWorld().getComponent<TransformComponent>(rootId).position;
        }

        // Collect overrides from all entities in the instance
        std::function<void(World::EntityId)> collectOverrides = [&](World::EntityId entityId) {
            auto* inst = m_editor.getWorld().tryGetComponent<PrefabInstanceComponent>(entityId);
            if (inst != nullptr && inst->prefabId == prefabId) {
                for (const auto& override : inst->overrides) {
                    savedOverrides.push_back(override);
                }
            }

            Hierarchy::forEachChild(m_editor.getWorld(), entityId, [&](World::EntityId childId) {
                collectOverrides(childId);
                return true;
            });
        };
        collectOverrides(rootId);

        // Step 2: Delete the old instance hierarchy
        std::vector<World::EntityId> entitiesToDelete;
        std::function<void(World::EntityId)> collectForDelete = [&](World::EntityId entityId) {
            Hierarchy::forEachChild(m_editor.getWorld(), entityId, [&](World::EntityId childId) {
                collectForDelete(childId);
                return true;
            });
            entitiesToDelete.push_back(entityId);
        };
        collectForDelete(rootId);

        // Delete in reverse order (children first)
        for (auto it = entitiesToDelete.rbegin(); it != entitiesToDelete.rend(); ++it) {
            m_editor.getWorld().destroyEntity(*it);
        }

        // Step 3: Re-instantiate from the updated prefab
        Entity newRoot = m_prefab.instantiate(m_editor.getWorld(), savedPosition);

        if (!newRoot.isValid()) {
            LIMBO_LOG_EDITOR_ERROR("PrefabStage: Failed to re-instantiate prefab");
            continue;
        }

        // Step 4: Re-apply saved overrides
        for (const auto& override : savedOverrides) {
            // Find the entity with matching localId
            std::function<void(World::EntityId)> applyOverride = [&](World::EntityId entityId) {
                auto* inst = m_editor.getWorld().tryGetComponent<PrefabInstanceComponent>(entityId);
                if (inst != nullptr && inst->localId == override.targetLocalId) {
                    inst->overrides.push_back(override);

                    // Also apply the actual value to the component
                    // This is a simplified version - for full support we'd need
                    // to handle each component type
                    if (override.component == "Transform" &&
                        m_editor.getWorld().hasComponent<TransformComponent>(entityId)) {
                        auto& transform =
                            m_editor.getWorld().getComponent<TransformComponent>(entityId);
                        if (override.property == "position" && override.value.is_array()) {
                            transform.position = glm::vec3(override.value[0].get<f32>(),
                                                           override.value[1].get<f32>(),
                                                           override.value[2].get<f32>());
                        } else if (override.property == "rotation" && override.value.is_array()) {
                            transform.rotation = glm::vec3(override.value[0].get<f32>(),
                                                           override.value[1].get<f32>(),
                                                           override.value[2].get<f32>());
                        } else if (override.property == "scale" && override.value.is_array()) {
                            transform.scale = glm::vec3(override.value[0].get<f32>(),
                                                        override.value[1].get<f32>(),
                                                        override.value[2].get<f32>());
                        }
                    }
                    // Add more component types as needed...
                }

                Hierarchy::forEachChild(m_editor.getWorld(), entityId,
                                        [&](World::EntityId childId) {
                                            applyOverride(childId);
                                            return true;
                                        });
            };
            applyOverride(newRoot.id());
        }

        LIMBO_LOG_EDITOR_INFO("PrefabStage: Updated instance with {} overrides preserved",
                              savedOverrides.size());
    }

    m_editor.markSceneModified();
}

}  // namespace limbo::editor
