#include "limbo/imgui/DebugPanels.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/physics/2d/PhysicsComponents2D.hpp"
#include "limbo/assets/AssetManager.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/debug/Log.hpp"
#include "limbo/debug/Profiler.hpp"
#include "limbo/debug/GPUTimer.hpp"
#include "limbo/core/FrameAllocator.hpp"
#include "limbo/core/ThreadPool.hpp"

#include <imgui.h>
#include <deque>
#include <mutex>
#include <string>

namespace limbo {
namespace DebugPanels {

// ============================================================================
// Stats Panel
// ============================================================================

void showStatsPanel(f32 deltaTime, GPUTimer* gpuTimer) {
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
    ImGui::Text("CPU Frame Time: %.2f ms", displayMs);

    // GPU timing (if available)
    if (gpuTimer != nullptr && gpuTimer->isInitialized()) {
        f64 const gpuTimeMs = gpuTimer->getTotalTimeMs();
        ImGui::Text("GPU Frame Time: %.2f ms", gpuTimeMs);

        // Show GPU vs CPU comparison
        if (gpuTimeMs > 0.0 && displayMs > 0.0f) {
            bool const gpuBound = gpuTimeMs > static_cast<f64>(displayMs);
            if (gpuBound) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "GPU Bound");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "CPU Bound");
            }
        }

