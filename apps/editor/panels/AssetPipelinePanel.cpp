#include "AssetPipelinePanel.hpp"

#include "../EditorApp.hpp"
#include "limbo/debug/Log.hpp"

#include <imgui.h>

#include <fstream>

namespace limbo::editor {

AssetPipelinePanel::AssetPipelinePanel(EditorApp& editor) : m_editor(editor) {}

void AssetPipelinePanel::init() {
    m_projectRoot = std::filesystem::current_path();

    // Initialize registry
    m_registry.init(m_projectRoot, m_sourceDir, m_importedDir);
    m_registry.load();

    // Initialize importer
    m_importer.init(m_registry);

    // Setup hot reload manager
    m_hotReloadManager.setReloadHandler([this](AssetId id) {
        LIMBO_LOG_ASSET_INFO("Hot reload triggered for asset: {}", id.toString());
        // In a real scenario, this would reload the actual asset
        return true;
    });

    m_hotReloadManager.setAfterReloadCallback([this](const ReloadEvent& event) {
        m_reloadHistory.push_back(event);
        if (m_reloadHistory.size() > m_maxHistorySize) {
            m_reloadHistory.erase(m_reloadHistory.begin());
        }
    });

    m_initialized = true;
    LIMBO_LOG_EDITOR_INFO("AssetPipelinePanel initialized");
}

void AssetPipelinePanel::shutdown() {
    m_hotReloadManager.unwatchAll();
    m_registry.save();
}

void AssetPipelinePanel::update(f32 deltaTime) {
    if (m_hotReloadEnabled) {
        m_hotReloadManager.poll();
    }
}

void AssetPipelinePanel::render() {
    if (!m_open) {
        return;
    }

    ImGuiWindowFlags const windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Asset Pipeline", &m_open, windowFlags);

    if (!m_initialized) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Not initialized!");
        ImGui::End();
        return;
    }

    // Tab bar for different views
    if (ImGui::BeginTabBar("AssetPipelineTabs")) {
        if (ImGui::BeginTabItem("Registry")) {
            renderRegistryTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Import")) {
            renderImportTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hot Reload")) {
            renderHotReloadTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Dependencies")) {
            renderDependencyTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void AssetPipelinePanel::renderRegistryTab() {
    // Registry info
    ImGui::Text("Project Root: %s", m_projectRoot.string().c_str());
    ImGui::Text("Source Dir: %s", m_registry.getSourceDir().string().c_str());
    ImGui::Text("Imported Dir: %s", m_registry.getImportedDir().string().c_str());

    ImGui::Separator();

    // Actions
    if (ImGui::Button("Scan Assets")) {
        scanAssets();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Registry")) {
        if (m_registry.save()) {
            LIMBO_LOG_ASSET_INFO("Registry saved");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Registry")) {
        if (m_registry.load()) {
            LIMBO_LOG_ASSET_INFO("Registry reloaded");
        }
    }

    ImGui::Separator();

    // Scan results
    if (m_hasScanned) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "New: %zu", m_newAssets.size());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Modified: %zu",
                           m_modifiedAssets.size());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "Deleted: %zu", m_deletedAssets.size());
        ImGui::Separator();
    }

    // Asset list
    std::vector<AssetId> allAssets = m_registry.getAllAssetIds();
    ImGui::Text("Total Assets: %zu", allAssets.size());

