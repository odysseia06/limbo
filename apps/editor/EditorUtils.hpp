#pragma once

#include <filesystem>
#include <string>

namespace limbo::editor {

/**
 * Sanitize user input to a safe scene filename.
 * Strips directory components (prevents path traversal), rejects invalid names,
 * and auto-appends .json extension if missing.
 * @return Sanitized filename, or empty string if input is invalid.
 */
inline std::string sanitizeSceneFilename(const std::string& input) {
    // Strip path components (directory separators, ..)
    std::string filename = std::filesystem::path(input).filename().string();

    // Reject empty, ".", and ".."
    if (filename.empty() || filename == "." || filename == "..") {
        return {};
    }

    // Auto-append .json extension if missing
    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json") {
        filename += ".json";
    }

    return filename;
}

/**
 * Build the editor window title string from scene state.
 * @param scenePath Current scene file path (empty for unsaved scenes).
 * @param modified Whether the scene has unsaved changes.
 * @return Title string like "Limbo Editor - scene.json*"
 */
inline std::string buildEditorWindowTitle(const std::filesystem::path& scenePath, bool modified) {
    std::string title = "Limbo Editor";
    if (!scenePath.empty()) {
        title += " - " + scenePath.filename().string();
    } else {
        title += " - Untitled";
    }
    if (modified) {
        title += "*";
    }
    return title;
}

}  // namespace limbo::editor