        // Show individual GPU timers if there are multiple
        if (gpuTimer->getTimerCount() > 1) {
            if (ImGui::TreeNode("GPU Timers")) {
                for (u32 i = 0; i < gpuTimer->getTimerCount(); ++i) {
                    const char* name = gpuTimer->getTimerName(i);
                    f64 const timeMs = gpuTimer->getTimerTimeMs(i);
                    if (name != nullptr) {
                        ImGui::Text("  %s: %.2f ms", name, timeMs);
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    // FPS graph
    ImGui::PlotLines("##FPS", fpsHistory, 120, fpsIndex, nullptr, 0.0f, 120.0f, ImVec2(0, 50));

    ImGui::Separator();

    // Renderer2D stats
    auto stats = Renderer2D::getStats();
    ImGui::Text("Renderer2D:");
    ImGui::Text("  Draw Calls: %u", stats.drawCalls);
    ImGui::Text("  Batches: %u", stats.batchCount);
    ImGui::Text("  Quads: %u", stats.quadCount);
    ImGui::Text("  Lines: %u", stats.lineCount);
    ImGui::Text("  Vertices: %u", stats.vertexCount());
    ImGui::Text("  Texture Binds: %u", stats.textureBinds);

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

// ============================================================================
// Profiler Panel
// ============================================================================

namespace {

// Colors for profiler bars (cycle through these)
const ImVec4 PROFILER_COLORS[] = {
    ImVec4(0.4f, 0.6f, 0.9f, 1.0f),  // Blue
    ImVec4(0.5f, 0.8f, 0.5f, 1.0f),  // Green
    ImVec4(0.9f, 0.7f, 0.4f, 1.0f),  // Orange
    ImVec4(0.8f, 0.5f, 0.8f, 1.0f),  // Purple
    ImVec4(0.9f, 0.5f, 0.5f, 1.0f),  // Red
    ImVec4(0.5f, 0.8f, 0.8f, 1.0f),  // Cyan
    ImVec4(0.8f, 0.8f, 0.5f, 1.0f),  // Yellow
};
constexpr size_t PROFILER_COLOR_COUNT = sizeof(PROFILER_COLORS) / sizeof(PROFILER_COLORS[0]);

ImVec4 getProfilerColor(size_t index) {
    return PROFILER_COLORS[index % PROFILER_COLOR_COUNT];
}

// Smoothed sample data for stable display
struct SmoothedSample {
    const char* name = nullptr;
    f64 avgDurationMs = 0.0;
    u32 depth = 0;
};

// Profiler display state
struct ProfilerDisplayState {
    std::vector<SmoothedSample> smoothedSamples;
    f64 smoothedFrameTimeMs = 0.0;
    f32 updateTimer = 0.0f;
    bool paused = false;

    static constexpr f32 UPDATE_INTERVAL = 0.1f;  // Update every 100ms
    static constexpr f64 SMOOTHING_FACTOR = 0.3;  // Lower = smoother
};

ProfilerDisplayState s_profilerState;

}  // namespace

void showProfilerPanel() {
    ImGui::Begin("Profiler");

    // Capture controls
    static bool autoCaptureEnabled = true;

    if (s_profilerState.paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Resume")) {
            s_profilerState.paused = false;
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Pause")) {
            s_profilerState.paused = true;
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto Capture", &autoCaptureEnabled);
    ImGui::SameLine();

    if (ImGui::Button("Export CSV")) {
        if (profiler::Profiler::exportToCSV("profiler_data.csv")) {
            LIMBO_LOG_CORE_INFO("Profiler: Exported to profiler_data.csv");
        }
    }

    // Capture frame data if not paused
    if (!s_profilerState.paused && autoCaptureEnabled) {
        profiler::Profiler::captureFrame();
    }

    ImGui::Separator();

    // Get captured frame data
    const profiler::FrameData* frameData = profiler::Profiler::getCapturedFrame();
    if (!frameData || frameData->samples.empty()) {
        ImGui::TextDisabled("No profiler data captured");
        ImGui::End();
        return;
    }

    // Update smoothed values periodically (not every frame)
    s_profilerState.updateTimer += ImGui::GetIO().DeltaTime;
    bool const shouldUpdateSmoothed =
        s_profilerState.updateTimer >= ProfilerDisplayState::UPDATE_INTERVAL;

    if (shouldUpdateSmoothed && !s_profilerState.paused) {
        s_profilerState.updateTimer = 0.0f;

        // Smooth frame time
        f64 const currentFrameMs = frameData->getFrameDurationMs();
        s_profilerState.smoothedFrameTimeMs =
            s_profilerState.smoothedFrameTimeMs * (1.0 - ProfilerDisplayState::SMOOTHING_FACTOR) +
            currentFrameMs * ProfilerDisplayState::SMOOTHING_FACTOR;

        // Update smoothed samples
        s_profilerState.smoothedSamples.resize(frameData->samples.size());
        for (size_t i = 0; i < frameData->samples.size(); ++i) {
            const auto& sample = frameData->samples[i];
            auto& smoothed = s_profilerState.smoothedSamples[i];

            smoothed.name = sample.name;
            smoothed.depth = sample.depth;

            f64 const currentMs = sample.getDurationMs();
            if (smoothed.avgDurationMs == 0.0) {
                smoothed.avgDurationMs = currentMs;
            } else {
                smoothed.avgDurationMs =
                    smoothed.avgDurationMs * (1.0 - ProfilerDisplayState::SMOOTHING_FACTOR) +
                    currentMs * ProfilerDisplayState::SMOOTHING_FACTOR;
            }
        }
    }

    // Frame summary (use smoothed values)
    f64 const displayFrameTime = s_profilerState.smoothedFrameTimeMs > 0.0
                                     ? s_profilerState.smoothedFrameTimeMs
                                     : frameData->getFrameDurationMs();
    ImGui::Text("Frame Time: %.2f ms (%.0f FPS)", displayFrameTime,
                displayFrameTime > 0.0 ? 1000.0 / displayFrameTime : 0.0);
    ImGui::Text("Samples: %zu", frameData->samples.size());
    if (s_profilerState.paused) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "[PAUSED]");
    }

    ImGui::Separator();

    // Memory and thread stats
    if (ImGui::CollapsingHeader("System Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Frame allocator stats
        if (frame::isInitialized()) {
            auto& allocator = frame::get();
            f32 const usagePercent = allocator.getUsagePercent() * 100.0f;
            ImGui::Text("Frame Allocator: %.1f KB / %.1f KB (%.1f%%)",
                        static_cast<f32>(allocator.getUsedBytes()) / 1024.0f,
                        static_cast<f32>(allocator.getCapacity()) / 1024.0f, usagePercent);
            ImGui::ProgressBar(allocator.getUsagePercent(), ImVec2(-1, 0));
            ImGui::Text("Peak Usage: %.1f KB",
                        static_cast<f32>(allocator.getPeakUsage()) / 1024.0f);
        }

        ImGui::Spacing();

        // Thread pool stats
        if (ThreadPool::isInitialized()) {
            ImGui::Text("Thread Pool: %u workers, %zu pending jobs", ThreadPool::getWorkerCount(),
                        ThreadPool::getPendingJobCount());
        }
    }

    ImGui::Separator();

    // Sample hierarchy
    if (ImGui::CollapsingHeader("CPU Timeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Timeline visualization
        ImVec2 const canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 const canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 100.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Background
        drawList->AddRectFilled(canvasPos,
                                ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                                IM_COL32(30, 30, 30, 255));

        // Scale factor: pixels per microsecond
        f32 const frameTimeUs = static_cast<f32>(frameData->getFrameDurationMs() * 1000.0);
        f32 const pixelsPerUs = frameTimeUs > 0 ? canvasSize.x / frameTimeUs : 1.0f;

        // Frame start time in nanoseconds
        u64 const frameStartNs = frameData->frameStartTime;

        // Draw samples as bars
        for (size_t i = 0; i < frameData->samples.size(); ++i) {
            const auto& sample = frameData->samples[i];

            // Convert from nanoseconds to microseconds relative to frame start
            f32 const startUs = static_cast<f32>(sample.startTime - frameStartNs) / 1000.0f;
            f32 const durationUs = static_cast<f32>(sample.getDurationUs());

            f32 const startX = canvasPos.x + startUs * pixelsPerUs;
            f32 const width = durationUs * pixelsPerUs;
            f32 const barY = canvasPos.y + static_cast<f32>(sample.depth) * 20.0f;
            f32 constexpr barHeight = 18.0f;

            // Clamp to canvas
            if (barY + barHeight > canvasPos.y + canvasSize.y) {
                continue;
            }

            ImVec4 const color = getProfilerColor(i);
            ImU32 const colorU32 =
                IM_COL32(static_cast<u8>(color.x * 255), static_cast<u8>(color.y * 255),
                         static_cast<u8>(color.z * 255), 200);

            drawList->AddRectFilled(ImVec2(startX, barY),
                                    ImVec2(startX + std::max(width, 2.0f), barY + barHeight),
                                    colorU32);

            // Draw label if bar is wide enough
            if (width > 40.0f) {
                char label[64];
                snprintf(label, sizeof(label), "%s", sample.name);
                drawList->AddText(ImVec2(startX + 2.0f, barY + 2.0f), IM_COL32(255, 255, 255, 255),
                                  label);
            }

            // Tooltip on hover
            ImVec2 const mousePos = ImGui::GetMousePos();
            if (mousePos.x >= startX && mousePos.x <= startX + width && mousePos.y >= barY &&
                mousePos.y <= barY + barHeight) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", sample.name);
                ImGui::Text("Duration: %.3f ms", sample.getDurationMs());
                ImGui::Text("Depth: %u", sample.depth);
                ImGui::EndTooltip();
            }
        }

        // Reserve space for canvas
        ImGui::Dummy(canvasSize);
    }

    // Sample table (use smoothed values for stable display)
    if (ImGui::CollapsingHeader("Sample Details", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("SamplesTable", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY,
                              ImVec2(0, 200))) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            f64 const frameTotalMs = s_profilerState.smoothedFrameTimeMs > 0.0
                                         ? s_profilerState.smoothedFrameTimeMs
                                         : frameData->getFrameDurationMs();

            // Use smoothed samples if available, otherwise use raw data
            bool const useSmoothed =
                !s_profilerState.smoothedSamples.empty() &&
                s_profilerState.smoothedSamples.size() == frameData->samples.size();

            for (size_t i = 0; i < frameData->samples.size(); ++i) {
                const auto& sample = frameData->samples[i];

                f64 durationMs;
                if (useSmoothed) {
                    durationMs = s_profilerState.smoothedSamples[i].avgDurationMs;
                } else {
                    durationMs = sample.getDurationMs();
                }

                f64 const percent = frameTotalMs > 0.0 ? (durationMs / frameTotalMs) * 100.0 : 0.0;

                ImGui::TableNextRow();

                // Indent based on depth
                ImGui::TableSetColumnIndex(0);
                for (u32 d = 0; d < sample.depth; ++d) {
                    ImGui::Text("  ");
                    ImGui::SameLine(0, 0);
                }
                ImGui::Text("%s", sample.name);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", durationMs);

                ImGui::TableSetColumnIndex(2);
                // Color code by percentage
                if (percent > 50.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.1f%%", percent);
                } else if (percent > 25.0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%.1f%%", percent);
                } else {
                    ImGui::Text("%.1f%%", percent);
                }
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

// ============================================================================
// Entity Bounds Visualization
// ============================================================================

void drawEntityBounds(World& world, bool showTransformBounds, bool showColliderBounds,
                      const glm::vec4& boundsColor, const glm::vec4& colliderColor) {
    // Draw transform-based bounds for visual components
    if (showTransformBounds) {
        // Quad renderers
        auto quadView = world.view<TransformComponent, QuadRendererComponent>();
        for (auto entity : quadView) {
            const auto& transform = quadView.get<TransformComponent>(entity);
            const auto& quad = quadView.get<QuadRendererComponent>(entity);

            Renderer2D::drawRect(transform.position, quad.size, transform.rotation.z, boundsColor);
        }

        // Sprite renderers (assume 1x1 size unless we have scale info)
        auto spriteView = world.view<TransformComponent, SpriteRendererComponent>();
        for (auto entity : spriteView) {
            const auto& transform = spriteView.get<TransformComponent>(entity);

            // Use transform scale as sprite size
            glm::vec2 const size(transform.scale.x, transform.scale.y);
            Renderer2D::drawRect(transform.position, size, transform.rotation.z, boundsColor);
        }

        // Circle renderers
        auto circleView = world.view<TransformComponent, CircleRendererComponent>();
        for (auto entity : circleView) {
            const auto& transform = circleView.get<TransformComponent>(entity);
            const auto& circle = circleView.get<CircleRendererComponent>(entity);

            Renderer2D::drawCircle(glm::vec2(transform.position), circle.radius, boundsColor,
                                   circle.segments);
        }
    }

    // Draw collider bounds
    if (showColliderBounds) {
        // Box colliders
        auto boxView = world.view<TransformComponent, BoxCollider2DComponent>();
        for (auto entity : boxView) {
            const auto& transform = boxView.get<TransformComponent>(entity);
            const auto& collider = boxView.get<BoxCollider2DComponent>(entity);

            glm::vec3 const pos = transform.position + glm::vec3(collider.offset, 0.0f);
            // BoxCollider2D uses half-extents, so multiply by 2 for full size
            glm::vec2 const size = collider.size * 2.0f;

            Renderer2D::drawRect(pos, size, transform.rotation.z, colliderColor);
        }

        // Circle colliders
        auto circleColliderView = world.view<TransformComponent, CircleCollider2DComponent>();
        for (auto entity : circleColliderView) {
            const auto& transform = circleColliderView.get<TransformComponent>(entity);
            const auto& collider = circleColliderView.get<CircleCollider2DComponent>(entity);

            glm::vec2 const center = glm::vec2(transform.position) + collider.offset;

            Renderer2D::drawCircle(center, collider.radius, colliderColor);
        }
    }
}

}  // namespace DebugPanels
}  // namespace limbo
