#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/platform/Input.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace limbo {

/**
 * Input context for switching between editor and game input
 */
enum class InputContext : u8 {
    Game,    // Normal gameplay input
    Editor,  // Editor UI input (may block game input)
    Menu,    // Menu/pause input
    Cutscene // Cutscene input (limited player control)
};

/**
 * Input binding source types
 */
struct KeyBinding {
    Key key = Key::Unknown;
    Modifier requiredModifiers = Modifier::None;
};

struct MouseButtonBinding {
    MouseButton button = MouseButton::Left;
    Modifier requiredModifiers = Modifier::None;
};

struct MouseAxisBinding {
    enum class Axis { X, Y, ScrollX, ScrollY };
    Axis axis = Axis::X;
    f32 sensitivity = 1.0f;
    bool inverted = false;
};

struct KeyAxisBinding {
    Key positiveKey = Key::Unknown;
    Key negativeKey = Key::Unknown;
};

using ActionBinding = std::variant<KeyBinding, MouseButtonBinding>;
using AxisBinding = std::variant<KeyAxisBinding, MouseAxisBinding>;

/**
 * Action mapping - digital on/off input
 */
struct ActionMapping {
    String name;
    std::vector<ActionBinding> bindings;
    InputContext context = InputContext::Game;
};

/**
 * Axis mapping - analog -1 to 1 input
 */
struct AxisMapping {
    String name;
    std::vector<AxisBinding> bindings;
    InputContext context = InputContext::Game;
    f32 deadzone = 0.1f;
    f32 sensitivity = 1.0f;
};

/**
 * Input configuration loaded from JSON
 */
struct InputConfig {
    std::vector<ActionMapping> actions;
    std::vector<AxisMapping> axes;
};

/**
 * Callback types for input events
 */
using ActionCallback = std::function<void(const String& action, bool pressed)>;
using AxisCallback = std::function<void(const String& axis, f32 value)>;

/**
 * InputManager - High-level input abstraction with action/axis mapping
 *
 * Provides:
 * - Named action mappings (e.g., "Jump", "Fire")
 * - Named axis mappings (e.g., "MoveHorizontal", "LookVertical")
 * - Multiple bindings per action/axis
 * - Input context switching (game vs editor)
 * - JSON configuration loading
 */
class LIMBO_API InputManager {
public:
    /**
     * Initialize the input manager
     */
    static void init();

    /**
     * Shutdown the input manager
     */
    static void shutdown();

    /**
     * Update input state - call once per frame after Input::update()
     */
    static void update();

    // ========================================================================
    // Action Queries
    // ========================================================================

    /**
     * Check if an action is currently active (button held)
     */
    [[nodiscard]] static bool isActionDown(const String& action);

    /**
     * Check if an action was just pressed this frame
     */
    [[nodiscard]] static bool isActionPressed(const String& action);

    /**
     * Check if an action was just released this frame
     */
    [[nodiscard]] static bool isActionReleased(const String& action);

    // ========================================================================
    // Axis Queries
    // ========================================================================

    /**
     * Get the current value of an axis (-1 to 1)
     */
    [[nodiscard]] static f32 getAxisValue(const String& axis);

    /**
     * Get the raw axis value without deadzone or sensitivity applied
     */
    [[nodiscard]] static f32 getAxisRawValue(const String& axis);

    // ========================================================================
    // Context Management
    // ========================================================================

    /**
     * Set the current input context
     */
    static void setContext(InputContext context);

    /**
     * Get the current input context
     */
    [[nodiscard]] static InputContext getContext();

    /**
     * Push a context onto the stack (for temporary context changes)
     */
    static void pushContext(InputContext context);

    /**
     * Pop a context from the stack
     */
    static void popContext();

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Load input configuration from a JSON file
     */
    static bool loadConfig(const std::filesystem::path& path);

    /**
     * Save current input configuration to a JSON file
     */
    static bool saveConfig(const std::filesystem::path& path);

    /**
     * Register an action mapping programmatically
     */
    static void registerAction(const ActionMapping& mapping);

    /**
     * Register an axis mapping programmatically
     */
    static void registerAxis(const AxisMapping& mapping);

    /**
     * Clear all registered mappings
     */
    static void clearMappings();

    /**
     * Get all registered action names
     */
    [[nodiscard]] static std::vector<String> getActionNames();

    /**
     * Get all registered axis names
     */
    [[nodiscard]] static std::vector<String> getAxisNames();

    // ========================================================================
    // Callbacks (optional event-driven approach)
    // ========================================================================

    /**
     * Add a callback for action events
     * @return Handle to remove the callback later
     */
    static u32 addActionCallback(ActionCallback callback);

    /**
     * Remove an action callback by handle
     */
    static void removeActionCallback(u32 handle);

    /**
     * Add a callback for axis events (called when value changes)
     * @return Handle to remove the callback later
     */
    static u32 addAxisCallback(AxisCallback callback);

    /**
     * Remove an axis callback by handle
     */
    static void removeAxisCallback(u32 handle);

    // ========================================================================
    // Rebinding Support
    // ========================================================================

    /**
     * Start listening for the next input to rebind an action
     * @param action The action name to rebind
     * @param bindingIndex Which binding slot to replace (0 = primary)
     */
    static void startRebinding(const String& action, u32 bindingIndex = 0);

    /**
     * Cancel the current rebinding operation
     */
    static void cancelRebinding();

    /**
     * Check if currently waiting for rebind input
     */
    [[nodiscard]] static bool isRebinding();

    /**
     * Get the action currently being rebound
     */
    [[nodiscard]] static const String& getRebindingAction();

private:
    InputManager() = default;
};

}  // namespace limbo
