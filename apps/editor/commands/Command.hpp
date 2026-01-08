#pragma once

#include <limbo/core/Base.hpp>
#include <limbo/core/Types.hpp>

#include <string>

namespace limbo::editor {

/**
 * Command - Base class for undo/redo commands
 *
 * Implements the Command pattern for editor operations.
 * Each command stores enough state to both execute and undo the operation.
 */
class Command {
public:
    virtual ~Command() = default;

    /**
     * Execute the command (do/redo)
     * @return True if successful
     */
    virtual bool execute() = 0;

    /**
     * Undo the command
     * @return True if successful
     */
    virtual bool undo() = 0;

    /**
     * Get a human-readable description of the command
     * Used for UI display and debugging
     */
    [[nodiscard]] virtual String getDescription() const = 0;

    /**
     * Check if this command can be merged with another
     * Used for combining rapid property changes (e.g., dragging a slider)
     * @param other The command to potentially merge with
     * @return True if the commands can be merged
     */
    [[nodiscard]] virtual bool canMergeWith([[maybe_unused]] const Command& other) const {
        return false;
    }

    /**
     * Merge another command into this one
     * Called when canMergeWith returns true
     * @param other The command to merge
     */
    virtual void mergeWith([[maybe_unused]] const Command& other) {}

    /**
     * Get the command type ID for merging checks
     * Commands can only merge if they have the same type
     */
    [[nodiscard]] virtual usize getTypeId() const = 0;
};

/**
 * Helper macro for implementing getTypeId() using typeid hash
 */
#define COMMAND_TYPE_ID()                                                                          \
    [[nodiscard]] usize getTypeId() const override {                                               \
        return typeid(*this).hash_code();                                                          \
    }

}  // namespace limbo::editor
