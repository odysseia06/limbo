#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/assets/AssetRegistry.hpp>
#include <limbo/assets/AssetImporter.hpp>
#include <limbo/assets/HotReloadManager.hpp>

#include <imgui.h>

#include <filesystem>
#include <vector>

namespace limbo::editor {

class EditorApp;

/**
 * AssetPipelinePanel - Test and manage the asset pipeline
 *
 * Provides UI for:
 * - Viewing asset registry status
 * - Scanning for new/modified/deleted assets
 * - Importing assets
 * - Testing hot reload
 * - Viewing dependency graph
 */
class AssetPipelinePanel {
public:
    explicit AssetPipelinePanel(EditorApp& editor);

    void init();
    void shutdown();
    void update(f32 deltaTime);
    void render();

    [[nodiscard]] bool& isOpen() { return m_open; }

    // Access for EditorApp
    [[nodiscard]] AssetRegistry& getRegistry() { return m_registry; }
    [[nodiscard]] AssetImporterManager& getImporter() { return m_importer; }
    [[nodiscard]] HotReloadManager& getHotReloadManager() { return m_hotReloadManager; }

private:
    void renderRegistryTab();
    void renderImportTab();
    void renderHotReloadTab();
    void renderDependencyTab();

    void scanAssets();
    void importAllAssets();
    void rebuildAllAssets();

    // Hot reload test helpers
    void createTestAsset();
    void modifyTestAsset();

    String assetTypeToString(AssetType type) const;
    ImVec4 assetTypeToColor(AssetType type) const;

private:
    EditorApp& m_editor;
    bool m_open = true;

    // Asset pipeline components
    AssetRegistry m_registry;
    AssetImporterManager m_importer;
    HotReloadManager m_hotReloadManager;

    // State
    bool m_initialized = false;
    std::filesystem::path m_projectRoot;
    String m_sourceDir = "assets";
    String m_importedDir = "build/imported";

    // Scan results
    bool m_hasScanned = false;
    std::vector<String> m_newAssets;
    std::vector<AssetId> m_modifiedAssets;
    std::vector<AssetId> m_deletedAssets;

    // Hot reload state
    bool m_hotReloadEnabled = false;
    std::vector<ReloadEvent> m_reloadHistory;
    u32 m_maxHistorySize = 50;

    // Import progress
    bool m_importing = false;
    usize m_importCurrent = 0;
    usize m_importTotal = 0;
    String m_importCurrentAsset;

    // Test asset for hot reload demo
    std::filesystem::path m_testAssetPath;
    bool m_testAssetCreated = false;
};

}  // namespace limbo::editor
