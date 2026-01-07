#include "limbo/imgui/DebugPanels.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/assets/AssetManager.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/debug/Log.hpp"

#include <imgui.h>
#include <deque>
#include <mutex>
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

namespace {

struct ConsoleLogEntry {
    std::string message;
    std::string category;
    spdlog::level::level_enum level;
};

std::deque<ConsoleLogEntry> s_logBuffer;
std::mutex s_logBufferMutex;
constexpr size_t MAX_LOG_ENTRIES = 1000;
bool s_callbackRegistered = false;

void logCallback(const log::LogEntry& entry) {
    std::lock_guard<std::mutex> lock(s_logBufferMutex);

    ConsoleLogEntry consoleEntry;
    consoleEntry.message = entry.message;
    consoleEntry.category = entry.category;
    consoleEntry.level = entry.level;

    s_logBuffer.push_back(std::move(consoleEntry));

    // Limit buffer size
    while (s_logBuffer.size() > MAX_LOG_ENTRIES) {
        s_logBuffer.pop_front();
    }
}

ImVec4 getLevelColor(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::trace:
        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    case spdlog::level::debug:
        return ImVec4(0.6f, 0.6f, 0.8f, 1.0f);
    case spdlog::level::info:
        return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
    case spdlog::level::warn:
        return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
    case spdlog::level::err:
        return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    case spdlog::level::critical:
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
    default:
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

const char* getLevelName(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::trace:
        return "TRACE";
    case spdlog::level::debug:
        return "DEBUG";
    case spdlog::level::info:
        return "INFO";
    case spdlog::level::warn:
        return "WARN";
    case spdlog::level::err:
        return "ERROR";
    case spdlog::level::critical:
        return "CRIT";
    default:
        return "???";
    }
}

}  // namespace

void showLogConsole() {
    // Register callback on first call
    if (!s_callbackRegistered) {
        log::addLogCallback(logCallback);
        s_callbackRegistered = true;
    }

    ImGui::Begin("Log Console");

    // Filter buttons
    static bool showTrace = false;
    static bool showDebug = true;
    static bool showInfo = true;
    static bool showWarn = true;
    static bool showError = true;

    // Category filter
    static char categoryFilter[64] = "";

    ImGui::Checkbox("Trace", &showTrace);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &showDebug);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warn", &showWarn);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(s_logBufferMutex);
        s_logBuffer.clear();
    }

    ImGui::SetNextItemWidth(120);
    ImGui::InputTextWithHint("##CategoryFilter", "Category filter", categoryFilter,
                             sizeof(categoryFilter));

    ImGui::Separator();

    // Log entry count
    {
        std::lock_guard<std::mutex> lock(s_logBufferMutex);
        ImGui::Text("Entries: %zu / %zu", s_logBuffer.size(), MAX_LOG_ENTRIES);
    }

    ImGui::Separator();

    // Log display
    if (ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        std::lock_guard<std::mutex> lock(s_logBufferMutex);

        for (const auto& entry : s_logBuffer) {
            // Level filter
            bool showLevel = false;
            switch (entry.level) {
            case spdlog::level::trace:
                showLevel = showTrace;
                break;
            case spdlog::level::debug:
                showLevel = showDebug;
                break;
            case spdlog::level::info:
                showLevel = showInfo;
                break;
            case spdlog::level::warn:
                showLevel = showWarn;
                break;
            case spdlog::level::err:
            case spdlog::level::critical:
                showLevel = showError;
                break;
            default:
                showLevel = true;
                break;
            }

            if (!showLevel) {
                continue;
            }

            // Category filter
            if (categoryFilter[0] != '\0') {
                if (entry.category.find(categoryFilter) == std::string::npos) {
                    continue;
                }
            }

            // Format: [LEVEL] [CATEGORY] message
            ImVec4 color = getLevelColor(entry.level);
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            ImGui::Text("[%s] [%s] %s", getLevelName(entry.level), entry.category.c_str(),
                        entry.message.c_str());

            ImGui::PopStyleColor();
        }

        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace DebugPanels
}  // namespace limbo
