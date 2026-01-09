#pragma once

#include "Command.hpp"

#include <limbo/core/Base.hpp>
#include <limbo/core/Types.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace limbo::editor {

/**
 * CommandHistory - Manages undo/redo stack for editor operations
 *
 * Maintains a history of executed commands and provides undo/redo functionality.
 * Supports command merging for combining rapid sequential changes.
 */
class CommandHistory {
public:
    using CommandPtr = Unique<Command>;
    using HistoryChangedCallback = std::function<void()>;

    CommandHistory() = default;
    ~CommandHistory() = default;

    // Non-copyable
    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    /**
     * Execute a command and add it to the history
     * @param command The command to execute
     * @return True if the command executed successfully
     */
    bool execute(CommandPtr command);

    /**
     * Undo the last command
     * @return True if undo succeeded
     */
    bool undo();

    /**
     * Redo the last undone command
     * @return True if redo succeeded
     */
    bool redo();

    /**
     * Check if undo is available
     */
    [[nodiscard]] bool canUndo() const { return m_currentIndex >= 0; }

    /**
     * Check if redo is available
     */
    [[nodiscard]] bool canRedo() const {
        return m_currentIndex < static_cast<i32>(m_commands.size()) - 1;
    }

    /**
     * Get the description of the command that would be undone
     */
    [[nodiscard]] String getUndoDescription() const;

    /**
     * Get the description of the command that would be redone
     */
    [[nodiscard]] String getRedoDescription() const;

    /**
     * Clear all history
     */
    void clear();

    /**
     * Get the number of commands in history
     */
    [[nodiscard]] usize size() const { return m_commands.size(); }

    /**
     * Get the current position in history
     * -1 means no commands executed, 0 means first command executed, etc.
     */
    [[nodiscard]] i32 getCurrentIndex() const { return m_currentIndex; }

    /**
     * Set the maximum number of commands to keep in history
     * Oldest commands are removed when the limit is exceeded
     * @param maxSize Maximum history size (0 = unlimited)
     */
    void setMaxHistorySize(usize maxSize) { m_maxHistorySize = maxSize; }

    /**
     * Begin a compound command group
     * All commands executed until endGroup() are treated as a single undo operation
     */
    void beginGroup(const String& description);

    /**
     * End a compound command group
     */
    void endGroup();

    /**
     * Check if currently inside a command group
     */
    [[nodiscard]] bool isInGroup() const { return m_groupDepth > 0; }

    /**
     * Enable or disable command merging
     * When enabled, compatible sequential commands are merged
     */
    void setMergingEnabled(bool enabled) { m_mergingEnabled = enabled; }

    /**
     * Set callback for history changes (for UI updates)
     */
    void setHistoryChangedCallback(HistoryChangedCallback callback) {
        m_historyChangedCallback = std::move(callback);
    }

    /**
     * Mark a clean state (e.g., after saving)
     * Used to track if document has unsaved changes
     */
    void markClean() { m_cleanIndex = m_currentIndex; }

    /**
     * Check if the current state matches the clean state
     */
    [[nodiscard]] bool isClean() const { return m_currentIndex == m_cleanIndex; }

private:
    void notifyHistoryChanged();
    void trimHistory();

private:
    std::vector<CommandPtr> m_commands;
    i32 m_currentIndex = -1;  // Points to last executed command
    i32 m_cleanIndex = -1;    // Index when last marked clean
    usize m_maxHistorySize = 100;
    bool m_mergingEnabled = true;

    // Group support
    i32 m_groupDepth = 0;
    std::vector<CommandPtr> m_groupCommands;
    String m_groupDescription;

    HistoryChangedCallback m_historyChangedCallback;
};

/**
 * CompoundCommand - Groups multiple commands as a single undoable operation
 */
class CompoundCommand : public Command {
public:
    explicit CompoundCommand(const String& description) : m_description(description) {}

    void addCommand(Unique<Command> command) { m_commands.push_back(std::move(command)); }

    [[nodiscard]] bool isEmpty() const { return m_commands.empty(); }

    bool execute() override {
        for (auto& cmd : m_commands) {
            if (!cmd->execute()) {
                return false;
            }
        }
        return true;
    }

    bool undo() override {
        // Undo in reverse order
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
            if (!(*it)->undo()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] String getDescription() const override { return m_description; }

    COMMAND_TYPE_ID()

private:
    String m_description;
    std::vector<Unique<Command>> m_commands;
};

/**
 * RAII helper for command groups
 */
class ScopedCommandGroup {
public:
    ScopedCommandGroup(CommandHistory& history, const String& description) : m_history(history) {
        m_history.beginGroup(description);
    }

    ~ScopedCommandGroup() { m_history.endGroup(); }

    // Non-copyable
    ScopedCommandGroup(const ScopedCommandGroup&) = delete;
    ScopedCommandGroup& operator=(const ScopedCommandGroup&) = delete;

private:
    CommandHistory& m_history;
};

}  // namespace limbo::editor
