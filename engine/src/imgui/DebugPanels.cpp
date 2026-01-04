#include "limbo/imgui/DebugPanels.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/assets/AssetManager.hpp"
#include "limbo/render/2d/Renderer2D.hpp"

#include <imgui.h>
#include <deque>
#include <string>

namespace limbo {
namespace DebugPanels {

// ============================================================================
// Stats Panel
// ============================================================================

void showStatsPanel(f32 deltaTime) {
    ImGui::Begin("Stats");

    // FPS and frame time
    static float fpsHistory[120] = {0};
    static int fpsIndex = 0;
    static float fpsUpdateTimer = 0.0f;
    static float displayFps = 0.0f;
    static float displayMs = 0.0f;

    float const fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    fpsHistory[fpsIndex] = fps;
    fpsIndex = (fpsIndex + 1) % 120;

    fpsUpdateTimer += deltaTime;
    if (fpsUpdateTimer >= 0.25f) {
        fpsUpdateTimer = 0.0f;
        float sum = 0.0f;
        for (float const f : fpsHistory) {
            sum += f;
        }
        displayFps = sum / 120.0f;
        displayMs = 1000.0f / displayFps;
    }

    ImGui::Text("FPS: %.1f", displayFps);
    ImGui::Text("Frame Time: %.2f ms", displayMs);

    // FPS graph
    ImGui::PlotLines("##FPS", fpsHistory, 120, fpsIndex, nullptr, 0.0f, 120.0f, ImVec2(0, 50));

    ImGui::Separator();

    // Renderer2D stats
    auto stats = Renderer2D::getStats();
    ImGui::Text("Renderer2D:");
    ImGui::Text("  Draw Calls: %u", stats.drawCalls);
    ImGui::Text("  Quads: %u", stats.quadCount);
    ImGui::Text("  Vertices: %u", stats.quadCount * 4);
    ImGui::Text("  Indices: %u", stats.quadCount * 6);

    ImGui::End();
}

// ============================================================================
// Entity Inspector
// ============================================================================

void showEntityInspector(World& world) {
    ImGui::Begin("Entity Inspector");

    ImGui::Text("Entities: %zu", world.entityCount());
    ImGui::Separator();

    // Get all entities with NameComponent
    auto view = world.view<NameComponent>();

    static World::EntityId selectedEntity = entt::null;

    // Entity list
    if (ImGui::BeginChild("EntityList", ImVec2(200, 0),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX)) {
        for (auto entity : view) {
            const auto& name = view.get<NameComponent>(entity);

            bool const isSelected = (selectedEntity == entity);
            if (ImGui::Selectable(name.name.c_str(), isSelected)) {
                selectedEntity = entity;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Component details
    if (ImGui::BeginChild("ComponentDetails", ImVec2(0, 0))) {
        if (selectedEntity != entt::null && world.isValid(selectedEntity)) {
            // Name
            if (world.hasComponent<NameComponent>(selectedEntity)) {
                auto& name = world.getComponent<NameComponent>(selectedEntity);
                ImGui::Text("Name: %s", name.name.c_str());
                ImGui::Separator();
            }

            // Transform
            if (world.hasComponent<TransformComponent>(selectedEntity)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& transform = world.getComponent<TransformComponent>(selectedEntity);

                    ImGui::DragFloat3("Position", &transform.position.x, 0.01f);

                    glm::vec3 rotationDeg = glm::degrees(transform.rotation);
                    if (ImGui::DragFloat3("Rotation", &rotationDeg.x, 1.0f)) {
                        transform.rotation = glm::radians(rotationDeg);
                    }

                    ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f, 0.01f, 100.0f);
                }
            }

            // Sprite Renderer
            if (world.hasComponent<SpriteRendererComponent>(selectedEntity)) {
                if (ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& sprite = world.getComponent<SpriteRendererComponent>(selectedEntity);

                    ImGui::ColorEdit4("Color", &sprite.color.x);
                }
            }
        } else {
            ImGui::TextDisabled("Select an entity to inspect");
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

// ============================================================================
// Asset Browser
// ============================================================================

void showAssetBrowser(AssetManager& assetManager) {
    ImGui::Begin("Asset Browser");

    ImGui::Text("Asset Root: %s", assetManager.getAssetRoot().string().c_str());
    ImGui::Text("Loaded Assets: %zu", assetManager.assetCount());
    ImGui::Text("Hot-Reload: %s", assetManager.isHotReloadEnabled() ? "Enabled" : "Disabled");

    ImGui::Separator();

    if (ImGui::Button("Reload All")) {
        assetManager.reloadAll();
    }

    ImGui::Separator();

    // Asset list would go here
    // For now, just show count since we don't have iteration over assets
    ImGui::TextDisabled("(Asset list not yet implemented)");

    ImGui::End();
}

// ============================================================================
// Demo Window
// ============================================================================

void showDemoWindow() {
    ImGui::ShowDemoWindow();
}

// ============================================================================
// Log Console
// ============================================================================

// Simple ring buffer for log messages
struct LogEntry {
    std::string message;
    int level;  // 0=trace, 1=debug, 2=info, 3=warn, 4=error
};

static std::deque<LogEntry> s_logBuffer;
static constexpr size_t MAX_LOG_ENTRIES = 500;

void showLogConsole() {
    ImGui::Begin("Log Console");

    // Filter buttons
    static bool showTrace = false;
    static bool showDebug = true;
    static bool showInfo = true;
    static bool showWarn = true;
    static bool showError = true;

    ImGui::Checkbox("Trace", &showTrace);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &showDebug);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warn", &showWarn);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);

    if (ImGui::Button("Clear")) {
        s_logBuffer.clear();
    }

    ImGui::Separator();

    // Log display
    if (ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        for (const auto& entry : s_logBuffer) {
            bool show = false;
            ImVec4 color;

            switch (entry.level) {
            case 0:
                show = showTrace;
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                break;
            case 1:
                show = showDebug;
                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                break;
            case 2:
                show = showInfo;
                color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                break;
            case 3:
                show = showWarn;
                color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                break;
            case 4:
                show = showError;
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                break;
            default:
                show = true;
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            }

            if (show) {
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(entry.message.c_str());
                ImGui::PopStyleColor();
            }
        }

        // Auto-scroll
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace DebugPanels
}  // namespace limbo
