#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

namespace limbo {

// Forward declarations
class World;
class AssetManager;

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
 */
LIMBO_API void showStatsPanel(f32 deltaTime);

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

}  // namespace DebugPanels

}  // namespace limbo
