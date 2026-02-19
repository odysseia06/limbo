#include "PrefabOverridesPanel.hpp"
#include "../commands/Command.hpp"
#include "EditorApp.hpp"

#include <limbo/ecs/Hierarchy.hpp>
#include <limbo/scene/Prefab.hpp>

#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace limbo::editor {

namespace {

/// Command to apply all prefab overrides (undoable)
///
/// Captures the original prefab file content and instance overrides before
/// applying, so undo can fully restore both the asset file and world state.
class ApplyAllOverridesCommand : public Command {
public:
    ApplyAllOverridesCommand(World& world, World::EntityId rootId, UUID prefabId,
                             std::filesystem::path prefabPath)
        : m_world(world), m_rootId(rootId), m_prefabId(prefabId),
          m_prefabPath(std::move(prefabPath)) {}

    bool execute() override {
        // Snapshot the original prefab file for undo
        {
            std::ifstream file(m_prefabPath, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            std::ostringstream ss;
            ss << file.rdbuf();
            m_originalPrefabContent = ss.str();
        }

        // Snapshot instance overrides for undo
        m_savedOverrides.clear();
        captureOverrides(m_rootId);

        // Load prefab, apply overrides to its data, and save to disk.
        // applyInstanceChanges modifies both the Prefab object and clears
        // overrides on the world entities, so if save fails we must restore.
        Prefab prefab;
        if (!prefab.loadFromFile(m_prefabPath)) {
            return false;
        }
        if (!prefab.applyInstanceChanges(m_world, m_rootId)) {
            return false;
        }
        if (!prefab.saveToFile(m_prefabPath)) {
            // Save failed — restore world overrides that applyInstanceChanges cleared
            restoreOverrides();
            return false;
        }
        prefab.updateInstances(m_world, true);
        return true;
    }

    bool undo() override {
        // Restore the original prefab file
        {
            std::ofstream file(m_prefabPath, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return false;
            }
            file << m_originalPrefabContent;
            if (!file.good()) {
                return false;
            }
        }

        // Restore instance overrides
        restoreOverrides();

        // Reload the restored prefab and update instances
        Prefab prefab;
        if (!prefab.loadFromFile(m_prefabPath)) {
            return false;
        }
        prefab.updateInstances(m_world, true);
        return true;
    }

    [[nodiscard]] String getDescription() const override { return "Apply All Prefab Overrides"; }

    COMMAND_TYPE_ID()

private:
    void captureOverrides(World::EntityId entityId) {
        auto* inst = m_world.tryGetComponent<PrefabInstanceComponent>(entityId);
        if (inst != nullptr && inst->prefabId == m_prefabId) {
            m_savedOverrides[entityId] = inst->overrides;
        }
        Hierarchy::forEachChild(m_world, entityId, [this](World::EntityId childId) {
            captureOverrides(childId);
            return true;
        });
    }

    void restoreOverrides() {
        for (auto& [entityId, overrides] : m_savedOverrides) {
            if (!m_world.isValid(entityId)) {
                continue;
            }
            auto* inst = m_world.tryGetComponent<PrefabInstanceComponent>(entityId);
            if (inst != nullptr) {
                inst->overrides = overrides;
            }
        }
    }

    World& m_world;
    World::EntityId m_rootId;
    UUID m_prefabId;
    std::filesystem::path m_prefabPath;
    String m_originalPrefabContent;
    std::unordered_map<World::EntityId, std::vector<PrefabOverride>> m_savedOverrides;
};

}  // namespace

PrefabOverridesPanel::PrefabOverridesPanel(EditorApp& editor) : m_editor(editor) {}

void PrefabOverridesPanel::init() {}

void PrefabOverridesPanel::shutdown() {}

void PrefabOverridesPanel::render() {
    if (!m_open) {
        return;
    }

    ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Prefab Overrides", &m_open, windowFlags);

    Entity selectedEntity = m_editor.getSelectedEntity();

    if (!selectedEntity.isValid()) {
        drawEmptyState();
        ImGui::End();
        return;
    }

    // Check if selected entity or any parent is a prefab instance
    Entity prefabRoot;
    Entity current = selectedEntity;
    while (current.isValid()) {
        if (current.hasComponent<PrefabInstanceComponent>()) {
            auto& prefabInst = current.getComponent<PrefabInstanceComponent>();
            if (prefabInst.isRoot) {
                prefabRoot = current;
                break;
            }
        }
        // Move to parent
        if (current.hasComponent<HierarchyComponent>()) {
            auto& hierarchy = current.getComponent<HierarchyComponent>();
            if (hierarchy.parent != entt::null) {
                current = Entity(hierarchy.parent, &m_editor.getWorld());
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (!prefabRoot.isValid()) {
        drawEmptyState();
        ImGui::End();
        return;
    }

    // We have a prefab instance root
    auto& rootPrefabInst = prefabRoot.getComponent<PrefabInstanceComponent>();

    // Draw header info
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.9f, 1.0f), "Prefab Instance");
    if (prefabRoot.hasComponent<NameComponent>()) {
        ImGui::SameLine();
        ImGui::Text("- %s", prefabRoot.getComponent<NameComponent>().name.c_str());
    }

    ImGui::Separator();

    drawToolbar();

    ImGui::Separator();

    // Collect all prefab instance entities in the hierarchy
    std::vector<Entity> prefabInstances;
    collectPrefabInstances(prefabRoot, prefabInstances);

    // Count total overrides
    usize totalOverrides = 0;
    for (auto& inst : prefabInstances) {
        if (inst.hasComponent<PrefabInstanceComponent>()) {
            totalOverrides += inst.getComponent<PrefabInstanceComponent>().overrides.size();
        }
    }

    if (totalOverrides == 0) {
        ImGui::TextDisabled("No overrides");
        ImGui::TextWrapped("Modify properties on this prefab instance to create overrides. "
                           "Overrides are changes that differ from the original prefab asset.");
        ImGui::End();
        return;
    }

    // Draw overrides list
    ImGui::Text("Overrides: %zu", totalOverrides);
    ImGui::Separator();

    drawOverridesList();

    ImGui::End();
}

void PrefabOverridesPanel::drawToolbar() {
    Entity selectedEntity = m_editor.getSelectedEntity();
    if (!selectedEntity.isValid()) {
        return;
    }

    // Find prefab root
    Entity prefabRoot;
    Entity current = selectedEntity;
    while (current.isValid()) {
        if (current.hasComponent<PrefabInstanceComponent>()) {
            auto& prefabInst = current.getComponent<PrefabInstanceComponent>();
            if (prefabInst.isRoot) {
                prefabRoot = current;
                break;
            }
        }
        if (current.hasComponent<HierarchyComponent>()) {
            auto& hierarchy = current.getComponent<HierarchyComponent>();
            if (hierarchy.parent != entt::null) {
                current = Entity(hierarchy.parent, &m_editor.getWorld());
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (!prefabRoot.isValid()) {
        return;
    }

    auto& rootPrefabInst = prefabRoot.getComponent<PrefabInstanceComponent>();

    float buttonWidth = 100.0f;

    // Revert All button
    if (ImGui::Button("Revert All", ImVec2(buttonWidth, 0))) {
        // Collect all instances and clear their overrides
        std::vector<Entity> instances;
        collectPrefabInstances(prefabRoot, instances);
        for (auto& inst : instances) {
            if (inst.hasComponent<PrefabInstanceComponent>()) {
                inst.getComponent<PrefabInstanceComponent>().clearAllOverrides();
            }
        }
        m_editor.markSceneModified();
        LIMBO_LOG_EDITOR_INFO("Reverted all overrides on prefab instance");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Revert all overrides to match the original prefab");
    }

    ImGui::SameLine();

    // Apply All button
    if (ImGui::Button("Apply All", ImVec2(buttonWidth, 0))) {
        // Find the prefab file on disk
        std::filesystem::path prefabsDir = std::filesystem::current_path() / "assets" / "prefabs";
        std::filesystem::path foundPath;

        if (std::filesystem::exists(prefabsDir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(prefabsDir)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".prefab") {
                    continue;
                }
                Prefab tempPrefab;
                if (tempPrefab.loadFromFile(entry.path()) &&
                    tempPrefab.getPrefabId() == rootPrefabInst.prefabId) {
                    foundPath = entry.path();
                    break;
                }
            }
        }

        if (!foundPath.empty()) {
            auto cmd = std::make_unique<ApplyAllOverridesCommand>(
                m_editor.getWorld(), prefabRoot.id(), rootPrefabInst.prefabId, foundPath);
            m_editor.executeCommand(std::move(cmd));
        } else {
            LIMBO_LOG_EDITOR_WARN("Could not find prefab asset with ID: {}",
                                  rootPrefabInst.prefabId.toString());
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Apply all overrides to the prefab asset");
    }

    ImGui::SameLine();

    // Open Prefab button
    if (ImGui::Button("Open Prefab", ImVec2(buttonWidth, 0))) {
        m_editor.getPrefabStage().openFromInstance(rootPrefabInst.prefabId);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Edit the prefab asset in isolation");
    }

    // Filter input
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##OverrideFilter", "Filter overrides...", m_filterBuffer,
                             sizeof(m_filterBuffer));
}

void PrefabOverridesPanel::drawOverridesList() {
    Entity selectedEntity = m_editor.getSelectedEntity();
    if (!selectedEntity.isValid()) {
        return;
    }

    // Find prefab root
    Entity prefabRoot;
    Entity current = selectedEntity;
    while (current.isValid()) {
        if (current.hasComponent<PrefabInstanceComponent>()) {
            auto& prefabInst = current.getComponent<PrefabInstanceComponent>();
            if (prefabInst.isRoot) {
                prefabRoot = current;
                break;
            }
        }
        if (current.hasComponent<HierarchyComponent>()) {
            auto& hierarchy = current.getComponent<HierarchyComponent>();
            if (hierarchy.parent != entt::null) {
                current = Entity(hierarchy.parent, &m_editor.getWorld());
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (!prefabRoot.isValid()) {
        return;
    }

    // Collect all instances
    std::vector<Entity> prefabInstances;
    collectPrefabInstances(prefabRoot, prefabInstances);

    String filterStr(m_filterBuffer);
    // Convert to lowercase for case-insensitive matching
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Begin table for overrides
    ImGuiTableFlags const tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

    float availableHeight = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("OverridesTable", 4, tableFlags, ImVec2(0, availableHeight))) {
        ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (auto& inst : prefabInstances) {
            if (!inst.hasComponent<PrefabInstanceComponent>()) {
                continue;
            }

            auto& prefabInst = inst.getComponent<PrefabInstanceComponent>();

            for (usize i = 0; i < prefabInst.overrides.size(); ++i) {
                const auto& override = prefabInst.overrides[i];

                // Apply filter
                if (!filterStr.empty()) {
                    String searchable =
                        override.component + "." + override.property + " " + override.targetLocalId;
                    std::transform(searchable.begin(), searchable.end(), searchable.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (searchable.find(filterStr) == String::npos) {
                        continue;
                    }
                }

                drawOverrideRow(inst, prefabInst, override, i);
            }
        }

        ImGui::EndTable();
    }
}

void PrefabOverridesPanel::drawOverrideRow(Entity entity, PrefabInstanceComponent& prefabInst,
                                           const PrefabOverride& override, usize index) {
    ImGui::TableNextRow();

    // Entity column
    ImGui::TableNextColumn();
    String entityName = "Entity";
    if (entity.hasComponent<NameComponent>()) {
        entityName = entity.getComponent<NameComponent>().name;
    }
    // Highlight if this is the selected entity
    if (entity == m_editor.getSelectedEntity()) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", entityName.c_str());
    } else {
        ImGui::Text("%s", entityName.c_str());
    }
    // Click to select entity
    if (ImGui::IsItemClicked()) {
        m_editor.selectEntity(entity);
    }

    // Property column
    ImGui::TableNextColumn();
    String propertyDisplay = getPropertyDisplayName(override.component, override.property);
    ImGui::Text("%s", propertyDisplay.c_str());

    // Value column
    ImGui::TableNextColumn();
    // Display value as string (simplified)
    String valueStr;
    try {
        if (override.value.is_array()) {
            valueStr = "[";
            for (usize j = 0; j < override.value.size() && j < 4; ++j) {
                if (j > 0)
                    valueStr += ", ";
                if (override.value[j].is_number()) {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "%.2f", override.value[j].get<f32>());
                    valueStr += buf;
                } else {
                    valueStr += override.value[j].dump();
                }
            }
            if (override.value.size() > 4) {
                valueStr += ", ...";
            }
            valueStr += "]";
        } else if (override.value.is_number()) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.3f", override.value.get<f32>());
            valueStr = buf;
        } else if (override.value.is_boolean()) {
            valueStr = override.value.get<bool>() ? "true" : "false";
        } else if (override.value.is_string()) {
            valueStr = "\"" + override.value.get<String>() + "\"";
        } else {
            valueStr = override.value.dump();
        }
    } catch (...) {
        valueStr = "(error)";
    }

    ImGui::TextWrapped("%s", valueStr.c_str());

    // Actions column
    ImGui::TableNextColumn();

    ImGui::PushID(static_cast<int>(index));

    // Revert button
    if (ImGui::SmallButton("Revert")) {
        prefabInst.clearOverride(override.component, override.property);
        m_editor.markSceneModified();
        LIMBO_LOG_EDITOR_INFO("Reverted override: {}.{}", override.component, override.property);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Revert this property to match the prefab");
    }

    ImGui::PopID();
}

void PrefabOverridesPanel::drawEmptyState() {
    ImGui::TextDisabled("No prefab instance selected");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a prefab instance in the scene to view and manage its overrides.");
}

String PrefabOverridesPanel::getPropertyDisplayName(const String& component,
                                                    const String& property) const {
    // Format: ComponentName.propertyName
    // Could add icons or formatting later
    return component + "." + property;
}

void PrefabOverridesPanel::collectPrefabInstances(Entity root, std::vector<Entity>& outInstances) {
    if (!root.isValid()) {
        return;
    }

    // Add this entity if it has PrefabInstanceComponent
    if (root.hasComponent<PrefabInstanceComponent>()) {
        outInstances.push_back(root);
    }

    // Recurse to children
    if (root.hasComponent<HierarchyComponent>()) {
        auto& hierarchy = root.getComponent<HierarchyComponent>();
        if (hierarchy.firstChild != entt::null) {
            Entity child(hierarchy.firstChild, &m_editor.getWorld());
            while (child.isValid()) {
                collectPrefabInstances(child, outInstances);

                // Move to next sibling
                if (child.hasComponent<HierarchyComponent>()) {
                    auto& childHierarchy = child.getComponent<HierarchyComponent>();
                    if (childHierarchy.nextSibling != entt::null) {
                        child = Entity(childHierarchy.nextSibling, &m_editor.getWorld());
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }
}

}  // namespace limbo::editor
