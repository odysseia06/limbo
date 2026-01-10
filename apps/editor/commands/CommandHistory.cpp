#include "CommandHistory.hpp"

#include <limbo/debug/Log.hpp>

namespace limbo::editor {

bool CommandHistory::execute(CommandPtr command) {
    if (!command) {
        return false;
    }

    // If in a group, add to group instead of main history
    if (m_groupDepth > 0) {
        if (command->execute()) {
            m_groupCommands.push_back(std::move(command));
            return true;
        }
        return false;
    }

    // Try to merge with the last command if merging is enabled
    if (m_mergingEnabled && m_currentIndex >= 0) {
        auto& lastCommand = m_commands[static_cast<usize>(m_currentIndex)];
        if (lastCommand->getTypeId() == command->getTypeId() &&
            lastCommand->canMergeWith(*command)) {
            // Execute the new command first
            if (!command->execute()) {
                return false;
            }
            // Merge into the existing command
            lastCommand->mergeWith(*command);
            notifyHistoryChanged();
            return true;
        }
    }

    // Execute the command
    if (!command->execute()) {
        LIMBO_LOG_EDITOR_WARN("Command failed to execute: {}", command->getDescription());
        return false;
    }

    // Remove any commands after current position (discard redo history)
    if (m_currentIndex < static_cast<i32>(m_commands.size()) - 1) {
        m_commands.erase(m_commands.begin() + m_currentIndex + 1, m_commands.end());
        // If we were past the clean state, we can never get back to it
        if (m_cleanIndex > m_currentIndex) {
            m_cleanIndex = -2;  // Impossible to reach
        }
    }

    // Add command to history
    m_commands.push_back(std::move(command));
    m_currentIndex = static_cast<i32>(m_commands.size()) - 1;

    // Trim history if needed
    trimHistory();

    notifyHistoryChanged();
    return true;
}

bool CommandHistory::undo() {
    if (!canUndo()) {
        return false;
    }

    auto& command = m_commands[static_cast<usize>(m_currentIndex)];
    if (!command->undo()) {
        LIMBO_LOG_EDITOR_WARN("Command failed to undo: {}", command->getDescription());
        return false;
    }

    m_currentIndex--;
    notifyHistoryChanged();
    return true;
}

bool CommandHistory::redo() {
    if (!canRedo()) {
        return false;
    }

    m_currentIndex++;
    auto& command = m_commands[static_cast<usize>(m_currentIndex)];
    if (!command->execute()) {
        LIMBO_LOG_EDITOR_WARN("Command failed to redo: {}", command->getDescription());
        m_currentIndex--;
        return false;
    }

    notifyHistoryChanged();
    return true;
}

String CommandHistory::getUndoDescription() const {
    if (!canUndo()) {
        return "";
    }
    return m_commands[static_cast<usize>(m_currentIndex)]->getDescription();
}

String CommandHistory::getRedoDescription() const {
    if (!canRedo()) {
        return "";
    }
    return m_commands[static_cast<usize>(m_currentIndex + 1)]->getDescription();
}

void CommandHistory::clear() {
    m_commands.clear();
    m_currentIndex = -1;
    m_cleanIndex = -1;
    m_groupDepth = 0;
    m_groupCommands.clear();
    notifyHistoryChanged();
}

void CommandHistory::beginGroup(const String& description) {
    if (m_groupDepth == 0) {
        m_groupDescription = description;
        m_groupCommands.clear();
    }
    m_groupDepth++;
}

void CommandHistory::endGroup() {
    if (m_groupDepth <= 0) {
        LIMBO_LOG_EDITOR_WARN("CommandHistory::endGroup called without matching beginGroup");
        return;
    }

    m_groupDepth--;

    if (m_groupDepth == 0 && !m_groupCommands.empty()) {
        // Create a compound command from the group
        auto compound = std::make_unique<CompoundCommand>(m_groupDescription);
        for (auto& cmd : m_groupCommands) {
            compound->addCommand(std::move(cmd));
        }
        m_groupCommands.clear();

        // Add to history (commands already executed, so just add)
        // Remove any commands after current position
        if (m_currentIndex < static_cast<i32>(m_commands.size()) - 1) {
            m_commands.erase(m_commands.begin() + m_currentIndex + 1, m_commands.end());
            if (m_cleanIndex > m_currentIndex) {
                m_cleanIndex = -2;
            }
        }

        m_commands.push_back(std::move(compound));
        m_currentIndex = static_cast<i32>(m_commands.size()) - 1;

        trimHistory();
        notifyHistoryChanged();
    }
}

void CommandHistory::notifyHistoryChanged() {
    if (m_historyChangedCallback) {
        m_historyChangedCallback();
    }
}

void CommandHistory::trimHistory() {
    if (m_maxHistorySize == 0 || m_commands.size() <= m_maxHistorySize) {
        return;
    }

    usize const toRemove = m_commands.size() - m_maxHistorySize;
    m_commands.erase(m_commands.begin(),
                     m_commands.begin() + static_cast<std::ptrdiff_t>(toRemove));
    m_currentIndex -= static_cast<i32>(toRemove);
    m_cleanIndex -= static_cast<i32>(toRemove);

    if (m_currentIndex < -1) {
        m_currentIndex = -1;
    }
}

}  // namespace limbo::editor
