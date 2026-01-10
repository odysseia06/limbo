#include "AssetBrowserPanel.hpp"
#include "EditorApp.hpp"

#include <limbo/debug/Log.hpp>

#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace limbo::editor {

AssetBrowserPanel::AssetBrowserPanel(EditorApp& editor) : m_editor(editor) {}

void AssetBrowserPanel::init() {
    m_baseDirectory = std::filesystem::current_path() / "assets";
    m_currentDirectory = m_baseDirectory;

    // Create assets directory if it doesn't exist
    if (!std::filesystem::exists(m_baseDirectory)) {
        std::filesystem::create_directories(m_baseDirectory);
    }

    m_needsRefresh = true;
}

void AssetBrowserPanel::shutdown() {}

void AssetBrowserPanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::Begin("Asset Browser", &m_open);

    drawToolbar();
    ImGui::Separator();

    // Refresh if needed
    if (m_needsRefresh) {
        refreshDirectory();
        m_needsRefresh = false;
    }

    // Asset grid
    drawAssetGrid();

    ImGui::End();
}

void AssetBrowserPanel::drawToolbar() {
    // Back button
    if (m_currentDirectory != m_baseDirectory) {
        if (ImGui::Button("<-")) {
            m_currentDirectory = m_currentDirectory.parent_path();
            m_needsRefresh = true;
        }
        ImGui::SameLine();
    }

    // Home button
    if (ImGui::Button("Home")) {
        m_currentDirectory = m_baseDirectory;
        m_needsRefresh = true;
    }
    ImGui::SameLine();

    // Refresh button
    if (ImGui::Button("Refresh")) {
        m_needsRefresh = true;
    }
    ImGui::SameLine();

    // Search box
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_searchBuffer,
                                 sizeof(m_searchBuffer))) {
        m_searchFilter = m_searchBuffer;
        // Convert to lowercase for case-insensitive search
        std::transform(m_searchFilter.begin(), m_searchFilter.end(), m_searchFilter.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    // Clear search button
    if (!m_searchFilter.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("X##ClearSearch")) {
            m_searchBuffer[0] = '\0';
            m_searchFilter.clear();
        }
    }

    ImGui::SameLine();

    // Thumbnail size slider
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("##Size", &m_thumbnailSize, 32.0f, 128.0f, "%.0f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Thumbnail Size");
    }

    // Current path display
    ImGui::Text("Path: %s", m_currentDirectory.string().c_str());
}

void AssetBrowserPanel::drawDirectoryTree(const std::filesystem::path& /*path*/) {
    // TODO: Implement directory tree view in side panel
}

void AssetBrowserPanel::drawAssetGrid() {
    float const panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / (m_thumbnailSize + m_padding));
    if (columnCount < 1) {
        columnCount = 1;
    }

    ImGui::Columns(columnCount, nullptr, false);

    if (!std::filesystem::exists(m_currentDirectory)) {
        ImGui::Text("Directory not found");
        ImGui::Columns(1);
        return;
    }

    int visibleCount = 0;
    for (const auto& entry : m_entries) {
        // Apply filter
        if (!matchesFilter(entry.filename)) {
            continue;
        }

        visibleCount++;
        ImGui::PushID(entry.filename.c_str());

        const char* icon = getAssetIcon(entry.path, entry.isDirectory);
        glm::vec4 const colorVec = getAssetColor(entry.path, entry.isDirectory);
        ImVec4 const color(colorVec.r, colorVec.g, colorVec.b, colorVec.a);

        // Draw button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(icon, ImVec2(m_thumbnailSize, m_thumbnailSize));

        // Double click to open
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (entry.isDirectory) {
                m_currentDirectory = entry.path;
                m_needsRefresh = true;
            } else {
                // Handle different asset types
                String const ext = entry.path.extension().string();
                if (ext == ".json") {
                    // Could be a scene or prefab
                    LIMBO_LOG_EDITOR_INFO("Opening asset: {}", entry.path.string());
                } else {
                    LIMBO_LOG_EDITOR_INFO("Asset selected: {}", entry.path.string());
                }
            }
        }

        // Drag source for drag-drop
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            String const pathStr = entry.path.string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);

            // Preview
            ImGui::Text("%s %s", icon, entry.filename.c_str());
            ImGui::EndDragDropSource();
        }

        // Tooltip with full path
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", entry.path.string().c_str());
            ImGui::EndTooltip();
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Open")) {
                if (entry.isDirectory) {
                    m_currentDirectory = entry.path;
                    m_needsRefresh = true;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                // TODO: Confirm and delete
            }
            if (ImGui::MenuItem("Rename")) {
                // TODO: Rename dialog
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show in Explorer")) {
                // TODO: Open file explorer
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);

        // Filename (truncated if too long)
        String displayName = entry.filename;
        float const maxTextWidth = m_thumbnailSize + m_padding;
        ImVec2 const textSize = ImGui::CalcTextSize(displayName.c_str());
        if (textSize.x > maxTextWidth) {
            // Truncate with ellipsis
            while (displayName.length() > 3 &&
                   ImGui::CalcTextSize((displayName + "...").c_str()).x > maxTextWidth) {
                displayName.pop_back();
            }
            displayName += "...";
        }

        ImGui::TextColored(color, "%s", displayName.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    if (visibleCount == 0 && !m_searchFilter.empty()) {
        ImGui::Columns(1);
        ImGui::TextDisabled("No assets match '%s'", m_searchBuffer);
    }

    ImGui::Columns(1);
}

