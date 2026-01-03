#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>

namespace limbo {

// Key codes (matching GLFW key codes for simplicity)
enum class Key : i32 {
    Unknown = -1,

    // Printable keys
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = 59,
    Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,

    // Function keys
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    ScrollLock = 281,
    NumLock = 282,
    PrintScreen = 283,
    Pause = 284,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
    Keypad0 = 320, Keypad1, Keypad2, Keypad3, Keypad4,
    Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
    KeypadDecimal = 330,
    KeypadDivide = 331,
    KeypadMultiply = 332,
    KeypadSubtract = 333,
    KeypadAdd = 334,
    KeypadEnter = 335,
    KeypadEqual = 336,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
    Menu = 348,

    MaxKeys = 512
};

// Mouse button codes
enum class MouseButton : i32 {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7,

    MaxButtons = 8
};

// Modifier key flags
enum class Modifier : u8 {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

inline Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline Modifier operator&(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasModifier(Modifier mods, Modifier flag) {
    return (static_cast<u8>(mods) & static_cast<u8>(flag)) != 0;
}

class Window;

// Input state manager - singleton accessed through Input::get()
class LIMBO_API Input {
public:
    // Initialize input system for a window
    static void init(Window& window);

    // Update input state (call once per frame, before polling events)
    static void update();

    // Keyboard queries
    [[nodiscard]] static bool isKeyDown(Key key);      // Currently pressed
    [[nodiscard]] static bool isKeyPressed(Key key);   // Just pressed this frame
    [[nodiscard]] static bool isKeyReleased(Key key);  // Just released this frame

    // Mouse button queries
    [[nodiscard]] static bool isMouseButtonDown(MouseButton button);
    [[nodiscard]] static bool isMouseButtonPressed(MouseButton button);
    [[nodiscard]] static bool isMouseButtonReleased(MouseButton button);

    // Mouse position
    [[nodiscard]] static glm::vec2 getMousePosition();
    [[nodiscard]] static glm::vec2 getMouseDelta();    // Movement since last frame
    [[nodiscard]] static f32 getMouseX();
    [[nodiscard]] static f32 getMouseY();

    // Mouse scroll
    [[nodiscard]] static glm::vec2 getScrollDelta();   // Scroll wheel movement this frame
    [[nodiscard]] static f32 getScrollX();
    [[nodiscard]] static f32 getScrollY();

    // Modifier keys
    [[nodiscard]] static Modifier getModifiers();
    [[nodiscard]] static bool isShiftDown();
    [[nodiscard]] static bool isControlDown();
    [[nodiscard]] static bool isAltDown();
    [[nodiscard]] static bool isSuperDown();

    // Cursor control
    static void setCursorMode(bool visible, bool locked = false);
    [[nodiscard]] static bool isCursorVisible();
    [[nodiscard]] static bool isCursorLocked();

private:
    Input() = default;

    // Internal callbacks - called from platform layer
    friend void internal_keyCallback(i32 key, i32 scancode, i32 action, i32 mods);
    friend void internal_mouseButtonCallback(i32 button, i32 action, i32 mods);
    friend void internal_cursorPosCallback(f64 x, f64 y);
    friend void internal_scrollCallback(f64 xOffset, f64 yOffset);
    friend void internal_charCallback(u32 codepoint);
};

} // namespace limbo
