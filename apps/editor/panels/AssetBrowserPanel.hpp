#pragma once

#include <limbo/Limbo.hpp>
#include <filesystem>
#include <vector>

namespace limbo::editor {

class EditorApp;

/**
 * AssetBrowserPanel - Browse and manage project assets
 *
 * Features:
 * - Grid view with thumbnails
 * - Search/filter by name
 * - Drag-drop assets to scene (sprites, prefabs)
 * - Directory navigation
 */
class AssetBrowserPanel {
public:
    explicit AssetBrowserPanel(EditorApp& editor);

    void init();
    void shutdown();
    void render();

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawToolbar();
    void drawDirectoryTree(const std::filesystem::path& path);
    void drawAssetGrid();
    void refreshDirectory();
    [[nodiscard]] bool matchesFilter(const std::string& filename) const;
    [[nodiscard]] const char* getAssetIcon(const std::filesystem::path& path,
                                           bool isDirectory) const;
    [[nodiscard]] glm::vec4 getAssetColor(const std::filesystem::path& path,
                                          bool isDirectory) const;

private:
    EditorApp& m_editor;
    bool m_open = true;

    std::filesystem::path m_baseDirectory;
    std::filesystem::path m_currentDirectory;

    // Search/filter
    char m_searchBuffer[256] = {0};
    String m_searchFilter;

    // Cached directory entries (refreshed on navigation)
    struct AssetEntry {
        std::filesystem::path path;
        String filename;
        bool isDirectory = false;
    };
    std::vector<AssetEntry> m_entries;
    bool m_needsRefresh = true;

    // View settings
    float m_thumbnailSize = 64.0f;
    float m_padding = 8.0f;
    bool m_showHiddenFiles = false;
};

}  // namespace limbo::editor
