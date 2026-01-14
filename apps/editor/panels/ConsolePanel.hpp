#pragma once

#include <limbo/Limbo.hpp>
#include <limbo/debug/Log.hpp>

#include <deque>

namespace limbo::editor {

class EditorApp;

/**
 * ConsolePanel - Display engine and script log messages
 *
 * Features:
 * - Ring buffer of log entries (max 1000)
 * - Filter by log level (debug/info/warn/error)
 * - Search box for filtering messages
 * - Clear button
 * - Auto-scroll toggle
 * - Color-coded by level
 */
class ConsolePanel {
public:
    explicit ConsolePanel(EditorApp& editor);
    ~ConsolePanel();

    void init();
    void shutdown();
    void render();

    /**
     * Add a log entry to the console
     * Called by log callback
     */
    void addEntry(const log::LogEntry& entry);

    /**
     * Clear all log entries
     */
    void clear();

    [[nodiscard]] bool& isOpen() { return m_open; }

private:
    void drawToolbar();
    void drawLogEntries();
    [[nodiscard]] bool matchesFilter(const log::LogEntry& entry) const;
    [[nodiscard]] glm::vec4 getLevelColor(spdlog::level::level_enum level) const;
    [[nodiscard]] const char* getLevelIcon(spdlog::level::level_enum level) const;

    /**
     * Extract file:line reference from a log message if present
     * Returns empty string if no file:line pattern found
     */
    [[nodiscard]] String extractFileLineRef(const String& message) const;

private:
    EditorApp& m_editor;
    bool m_open = true;

    // Log entries (ring buffer)
    static constexpr usize kMaxEntries = 1000;
    std::deque<log::LogEntry> m_entries;
    std::mutex m_entriesMutex;

    // Filter settings
    char m_searchBuffer[256] = {0};
    String m_searchFilter;
    bool m_showDebug = true;
    bool m_showInfo = true;
    bool m_showWarn = true;
    bool m_showError = true;

    // View settings
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;

    // Selection
    int m_selectedEntry = -1;

    // Category filter (empty = show all)
    String m_categoryFilter;
};

}  // namespace limbo::editor
