#include "ScriptDebugPanel.hpp"
#include "../EditorApp.hpp"

#include <limbo/scripting/ScriptComponent.hpp>
#include <limbo/ecs/Components.hpp>

#include <imgui.h>

namespace limbo::editor {

ScriptDebugPanel::ScriptDebugPanel(EditorApp& editor) : m_editor(editor) {}

void ScriptDebugPanel::render(World& world, ScriptSystem* scriptSystem) {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Script Debug", &m_open)) {
        drawToolbar(scriptSystem);
        ImGui::Separator();
        drawScriptList(world, scriptSystem);

        // Details panel at bottom if something is selected
        if (m_selectedScript != entt::null && world.isValid(m_selectedScript)) {
            ImGui::Separator();
            drawScriptDetails(world);
        }
    }
    ImGui::End();
}

void ScriptDebugPanel::drawToolbar(ScriptSystem* scriptSystem) {
    // Hot reload stats
    if (scriptSystem != nullptr) {
        auto& hotReload = scriptSystem->getHotReloadManager();

        ImGui::Text("Hot Reload:");
        ImGui::SameLine();

        if (hotReload.isEnabled()) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Enabled");
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Disabled");
        }

        ImGui::SameLine();
        ImGui::Text("| Reloads: %u", hotReload.getTotalReloads());

        if (hotReload.getFailedReloads() > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "| Failed: %u",
                               hotReload.getFailedReloads());
        }

        if (hotReload.getPendingReloadCount() > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "| Pending: %zu",
                               hotReload.getPendingReloadCount());
        }
    }

    // Filter toggles
    ImGui::Text("Show:");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
    ImGui::Checkbox("Running", &m_showRunning);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::Checkbox("Errors", &m_showErrors);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    ImGui::Checkbox("Pending", &m_showPending);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Checkbox("Disabled", &m_showDisabled);
    ImGui::PopStyleColor();
}

void ScriptDebugPanel::drawScriptList(World& world, ScriptSystem* scriptSystem) {
    ImGui::BeginChild("ScriptList", ImVec2(0, -100), ImGuiChildFlags_Borders);

    // Table header
    if (ImGui::BeginTable("ScriptsTable", 3,
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        world.each<ScriptComponent>([&](World::EntityId entityId, ScriptComponent& script) {
            // Determine status for filtering
            bool isError = script.hasError();
            bool isRunning = script.initialized && script.started && script.enabled && !isError;
            bool isPending = !script.initialized && script.enabled;
            bool isDisabled = !script.enabled;

            // Apply filters
            if (isError && !m_showErrors)
                return;
            if (isRunning && !m_showRunning)
                return;
            if (isPending && !m_showPending)
                return;
            if (isDisabled && !m_showDisabled)
                return;

            ImGui::TableNextRow();

            // Entity name column
            ImGui::TableNextColumn();
            String entityName = "Entity";
            if (world.hasComponent<NameComponent>(entityId)) {
                entityName = world.getComponent<NameComponent>(entityId).name;
            }

            bool isSelected = (m_selectedScript == entityId);
            if (ImGui::Selectable(entityName.c_str(), isSelected,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedScript = entityId;
            }

            // Script path column
            ImGui::TableNextColumn();
            if (script.scriptPath.empty()) {
                ImGui::TextDisabled("(none)");
            } else {
                String filename = script.scriptPath.filename().string();
                ImGui::TextUnformatted(filename.c_str());
            }

            // Status column
            ImGui::TableNextColumn();
            if (isError) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error");
            } else if (isDisabled) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Disabled");
            } else if (isRunning) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Running");
            } else if (isPending) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Pending");
            } else if (script.initialized) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Init");
            }
        });

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void ScriptDebugPanel::drawScriptDetails(World& world) {
    if (!world.hasComponent<ScriptComponent>(m_selectedScript)) {
        m_selectedScript = entt::null;
        return;
    }

    auto& script = world.getComponent<ScriptComponent>(m_selectedScript);

    ImGui::Text("Details");

    // Full script path
    ImGui::Text("Path: %s",
                script.scriptPath.empty() ? "(none)" : script.scriptPath.string().c_str());

    // Error details
    if (script.hasError()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        if (script.lastErrorLine > 0) {
            ImGui::Text("Error at line %d:", script.lastErrorLine);
        } else {
            ImGui::Text("Error:");
        }
        ImGui::TextWrapped("%s", script.lastError.c_str());
        ImGui::PopStyleColor();

        // Clear error button
        if (ImGui::Button("Clear Error & Retry")) {
            script.clearError();
            script.initialized = false;
            script.started = false;
            script.enabled = true;
        }
    }

    // Callbacks info
    if (script.initialized) {
        ImGui::Text("Callbacks:");
        ImGui::SameLine();

        if (script.onStart.valid())
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "onStart ");
        if (script.onUpdate.valid())
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "onUpdate ");
        if (script.onDestroy.valid())
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "onDestroy ");
        if (script.onCollisionBegin.valid())
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "onCollisionBegin ");
        if (script.onCollisionEnd.valid())
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "onCollisionEnd ");
        if (script.onTriggerEnter.valid())
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "onTriggerEnter ");
        if (script.onTriggerExit.valid())
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "onTriggerExit ");
    }

    // Actions
    ImGui::Spacing();
    if (ImGui::Button("Select Entity")) {
        Entity entity(m_selectedScript, &world);
        m_editor.selectEntity(entity);
    }

    ImGui::SameLine();
    if (ImGui::Button("Toggle Enabled")) {
        script.enabled = !script.enabled;
    }
}

}  // namespace limbo::editor
