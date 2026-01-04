#include "limbo/platform/Input.hpp"
#include "limbo/platform/Platform.hpp"
#include "limbo/core/Assert.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstring>

namespace limbo {

// Internal state
struct InputState {
    // Keyboard state
    std::array<bool, static_cast<size_t>(Key::MaxKeys)> keysCurrent{};
    std::array<bool, static_cast<size_t>(Key::MaxKeys)> keysPrevious{};

    // Mouse button state
    std::array<bool, static_cast<size_t>(MouseButton::MaxButtons)> mouseButtonsCurrent{};
    std::array<bool, static_cast<size_t>(MouseButton::MaxButtons)> mouseButtonsPrevious{};

    // Mouse position
    glm::vec2 mousePosition{0.0f};
    glm::vec2 mousePreviousPosition{0.0f};
    glm::vec2 mouseDelta{0.0f};

    // Scroll
    glm::vec2 scrollDelta{0.0f};
    glm::vec2 scrollAccumulator{0.0f};

    // Modifiers
    Modifier modifiers = Modifier::None;

    // Cursor state
    bool cursorVisible = true;
    bool cursorLocked = false;

    // Window reference
    GLFWwindow* window = nullptr;
    bool initialized = false;
};

static InputState s_state;

// Forward declarations for internal callbacks
void internal_keyCallback(i32 key, i32 scancode, i32 action, i32 mods);
void internal_mouseButtonCallback(i32 button, i32 action, i32 mods);
void internal_cursorPosCallback(f64 x, f64 y);
void internal_scrollCallback(f64 xOffset, f64 yOffset);
void internal_charCallback(u32 codepoint);

// GLFW callback trampolines
static void glfwKeyCallback(GLFWwindow* /*window*/, int key, int scancode, int action, int mods) {
    internal_keyCallback(key, scancode, action, mods);
}

static void glfwMouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int mods) {
    internal_mouseButtonCallback(button, action, mods);
}

static void glfwCursorPosCallback(GLFWwindow* /*window*/, double x, double y) {
    internal_cursorPosCallback(x, y);
}

static void glfwScrollCallback(GLFWwindow* /*window*/, double xOffset, double yOffset) {
    internal_scrollCallback(xOffset, yOffset);
}

static void glfwCharCallback(GLFWwindow* /*window*/, unsigned int codepoint) {
    internal_charCallback(codepoint);
}

void Input::init(Window& window) {
    s_state = InputState{};
    s_state.window = window.getNativeHandle();
    s_state.initialized = true;

    // Set up GLFW callbacks
    glfwSetKeyCallback(s_state.window, glfwKeyCallback);
    glfwSetMouseButtonCallback(s_state.window, glfwMouseButtonCallback);
    glfwSetCursorPosCallback(s_state.window, glfwCursorPosCallback);
    glfwSetScrollCallback(s_state.window, glfwScrollCallback);
    glfwSetCharCallback(s_state.window, glfwCharCallback);

    // Get initial mouse position
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(s_state.window, &x, &y);
    s_state.mousePosition = glm::vec2(static_cast<f32>(x), static_cast<f32>(y));
    s_state.mousePreviousPosition = s_state.mousePosition;

    spdlog::debug("Input system initialized");
}

void Input::update() {
    LIMBO_ASSERT(s_state.initialized, "Input system not initialized");

    // Copy current state to previous
    s_state.keysPrevious = s_state.keysCurrent;
    s_state.mouseButtonsPrevious = s_state.mouseButtonsCurrent;

    // Calculate mouse delta
    s_state.mouseDelta = s_state.mousePosition - s_state.mousePreviousPosition;
    s_state.mousePreviousPosition = s_state.mousePosition;

    // Copy scroll accumulator to delta and reset
    s_state.scrollDelta = s_state.scrollAccumulator;
    s_state.scrollAccumulator = glm::vec2(0.0f);
}

// Keyboard queries
bool Input::isKeyDown(Key key) {
    auto idx = static_cast<size_t>(key);
    if (idx >= s_state.keysCurrent.size()) {
        return false;
}
    return s_state.keysCurrent[idx];
}

bool Input::isKeyPressed(Key key) {
    auto idx = static_cast<size_t>(key);
    if (idx >= s_state.keysCurrent.size()) {
        return false;
}
    return s_state.keysCurrent[idx] && !s_state.keysPrevious[idx];
}

bool Input::isKeyReleased(Key key) {
    auto idx = static_cast<size_t>(key);
    if (idx >= s_state.keysCurrent.size()) {
        return false;
}
    return !s_state.keysCurrent[idx] && s_state.keysPrevious[idx];
}

// Mouse button queries
bool Input::isMouseButtonDown(MouseButton button) {
    auto idx = static_cast<size_t>(button);
    if (idx >= s_state.mouseButtonsCurrent.size()) {
        return false;
}
    return s_state.mouseButtonsCurrent[idx];
}

bool Input::isMouseButtonPressed(MouseButton button) {
    auto idx = static_cast<size_t>(button);
    if (idx >= s_state.mouseButtonsCurrent.size()) {
        return false;
}
    return s_state.mouseButtonsCurrent[idx] && !s_state.mouseButtonsPrevious[idx];
}