    if (ImGui::BeginChild("AssetList", ImVec2(0, 0), true)) {
        ImGui::Columns(4, "AssetColumns");
        ImGui::SetColumnWidth(0, 200);
        ImGui::SetColumnWidth(1, 100);
        ImGui::SetColumnWidth(2, 150);
        ImGui::SetColumnWidth(3, 200);

        ImGui::Text("Path");
        ImGui::NextColumn();
        ImGui::Text("Type");
        ImGui::NextColumn();
        ImGui::Text("Status");
        ImGui::NextColumn();
        ImGui::Text("ID");
        ImGui::NextColumn();
        ImGui::Separator();

        for (AssetId id : allAssets) {
            const AssetMetadata* meta = m_registry.getMetadata(id);
            if (!meta) {
                continue;
            }

            // Path
            ImGui::Text("%s", meta->sourcePath.c_str());
            ImGui::NextColumn();

            // Type with color
            ImVec4 typeColor = assetTypeToColor(meta->type);
            ImGui::TextColored(typeColor, "%s", assetTypeToString(meta->type).c_str());
            ImGui::NextColumn();

            // Status
            if (meta->importedPath.empty()) {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Needs Import");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Imported");
            }
            ImGui::NextColumn();

            // ID (truncated)
            String idStr = id.toString();
            if (idStr.length() > 16) {
                idStr = idStr.substr(0, 16) + "...";
            }
            ImGui::Text("%s", idStr.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", id.toString().c_str());
            }
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }
    ImGui::EndChild();
}

