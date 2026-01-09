#include "AssetBrowserPanel.hpp"
#include "EditorApp.hpp"

#include <imgui.h>
#include <cstdio>

namespace limbo::editor {

AssetBrowserPanel::AssetBrowserPanel(EditorApp& editor) : m_editor(editor) {}

void AssetBrowserPanel::init() {
    m_baseDirectory = std::filesystem::current_path() / "assets";
    m_currentDirectory = m_baseDirectory;

    // Create assets directory if it doesn't exist
    if (!std::filesystem::exists(m_baseDirectory)) {
        std::filesystem::create_directories(m_baseDirectory);
    }
}

void AssetBrowserPanel::shutdown() {}

void AssetBrowserPanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::Begin("Asset Browser", &m_open);

    // Back button
    if (m_currentDirectory != m_baseDirectory) {
        if (ImGui::Button("<-")) {
            m_currentDirectory = m_currentDirectory.parent_path();
        }
        ImGui::SameLine();
    }

    // Current path display
    ImGui::Text("%s", m_currentDirectory.string().c_str());
    ImGui::Separator();

    // Settings
    ImGui::SliderFloat("Thumbnail Size", &m_thumbnailSize, 32.0f, 128.0f);
    ImGui::Separator();

    // Asset grid
    drawAssetGrid();

    // Rename dialog
    if (m_openRenamePopup) {
        ImGui::OpenPopup("Rename Asset");
        m_openRenamePopup = false;
    }

    if (ImGui::BeginPopupModal("Rename Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        bool triggerRename = ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                                              ImGuiInputTextFlags_EnterReturnsTrue);

        bool buttonRename = ImGui::Button("Rename");
        if (triggerRename || buttonRename) {
            if (m_renameBuffer[0] != '\0') {
                std::filesystem::path newPath = m_itemToRename.parent_path() / m_renameBuffer;

                if (std::filesystem::exists(newPath)) {
                    m_renameErrorMessage = "Name already exists!";
                } else {
                    std::error_code ec;
                    std::filesystem::rename(m_itemToRename, newPath, ec);

                    if (ec) {
                        m_renameErrorMessage = "Error: " + ec.message();
                        spdlog::error("Failed to rename asset: {}", ec.message());
                    } else {
                        m_renameErrorMessage.clear();
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_renameErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }

        if (!m_renameErrorMessage.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", m_renameErrorMessage.c_str());
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void AssetBrowserPanel::drawDirectoryTree(const std::filesystem::path& path) {
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

    for (auto& entry : std::filesystem::directory_iterator(m_currentDirectory)) {
        const auto& path = entry.path();
        std::string const filename = path.filename().string();

        ImGui::PushID(filename.c_str());

        // Determine icon/color based on type
        ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        const char* icon = "[?]";

        if (entry.is_directory()) {
            color = ImVec4(0.9f, 0.8f, 0.3f, 1.0f);
            icon = "[D]";
        } else {
            std::string const ext = path.extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
                icon = "[I]";
            } else if (ext == ".json") {
                color = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
                icon = "[J]";
            } else if (ext == ".lua") {
                color = ImVec4(0.3f, 0.3f, 0.9f, 1.0f);
                icon = "[L]";
            } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag") {
                color = ImVec4(0.9f, 0.5f, 0.3f, 1.0f);
                icon = "[S]";
            } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
                color = ImVec4(0.9f, 0.3f, 0.6f, 1.0f);
                icon = "[A]";
            }
        }

        // Draw button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        ImGui::Button(icon, ImVec2(m_thumbnailSize, m_thumbnailSize));

        // Double click to open
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (entry.is_directory()) {
                m_currentDirectory = path;
            } else {
                // TODO: Open/import asset
                spdlog::info("Asset selected: {}", path.string());
            }
        }

        // Drag source for drag-drop
        if (ImGui::BeginDragDropSource()) {
            std::string const pathStr = path.string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s", filename.c_str());
            ImGui::EndDragDropSource();
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                // TODO: Confirm and delete
            }
            if (ImGui::MenuItem("Rename")) {
                m_itemToRename = path;
                snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s", filename.c_str());
                m_openRenamePopup = true;
            }
            if (ImGui::MenuItem("Show in Explorer")) {
                // TODO: Open file explorer
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);

        // Filename
        ImGui::TextColored(color, "%s", filename.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
}

void AssetBrowserPanel::refreshDirectory() {
    // Re-scan directory
}

}  // namespace limbo::editor