bool Input::isMouseButtonReleased(MouseButton button) {
    auto idx = static_cast<size_t>(button);
    if (idx >= s_state.mouseButtonsCurrent.size()) {
        return false;
}
    return !s_state.mouseButtonsCurrent[idx] && s_state.mouseButtonsPrevious[idx];
}

// Mouse position
glm::vec2 Input::getMousePosition() {
    return s_state.mousePosition;
}

glm::vec2 Input::getMouseDelta() {
    return s_state.mouseDelta;
}

f32 Input::getMouseX() {
    return s_state.mousePosition.x;
}

f32 Input::getMouseY() {
    return s_state.mousePosition.y;
}

// Mouse scroll
glm::vec2 Input::getScrollDelta() {
    return s_state.scrollDelta;
}

f32 Input::getScrollX() {
    return s_state.scrollDelta.x;
}

f32 Input::getScrollY() {
    return s_state.scrollDelta.y;
}

// Modifier keys
Modifier Input::getModifiers() {
    return s_state.modifiers;
}

bool Input::isShiftDown() {
    return hasModifier(s_state.modifiers, Modifier::Shift);
}

bool Input::isControlDown() {
    return hasModifier(s_state.modifiers, Modifier::Control);
}

bool Input::isAltDown() {
    return hasModifier(s_state.modifiers, Modifier::Alt);
}

bool Input::isSuperDown() {
    return hasModifier(s_state.modifiers, Modifier::Super);
}

// Cursor control
void Input::setCursorMode(bool visible, bool locked) {
    LIMBO_ASSERT(s_state.initialized, "Input system not initialized");

    s_state.cursorVisible = visible;
    s_state.cursorLocked = locked;

    if (locked) {
        glfwSetInputMode(s_state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else if (!visible) {
        glfwSetInputMode(s_state.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
        glfwSetInputMode(s_state.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

bool Input::isCursorVisible() {
    return s_state.cursorVisible;
}

bool Input::isCursorLocked() {
    return s_state.cursorLocked;
}

// Internal callbacks (friend functions)
void internal_keyCallback(i32 key, i32 /*scancode*/, i32 action, i32 mods) {
    if (key < 0 || key >= static_cast<i32>(Key::MaxKeys)) {
        return;
    }

    auto idx = static_cast<size_t>(key);

    if (action == GLFW_PRESS) {
        s_state.keysCurrent[idx] = true;
    } else if (action == GLFW_RELEASE) {
        s_state.keysCurrent[idx] = false;
    }
    // GLFW_REPEAT is ignored - we track state, not events

    // Update modifiers
    s_state.modifiers = Modifier::None;
    if (mods & GLFW_MOD_SHIFT) {
        s_state.modifiers = s_state.modifiers | Modifier::Shift;
    }
    if (mods & GLFW_MOD_CONTROL) {
        s_state.modifiers = s_state.modifiers | Modifier::Control;
    }
    if (mods & GLFW_MOD_ALT) {
        s_state.modifiers = s_state.modifiers | Modifier::Alt;
    }
    if (mods & GLFW_MOD_SUPER) {
        s_state.modifiers = s_state.modifiers | Modifier::Super;
    }
    if (mods & GLFW_MOD_CAPS_LOCK) {
        s_state.modifiers = s_state.modifiers | Modifier::CapsLock;
    }
    if (mods & GLFW_MOD_NUM_LOCK) {
        s_state.modifiers = s_state.modifiers | Modifier::NumLock;
    }
}

void internal_mouseButtonCallback(i32 button, i32 action, i32 mods) {
    if (button < 0 || button >= static_cast<i32>(MouseButton::MaxButtons)) {
        return;
    }

    auto idx = static_cast<size_t>(button);

    if (action == GLFW_PRESS) {
        s_state.mouseButtonsCurrent[idx] = true;
    } else if (action == GLFW_RELEASE) {
        s_state.mouseButtonsCurrent[idx] = false;
    }

    // Update modifiers (same as key callback)
    s_state.modifiers = Modifier::None;
    if (mods & GLFW_MOD_SHIFT) {
        s_state.modifiers = s_state.modifiers | Modifier::Shift;
    }
    if (mods & GLFW_MOD_CONTROL) {
        s_state.modifiers = s_state.modifiers | Modifier::Control;
    }
    if (mods & GLFW_MOD_ALT) {
        s_state.modifiers = s_state.modifiers | Modifier::Alt;
    }
    if (mods & GLFW_MOD_SUPER) {
        s_state.modifiers = s_state.modifiers | Modifier::Super;
    }
}

void internal_cursorPosCallback(f64 x, f64 y) {
    s_state.mousePosition = glm::vec2(static_cast<f32>(x), static_cast<f32>(y));
}

void internal_scrollCallback(f64 xOffset, f64 yOffset) {
    s_state.scrollAccumulator.x += static_cast<f32>(xOffset);
    s_state.scrollAccumulator.y += static_cast<f32>(yOffset);
}

void internal_charCallback(u32 /*codepoint*/) {
    // Text input - can be used for text fields later
    // For now, just ignore
}

}  // namespace limbo