void AssetPipelinePanel::renderImportTab() {
    // Import actions
    if (ImGui::Button("Import All")) {
        importAllAssets();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rebuild All")) {
        rebuildAllAssets();
    }

    ImGui::Separator();

    // Import progress
    if (m_importing) {
        ImGui::Text("Importing: %s", m_importCurrentAsset.c_str());
        float progress = m_importTotal > 0 ? static_cast<float>(m_importCurrent) /
                                                 static_cast<float>(m_importTotal)
                                           : 0.0f;
        ImGui::ProgressBar(
            progress, ImVec2(-1, 0),
            (std::to_string(m_importCurrent) + "/" + std::to_string(m_importTotal)).c_str());
    }

    ImGui::Separator();

    // Assets needing import
    std::vector<AssetId> needsImport = m_registry.getAssetsNeedingReimport();
    ImGui::Text("Assets Needing Import: %zu", needsImport.size());

    if (ImGui::BeginChild("ImportList", ImVec2(0, 200), true)) {
        for (AssetId id : needsImport) {
            const AssetMetadata* meta = m_registry.getMetadata(id);
            if (meta) {
                ImGui::BulletText("%s", meta->sourcePath.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Import settings info
    ImGui::Text("Registered Importers:");
    ImGui::BulletText("Textures: .png, .jpg, .jpeg, .bmp, .tga, .gif");
    ImGui::BulletText("Shaders: .glsl, .vert, .frag, .vs, .fs, .shader");
    ImGui::BulletText("Audio: .wav, .mp3, .ogg, .flac");
    ImGui::BulletText("Sprite Atlas: .atlas.json");
}

void AssetPipelinePanel::renderHotReloadTab() {
    // Hot reload toggle
    if (ImGui::Checkbox("Enable Hot Reload", &m_hotReloadEnabled)) {
        m_hotReloadManager.setEnabled(m_hotReloadEnabled);
        if (m_hotReloadEnabled) {
            // Watch all registered assets
            for (AssetId id : m_registry.getAllAssetIds()) {
                std::filesystem::path path = m_registry.getSourcePath(id);
                if (std::filesystem::exists(path)) {
                    m_hotReloadManager.watchAsset(id, path);
                }
            }
            LIMBO_LOG_ASSET_INFO("Hot reload enabled - watching {} assets",
                                 m_registry.getAllAssetIds().size());
        } else {
            m_hotReloadManager.unwatchAll();
            LIMBO_LOG_ASSET_INFO("Hot reload disabled");
        }
    }

    ImGui::Separator();

    // Statistics
    ImGui::Text("Total Reloads: %u", m_hotReloadManager.getTotalReloads());
    ImGui::SameLine();
    ImGui::Text("Failed: %u", m_hotReloadManager.getFailedReloads());
    ImGui::SameLine();
    if (ImGui::Button("Reset Stats")) {
        m_hotReloadManager.resetStats();
    }

    ImGui::Separator();

    // Test asset creation
    ImGui::Text("Hot Reload Test:");
    if (!m_testAssetCreated) {
        if (ImGui::Button("Create Test Asset")) {
            createTestAsset();
        }
    } else {
        ImGui::Text("Test asset: %s", m_testAssetPath.string().c_str());
        if (ImGui::Button("Modify Test Asset")) {
            modifyTestAsset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Test Asset")) {
            if (std::filesystem::exists(m_testAssetPath)) {
                std::filesystem::remove(m_testAssetPath);
                m_testAssetCreated = false;
                LIMBO_LOG_ASSET_INFO("Test asset deleted");
            }
        }
    }

    ImGui::Separator();

    // Reload history
    ImGui::Text("Reload History:");
    if (ImGui::BeginChild("ReloadHistory", ImVec2(0, 200), true)) {
        for (auto it = m_reloadHistory.rbegin(); it != m_reloadHistory.rend(); ++it) {
            const ReloadEvent& event = *it;
            ImVec4 color =
                event.success ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            String status = event.success ? "OK" : "FAIL";

            ImGui::TextColored(color, "[%s] %s", status.c_str(),
                               event.path.filename().string().c_str());
            if (ImGui::IsItemHovered() && !event.error.empty()) {
                ImGui::SetTooltip("%s", event.error.c_str());
            }
        }
    }
    ImGui::EndChild();
}

void AssetPipelinePanel::renderDependencyTab() {
    ImGui::Text("Asset Dependencies");
    ImGui::Separator();

    // Show dependency graph info
    std::vector<AssetId> allAssets = m_registry.getAllAssetIds();

    if (ImGui::BeginChild("DependencyList", ImVec2(0, 0), true)) {
        for (AssetId id : allAssets) {
            const AssetMetadata* meta = m_registry.getMetadata(id);
            if (!meta) {
                continue;
            }

            bool hasChildren = !meta->dependencies.empty() || !meta->dependents.empty();

            if (ImGui::TreeNode(meta->sourcePath.c_str())) {
                // Dependencies (what this asset depends on)
                if (!meta->dependencies.empty()) {
                    ImGui::Text("Depends on:");
                    for (AssetId depId : meta->dependencies) {
                        const AssetMetadata* depMeta = m_registry.getMetadata(depId);
                        if (depMeta) {
                            ImGui::BulletText("%s", depMeta->sourcePath.c_str());
                        }
                    }
                }

                // Dependents (what depends on this asset)
                if (!meta->dependents.empty()) {
                    ImGui::Text("Depended on by:");
                    for (AssetId depId : meta->dependents) {
                        const AssetMetadata* depMeta = m_registry.getMetadata(depId);
                        if (depMeta) {
                            ImGui::BulletText("%s", depMeta->sourcePath.c_str());
                        }
                    }
                }

                if (!hasChildren) {
                    ImGui::TextDisabled("No dependencies");
                }

                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();
}

void AssetPipelinePanel::scanAssets() {
    usize changes = m_registry.scanSourceDirectory();

    m_newAssets.clear();
    m_modifiedAssets.clear();
    m_deletedAssets.clear();

    for (const auto& path : m_registry.getNewAssets()) {
        m_newAssets.push_back(path);
    }
    for (AssetId id : m_registry.getModifiedAssets()) {
        m_modifiedAssets.push_back(id);
    }
    for (AssetId id : m_registry.getDeletedAssets()) {
        m_deletedAssets.push_back(id);
    }

    // Auto-register new assets
    for (const auto& path : m_newAssets) {
        std::filesystem::path fullPath = m_registry.getSourceDir() / path;
        AssetType type = AssetType::Unknown;

        String ext = fullPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
            type = AssetType::Texture;
        } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
            type = AssetType::Shader;
        } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
            type = AssetType::Audio;
        } else if (fullPath.string().ends_with(".atlas.json")) {
            type = AssetType::SpriteAtlas;
        }

        if (type != AssetType::Unknown) {
            m_registry.registerAsset(path, type);
        }
    }

    // Handle deleted assets
    for (AssetId id : m_deletedAssets) {
        m_registry.unregisterAsset(id);
    }

    m_registry.save();
    m_hasScanned = true;

    LIMBO_LOG_ASSET_INFO("Scan complete: {} new, {} modified, {} deleted", m_newAssets.size(),
                         m_modifiedAssets.size(), m_deletedAssets.size());
}

void AssetPipelinePanel::importAllAssets() {
    std::vector<AssetId> needsImport = m_registry.getAssetsNeedingReimport();
    m_importTotal = needsImport.size();
    m_importCurrent = 0;
    m_importing = true;

    m_importer.setProgressCallback([this](usize current, usize total, const String& path) {
        m_importCurrent = current;
        m_importTotal = total;
        m_importCurrentAsset = path;
    });

    usize imported = m_importer.importAll();

    m_importing = false;
    m_importCurrentAsset.clear();

    LIMBO_LOG_ASSET_INFO("Import complete: {} assets imported", imported);
}

void AssetPipelinePanel::rebuildAllAssets() {
    // Force reimport by clearing all source hashes
    std::vector<AssetId> allAssets = m_registry.getAllAssetIds();
    for (AssetId id : allAssets) {
        m_registry.updateSourceHash(id, 0);
    }

    importAllAssets();
}

void AssetPipelinePanel::createTestAsset() {
    m_testAssetPath = m_registry.getSourceDir() / "test_hot_reload.json";

    std::ofstream file(m_testAssetPath);
    if (file.is_open()) {
        file << "{\n  \"version\": 1,\n  \"message\": \"Initial content\"\n}\n";
        file.close();

        // Register with registry
        AssetId id = m_registry.registerAsset("test_hot_reload.json", AssetType::Unknown);
        m_registry.save();

        // Watch for hot reload
        if (m_hotReloadEnabled) {
            m_hotReloadManager.watchAsset(id, m_testAssetPath);
        }

        m_testAssetCreated = true;
        LIMBO_LOG_ASSET_INFO("Test asset created: {}", m_testAssetPath.string());
    } else {
        LIMBO_LOG_ASSET_ERROR("Failed to create test asset");
    }
}

void AssetPipelinePanel::modifyTestAsset() {
    if (!std::filesystem::exists(m_testAssetPath)) {
        LIMBO_LOG_ASSET_WARN("Test asset does not exist");
        return;
    }

    // Modify the file
    static int modifyCount = 1;
    std::ofstream file(m_testAssetPath);
    if (file.is_open()) {
        file << "{\n  \"version\": " << modifyCount << ",\n  \"message\": \"Modified content #"
             << modifyCount << "\"\n}\n";
        file.close();
        modifyCount++;

        LIMBO_LOG_ASSET_INFO("Test asset modified (version {})", modifyCount - 1);
    }
}

String AssetPipelinePanel::assetTypeToString(AssetType type) const {
    switch (type) {
    case AssetType::Texture:
        return "Texture";
    case AssetType::SpriteAtlas:
        return "SpriteAtlas";
    case AssetType::Shader:
        return "Shader";
    case AssetType::Audio:
        return "Audio";
    default:
        return "Unknown";
    }
}

ImVec4 AssetPipelinePanel::assetTypeToColor(AssetType type) const {
    switch (type) {
    case AssetType::Texture:
        return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);  // Green
    case AssetType::SpriteAtlas:
        return ImVec4(0.3f, 0.8f, 0.8f, 1.0f);  // Cyan
    case AssetType::Shader:
        return ImVec4(0.9f, 0.5f, 0.3f, 1.0f);  // Orange
    case AssetType::Audio:
        return ImVec4(0.9f, 0.3f, 0.6f, 1.0f);  // Pink
    default:
        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
    }
}

}  // namespace limbo::editor