void AssetBrowserPanel::refreshDirectory() {
    m_entries.clear();

    if (!std::filesystem::exists(m_currentDirectory)) {
        return;
    }

    // Collect entries
    for (const auto& entry : std::filesystem::directory_iterator(m_currentDirectory)) {
        const auto& path = entry.path();
        String const filename = path.filename().string();

        // Skip hidden files unless enabled
        if (!m_showHiddenFiles && !filename.empty() && filename[0] == '.') {
            continue;
        }

        AssetEntry assetEntry;
        assetEntry.path = path;
        assetEntry.filename = filename;
        assetEntry.isDirectory = entry.is_directory();

        m_entries.push_back(assetEntry);
    }

    // Sort: directories first, then alphabetically
    std::sort(m_entries.begin(), m_entries.end(), [](const AssetEntry& a, const AssetEntry& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory;  // Directories first
        }
        return a.filename < b.filename;  // Alphabetical
    });
}

bool AssetBrowserPanel::matchesFilter(const std::string& filename) const {
    if (m_searchFilter.empty()) {
        return true;
    }

    // Case-insensitive search
    String lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lowerFilename.find(m_searchFilter) != String::npos;
}

const char* AssetBrowserPanel::getAssetIcon(const std::filesystem::path& path,
                                            bool isDirectory) const {
    if (isDirectory) {
        return "[D]";
    }

    String const ext = path.extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return "[I]";  // Image
    }
    if (ext == ".json") {
        return "[J]";  // JSON (scene/prefab/config)
    }
    if (ext == ".lua") {
        return "[L]";  // Lua script
    }
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
        return "[S]";  // Shader
    }
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
        return "[A]";  // Audio
    }
    if (ext == ".ttf" || ext == ".otf") {
        return "[F]";  // Font
    }
    if (ext == ".prefab") {
        return "[P]";  // Prefab
    }

    return "[?]";
}

glm::vec4 AssetBrowserPanel::getAssetColor(const std::filesystem::path& path,
                                           bool isDirectory) const {
    if (isDirectory) {
        return glm::vec4(0.9f, 0.8f, 0.3f, 1.0f);  // Yellow
    }

    String const ext = path.extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        return glm::vec4(0.3f, 0.8f, 0.3f, 1.0f);  // Green
    }
    if (ext == ".json") {
        return glm::vec4(0.3f, 0.6f, 0.9f, 1.0f);  // Blue
    }
    if (ext == ".lua") {
        return glm::vec4(0.3f, 0.3f, 0.9f, 1.0f);  // Dark blue
    }
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
        return glm::vec4(0.9f, 0.5f, 0.3f, 1.0f);  // Orange
    }
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
        return glm::vec4(0.9f, 0.3f, 0.6f, 1.0f);  // Pink
    }
    if (ext == ".ttf" || ext == ".otf") {
        return glm::vec4(0.7f, 0.7f, 0.9f, 1.0f);  // Light purple
    }
    if (ext == ".prefab") {
        return glm::vec4(0.5f, 0.9f, 0.9f, 1.0f);  // Cyan
    }

    return glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);  // Gray
}

}  // namespace limbo::editor
