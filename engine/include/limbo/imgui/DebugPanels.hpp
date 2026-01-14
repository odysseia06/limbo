#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>

namespace limbo {

// Forward declarations
class World;
class AssetManager;
class GPUTimer;

/**
 * DebugPanels - Collection of debug UI panels for the engine
 *
 * These are standalone functions that can be called within an ImGui frame
 * to render various debug information panels.
 */
namespace DebugPanels {

/**
 * Render statistics panel (FPS, frame time, renderer stats)
 * @param deltaTime Current frame delta time
 * @param gpuTimer Optional GPU timer for GPU timing display (can be nullptr)
 */
LIMBO_API void showStatsPanel(f32 deltaTime, GPUTimer* gpuTimer = nullptr);

/**
 * Render entity inspector panel
 * Shows all entities and their components
 * @param world The ECS world to inspect
 */
LIMBO_API void showEntityInspector(World& world);

/**
 * Render asset browser panel
 * Shows loaded assets and their status
 * @param assetManager The asset manager to inspect
 */
LIMBO_API void showAssetBrowser(AssetManager& assetManager);

/**
 * Render the ImGui demo window
 * Useful for exploring ImGui features
 */
LIMBO_API void showDemoWindow();

/**
 * Render a simple log console
 * Displays recent log messages
 */
LIMBO_API void showLogConsole();

/**
 * Render the profiler panel
 * Shows hierarchical CPU timing breakdown and frame history
 */
LIMBO_API void showProfilerPanel();

/**
 * Draw debug bounds for all entities with visual components
 * Call this within a Renderer2D scene (between beginScene/endScene)
 * @param world The ECS world to visualize
 * @param showTransformBounds Show transform-based bounds (default true)
 * @param showColliderBounds Show physics collider bounds (default true)
 * @param boundsColor Color for transform bounds
 * @param colliderColor Color for collider bounds
 */
LIMBO_API void drawEntityBounds(World& world, bool showTransformBounds = true,
                                bool showColliderBounds = true,
                                const glm::vec4& boundsColor = glm::vec4(0.0f, 1.0f, 0.0f, 0.5f),
                                const glm::vec4& colliderColor = glm::vec4(0.0f, 0.5f, 1.0f, 0.5f));

}  // namespace DebugPanels

}  // namespace limbo
