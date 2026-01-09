#include "ConsolePanel.hpp"
#include "../EditorApp.hpp"

#include <imgui.h>

namespace limbo::editor {

ConsolePanel::ConsolePanel(EditorApp& editor) : m_editor(editor) {}

ConsolePanel::~ConsolePanel() = default;

void ConsolePanel::init() {
    // Register log callback
    log::addLogCallback([this](const log::LogEntry& entry) { addEntry(entry); });
}

void ConsolePanel::shutdown() {
    // Note: log callbacks are cleared when log system shuts down
}

void ConsolePanel::addEntry(const log::LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_entriesMutex);

    m_entries.push_back(entry);

    // Keep ring buffer size limited
    while (m_entries.size() > kMaxEntries) {
        m_entries.pop_front();
    }

    // Request scroll to bottom on next render
    if (m_autoScroll) {
        m_scrollToBottom = true;
    }
}

void ConsolePanel::clear() {
    std::lock_guard<std::mutex> lock(m_entriesMutex);
    m_entries.clear();
}

void ConsolePanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Console", &m_open, ImGuiWindowFlags_MenuBar)) {
        drawToolbar();
        drawLogEntries();
    }
    ImGui::End();
}

void ConsolePanel::drawToolbar() {
    if (ImGui::BeginMenuBar()) {
        // Clear button
        if (ImGui::Button("Clear")) {
            clear();
        }

        ImGui::Separator();

        // Level filters
        ImGui::TextUnformatted("Filter:");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Checkbox("Debug", &m_showDebug);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::Checkbox("Info", &m_showInfo);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::Checkbox("Warn", &m_showWarn);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Checkbox("Error", &m_showError);
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Auto-scroll toggle
        ImGui::Checkbox("Auto-scroll", &m_autoScroll);

        ImGui::Separator();

        // Search box
        ImGui::SetNextItemWidth(150);
        if (ImGui::InputTextWithHint("##Search", "Search...", m_searchBuffer,
                                     sizeof(m_searchBuffer))) {
            m_searchFilter = m_searchBuffer;
        }

        ImGui::EndMenuBar();
    }
}

void ConsolePanel::drawLogEntries() {
    // Log entries area
    ImGui::BeginChild("LogEntries", ImVec2(0, 0), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Lock for reading entries
    std::lock_guard<std::mutex> lock(m_entriesMutex);

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(m_entries.size()));

    // We need to filter, so we can't use the clipper directly
    // Instead, iterate through all entries and use clipper for visible range
    int visibleCount = 0;
    std::vector<usize> visibleIndices;
    visibleIndices.reserve(m_entries.size());

    for (usize i = 0; i < m_entries.size(); ++i) {
        if (matchesFilter(m_entries[i])) {
            visibleIndices.push_back(i);
        }
    }

    clipper.Begin(static_cast<int>(visibleIndices.size()));

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            usize entryIndex = visibleIndices[static_cast<usize>(row)];
            const auto& entry = m_entries[entryIndex];

            // Level icon and color
            glm::vec4 color = getLevelColor(entry.level);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, color.a));

            // Format: [LEVEL] [CATEGORY] message
            const char* icon = getLevelIcon(entry.level);
            ImGui::TextUnformatted(icon);
            ImGui::SameLine();

            // Category in brackets
            ImGui::TextDisabled("[%s]", entry.category.c_str());
            ImGui::SameLine();

            // Message
            ImGui::TextUnformatted(entry.message.c_str());

            ImGui::PopStyleColor();
        }
    }

    clipper.End();

    // Auto-scroll to bottom
    if (m_scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }

    ImGui::EndChild();
}

bool ConsolePanel::matchesFilter(const log::LogEntry& entry) const {
    // Check level filter
    switch (entry.level) {
    case spdlog::level::debug:
    case spdlog::level::trace:
        if (!m_showDebug)
            return false;
        break;
    case spdlog::level::info:
        if (!m_showInfo)
            return false;
        break;
    case spdlog::level::warn:
        if (!m_showWarn)
            return false;
        break;
    case spdlog::level::err:
    case spdlog::level::critical:
        if (!m_showError)
            return false;
        break;
    default:
        break;
    }

    // Check category filter
    if (!m_categoryFilter.empty() && entry.category != m_categoryFilter) {
        return false;
    }

    // Check search filter
    if (!m_searchFilter.empty()) {
        // Case-insensitive search in message
        String lowerMessage = entry.message;
        String lowerSearch = m_searchFilter;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

        if (lowerMessage.find(lowerSearch) == String::npos) {
            return false;
        }
    }

    return true;
}

glm::vec4 ConsolePanel::getLevelColor(spdlog::level::level_enum level) const {
    switch (level) {
    case spdlog::level::trace:
    case spdlog::level::debug:
        return {0.6f, 0.6f, 0.6f, 1.0f};  // Gray
    case spdlog::level::info:
        return {0.4f, 0.8f, 1.0f, 1.0f};  // Cyan
    case spdlog::level::warn:
        return {1.0f, 0.8f, 0.2f, 1.0f};  // Yellow
    case spdlog::level::err:
        return {1.0f, 0.3f, 0.3f, 1.0f};  // Red
    case spdlog::level::critical:
        return {1.0f, 0.0f, 0.0f, 1.0f};  // Bright red
    default:
        return {1.0f, 1.0f, 1.0f, 1.0f};  // White
    }
}

const char* ConsolePanel::getLevelIcon(spdlog::level::level_enum level) const {
    switch (level) {
    case spdlog::level::trace:
    case spdlog::level::debug:
        return "[D]";
    case spdlog::level::info:
        return "[I]";
    case spdlog::level::warn:
        return "[W]";
    case spdlog::level::err:
        return "[E]";
    case spdlog::level::critical:
        return "[!]";
    default:
        return "[ ]";
    }
}

}  // namespace limbo::editor
