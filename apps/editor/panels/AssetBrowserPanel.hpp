#pragma once

#include <limbo/Limbo.hpp>
#include <filesystem>

namespace limbo::editor {

class EditorApp;

/**
 * AssetBrowserPanel - Browse and manage project assets
 */
class AssetBrowserPanel {
public:
    explicit AssetBrowserPanel(EditorApp& editor);

    void init();
    void shutdown();
    void render();

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawDirectoryTree(const std::filesystem::path& path);
    void drawAssetGrid();
    void refreshDirectory();

private:
    EditorApp& m_editor;
    bool m_open = true;

    std::filesystem::path m_baseDirectory;
    std::filesystem::path m_currentDirectory;

    // Icons would be loaded as textures
    float m_thumbnailSize = 64.0f;
    float m_padding = 8.0f;
};

} // namespace limbo::editor
