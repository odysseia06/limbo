#include "limbo/input/InputManager.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <stack>

namespace limbo {

using json = nlohmann::json;

namespace {

struct ActionState {
    bool current = false;
    bool previous = false;
};

struct AxisState {
    f32 value = 0.0f;
    f32 rawValue = 0.0f;
};

struct InputManagerState {
    std::unordered_map<String, ActionMapping> actions;
    std::unordered_map<String, AxisMapping> axes;

    std::unordered_map<String, ActionState> actionStates;
    std::unordered_map<String, AxisState> axisStates;

    std::stack<InputContext> contextStack;
    InputContext currentContext = InputContext::Game;

    std::unordered_map<u32, ActionCallback> actionCallbacks;
    std::unordered_map<u32, AxisCallback> axisCallbacks;
    u32 nextCallbackHandle = 1;

    bool rebinding = false;
    String rebindingAction;
    u32 rebindingIndex = 0;

    bool initialized = false;
};

InputManagerState& getState() {
    static InputManagerState state;
    return state;
}

bool checkActionBinding(const ActionBinding& binding) {
    if (const auto* key = std::get_if<KeyBinding>(&binding)) {
        if (key->requiredModifiers != Modifier::None) {
            Modifier currentMods = Input::getModifiers();
            if ((currentMods & key->requiredModifiers) != key->requiredModifiers) {
                return false;
            }
        }
        return Input::isKeyDown(key->key);
    }
    if (const auto* mouse = std::get_if<MouseButtonBinding>(&binding)) {
        if (mouse->requiredModifiers != Modifier::None) {
            Modifier currentMods = Input::getModifiers();
            if ((currentMods & mouse->requiredModifiers) != mouse->requiredModifiers) {
                return false;
            }
        }
        return Input::isMouseButtonDown(mouse->button);
    }
    return false;
}

f32 getAxisBindingValue(const AxisBinding& binding) {
    if (const auto* keyAxis = std::get_if<KeyAxisBinding>(&binding)) {
        f32 value = 0.0f;
        if (Input::isKeyDown(keyAxis->positiveKey)) {
            value += 1.0f;
        }
        if (Input::isKeyDown(keyAxis->negativeKey)) {
            value -= 1.0f;
        }
        return value;
    }
    if (const auto* mouseAxis = std::get_if<MouseAxisBinding>(&binding)) {
        f32 value = 0.0f;
        switch (mouseAxis->axis) {
            case MouseAxisBinding::Axis::X:
                value = Input::getMouseDelta().x;
                break;
            case MouseAxisBinding::Axis::Y:
                value = Input::getMouseDelta().y;
                break;
            case MouseAxisBinding::Axis::ScrollX:
                value = Input::getScrollDelta().x;
                break;
            case MouseAxisBinding::Axis::ScrollY:
                value = Input::getScrollDelta().y;
                break;
        }
        value *= mouseAxis->sensitivity;
        if (mouseAxis->inverted) {
            value = -value;
        }
        return value;
    }
    return 0.0f;
}

f32 applyDeadzone(f32 value, f32 deadzone) {
    if (std::abs(value) < deadzone) {
        return 0.0f;
    }
    // Remap value from [deadzone, 1] to [0, 1]
    f32 sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
}

Key stringToKey(const String& str) {
    static const std::unordered_map<String, Key> keyMap = {
        {"Space", Key::Space},
        {"Apostrophe", Key::Apostrophe},
        {"Comma", Key::Comma},
        {"Minus", Key::Minus},
        {"Period", Key::Period},
        {"Slash", Key::Slash},
        {"0", Key::Num0},
        {"1", Key::Num1},
        {"2", Key::Num2},
        {"3", Key::Num3},
        {"4", Key::Num4},
        {"5", Key::Num5},
        {"6", Key::Num6},
        {"7", Key::Num7},
        {"8", Key::Num8},
        {"9", Key::Num9},
        {"Semicolon", Key::Semicolon},
        {"Equal", Key::Equal},
        {"A", Key::A},
        {"B", Key::B},
        {"C", Key::C},
        {"D", Key::D},
        {"E", Key::E},
        {"F", Key::F},
        {"G", Key::G},
        {"H", Key::H},
        {"I", Key::I},
        {"J", Key::J},
        {"K", Key::K},
        {"L", Key::L},
        {"M", Key::M},
        {"N", Key::N},
        {"O", Key::O},
        {"P", Key::P},
        {"Q", Key::Q},
        {"R", Key::R},
        {"S", Key::S},
        {"T", Key::T},
        {"U", Key::U},
        {"V", Key::V},
        {"W", Key::W},
        {"X", Key::X},
        {"Y", Key::Y},
        {"Z", Key::Z},
        {"LeftBracket", Key::LeftBracket},
        {"Backslash", Key::Backslash},
        {"RightBracket", Key::RightBracket},
        {"GraveAccent", Key::GraveAccent},
        {"Escape", Key::Escape},
        {"Enter", Key::Enter},
        {"Tab", Key::Tab},
        {"Backspace", Key::Backspace},
        {"Insert", Key::Insert},
        {"Delete", Key::Delete},
        {"Right", Key::Right},
        {"Left", Key::Left},
        {"Down", Key::Down},
        {"Up", Key::Up},
        {"PageUp", Key::PageUp},
        {"PageDown", Key::PageDown},
        {"Home", Key::Home},
        {"End", Key::End},
        {"CapsLock", Key::CapsLock},
        {"ScrollLock", Key::ScrollLock},
        {"NumLock", Key::NumLock},
        {"PrintScreen", Key::PrintScreen},
        {"Pause", Key::Pause},
        {"F1", Key::F1},
        {"F2", Key::F2},
        {"F3", Key::F3},
        {"F4", Key::F4},
        {"F5", Key::F5},
        {"F6", Key::F6},
        {"F7", Key::F7},
        {"F8", Key::F8},
        {"F9", Key::F9},
        {"F10", Key::F10},
        {"F11", Key::F11},
        {"F12", Key::F12},
        {"LeftShift", Key::LeftShift},
        {"LeftControl", Key::LeftControl},
        {"LeftAlt", Key::LeftAlt},
        {"LeftSuper", Key::LeftSuper},
        {"RightShift", Key::RightShift},
        {"RightControl", Key::RightControl},
        {"RightAlt", Key::RightAlt},
        {"RightSuper", Key::RightSuper},
    };

    auto it = keyMap.find(str);
    if (it != keyMap.end()) {
        return it->second;
    }
    return Key::Unknown;
}

String keyToString(Key key) {
    static const std::unordered_map<Key, String> keyMap = {
        {Key::Space, "Space"},
        {Key::A, "A"},
        {Key::B, "B"},
        {Key::C, "C"},
        {Key::D, "D"},
        {Key::E, "E"},
        {Key::F, "F"},
        {Key::G, "G"},
        {Key::H, "H"},
        {Key::I, "I"},
        {Key::J, "J"},
        {Key::K, "K"},
        {Key::L, "L"},
        {Key::M, "M"},
        {Key::N, "N"},
        {Key::O, "O"},
        {Key::P, "P"},
        {Key::Q, "Q"},
        {Key::R, "R"},
        {Key::S, "S"},
        {Key::T, "T"},
        {Key::U, "U"},
        {Key::V, "V"},
        {Key::W, "W"},
        {Key::X, "X"},
        {Key::Y, "Y"},
        {Key::Z, "Z"},
        {Key::Num0, "0"},
        {Key::Num1, "1"},
        {Key::Num2, "2"},
        {Key::Num3, "3"},
        {Key::Num4, "4"},
        {Key::Num5, "5"},
        {Key::Num6, "6"},
        {Key::Num7, "7"},
        {Key::Num8, "8"},
        {Key::Num9, "9"},
        {Key::Escape, "Escape"},
        {Key::Enter, "Enter"},
        {Key::Tab, "Tab"},
        {Key::Backspace, "Backspace"},
        {Key::Up, "Up"},
        {Key::Down, "Down"},
        {Key::Left, "Left"},
        {Key::Right, "Right"},
        {Key::LeftShift, "LeftShift"},
        {Key::LeftControl, "LeftControl"},
        {Key::LeftAlt, "LeftAlt"},
        {Key::RightShift, "RightShift"},
        {Key::RightControl, "RightControl"},
        {Key::RightAlt, "RightAlt"},
        {Key::F1, "F1"},
        {Key::F2, "F2"},
        {Key::F3, "F3"},
        {Key::F4, "F4"},
        {Key::F5, "F5"},
        {Key::F6, "F6"},
        {Key::F7, "F7"},
        {Key::F8, "F8"},
        {Key::F9, "F9"},
        {Key::F10, "F10"},
        {Key::F11, "F11"},
        {Key::F12, "F12"},
    };

    auto it = keyMap.find(key);
    if (it != keyMap.end()) {
        return it->second;
    }
    return "Unknown";
}

MouseButton stringToMouseButton(const String& str) {
    if (str == "Left")
        return MouseButton::Left;
    if (str == "Right")
        return MouseButton::Right;
    if (str == "Middle")
        return MouseButton::Middle;
    if (str == "Button4")
        return MouseButton::Button4;
    if (str == "Button5")
        return MouseButton::Button5;
    return MouseButton::Left;
}

String mouseButtonToString(MouseButton button) {
    switch (button) {
        case MouseButton::Left:
            return "Left";
        case MouseButton::Right:
            return "Right";
        case MouseButton::Middle:
            return "Middle";
        case MouseButton::Button4:
            return "Button4";
        case MouseButton::Button5:
            return "Button5";
        default:
            return "Unknown";
    }
}

InputContext stringToContext(const String& str) {
    if (str == "Game")
        return InputContext::Game;
    if (str == "Editor")
        return InputContext::Editor;
    if (str == "Menu")
        return InputContext::Menu;
    if (str == "Cutscene")
        return InputContext::Cutscene;
    return InputContext::Game;
}

String contextToString(InputContext context) {
    switch (context) {
        case InputContext::Game:
            return "Game";
        case InputContext::Editor:
            return "Editor";
        case InputContext::Menu:
            return "Menu";
        case InputContext::Cutscene:
            return "Cutscene";
    }
    return "Game";
}

}  // namespace

void InputManager::init() {
    auto& state = getState();
    state.actions.clear();
    state.axes.clear();
    state.actionStates.clear();
    state.axisStates.clear();
    while (!state.contextStack.empty()) {
        state.contextStack.pop();
    }
    state.currentContext = InputContext::Game;
    state.actionCallbacks.clear();
    state.axisCallbacks.clear();
    state.nextCallbackHandle = 1;
    state.rebinding = false;
    state.rebindingAction.clear();
    state.initialized = true;

    spdlog::debug("InputManager initialized");
}

void InputManager::shutdown() {
    auto& state = getState();
    state.actions.clear();
    state.axes.clear();
    state.actionStates.clear();
    state.axisStates.clear();
    state.actionCallbacks.clear();
    state.axisCallbacks.clear();
    state.initialized = false;

    spdlog::debug("InputManager shutdown");
}

void InputManager::update() {
    auto& state = getState();

    if (!state.initialized) {
        return;
    }

    // Update action states
    for (auto& [name, mapping] : state.actions) {
        // Skip if wrong context
        if (mapping.context != state.currentContext && mapping.context != InputContext::Game) {
            continue;
        }

        ActionState& actionState = state.actionStates[name];
        actionState.previous = actionState.current;

        // Check all bindings
        actionState.current = false;
        for (const auto& binding : mapping.bindings) {
            if (checkActionBinding(binding)) {
                actionState.current = true;
                break;
            }
        }

        // Fire callbacks on state change
        if (actionState.current != actionState.previous) {
            for (const auto& [handle, callback] : state.actionCallbacks) {
                callback(name, actionState.current);
            }
        }
    }

    // Update axis states
    for (auto& [name, mapping] : state.axes) {
        // Skip if wrong context
        if (mapping.context != state.currentContext && mapping.context != InputContext::Game) {
            continue;
        }

        AxisState& axisState = state.axisStates[name];
        f32 previousValue = axisState.value;

        // Sum all binding values
        f32 rawValue = 0.0f;
        for (const auto& binding : mapping.bindings) {
            rawValue += getAxisBindingValue(binding);
        }

        // Clamp to [-1, 1]
        rawValue = std::clamp(rawValue, -1.0f, 1.0f);
        axisState.rawValue = rawValue;

        // Apply deadzone and sensitivity
        f32 value = applyDeadzone(rawValue, mapping.deadzone);
        value *= mapping.sensitivity;
        value = std::clamp(value, -1.0f, 1.0f);
        axisState.value = value;

        // Fire callbacks on significant change
        constexpr f32 changeThreshold = 0.001f;
        if (std::abs(axisState.value - previousValue) > changeThreshold) {
            for (const auto& [handle, callback] : state.axisCallbacks) {
                callback(name, axisState.value);
            }
        }
    }

    // Handle rebinding
    if (state.rebinding) {
        // Check for key press
        for (i32 k = 0; k < static_cast<i32>(Key::MaxKeys); ++k) {
            Key key = static_cast<Key>(k);
            if (Input::isKeyPressed(key)) {
                // Skip modifier-only keys
                if (key == Key::LeftShift || key == Key::RightShift || key == Key::LeftControl ||
                    key == Key::RightControl || key == Key::LeftAlt || key == Key::RightAlt) {
                    continue;
                }

                // Apply rebinding
                auto it = state.actions.find(state.rebindingAction);
                if (it != state.actions.end()) {
                    KeyBinding newBinding;
                    newBinding.key = key;
                    newBinding.requiredModifiers = Input::getModifiers();

                    if (state.rebindingIndex < it->second.bindings.size()) {
                        it->second.bindings[state.rebindingIndex] = newBinding;
                    } else {
                        it->second.bindings.push_back(newBinding);
                    }
                }

                state.rebinding = false;
                state.rebindingAction.clear();
                break;
            }
        }

        // Check for mouse button press
        for (i32 b = 0; b < static_cast<i32>(MouseButton::MaxButtons); ++b) {
            MouseButton button = static_cast<MouseButton>(b);
            if (Input::isMouseButtonPressed(button)) {
                auto it = state.actions.find(state.rebindingAction);
                if (it != state.actions.end()) {
                    MouseButtonBinding newBinding;
                    newBinding.button = button;
                    newBinding.requiredModifiers = Input::getModifiers();

                    if (state.rebindingIndex < it->second.bindings.size()) {
                        it->second.bindings[state.rebindingIndex] = newBinding;
                    } else {
                        it->second.bindings.push_back(newBinding);
                    }
                }

                state.rebinding = false;
                state.rebindingAction.clear();
                break;
            }
        }
    }
}

bool InputManager::isActionDown(const String& action) {
    auto& state = getState();
    auto it = state.actionStates.find(action);
    if (it == state.actionStates.end()) {
        return false;
    }
    return it->second.current;
}

bool InputManager::isActionPressed(const String& action) {
    auto& state = getState();
    auto it = state.actionStates.find(action);
    if (it == state.actionStates.end()) {
        return false;
    }
    return it->second.current && !it->second.previous;
}

bool InputManager::isActionReleased(const String& action) {
    auto& state = getState();
    auto it = state.actionStates.find(action);
    if (it == state.actionStates.end()) {
        return false;
    }
    return !it->second.current && it->second.previous;
}

f32 InputManager::getAxisValue(const String& axis) {
    auto& state = getState();
    auto it = state.axisStates.find(axis);
    if (it == state.axisStates.end()) {
        return 0.0f;
    }
    return it->second.value;
}

f32 InputManager::getAxisRawValue(const String& axis) {
    auto& state = getState();
    auto it = state.axisStates.find(axis);
    if (it == state.axisStates.end()) {
        return 0.0f;
    }
    return it->second.rawValue;
}

void InputManager::setContext(InputContext context) {
    getState().currentContext = context;
}

InputContext InputManager::getContext() {
    return getState().currentContext;
}

void InputManager::pushContext(InputContext context) {
    auto& state = getState();
    state.contextStack.push(state.currentContext);
    state.currentContext = context;
}

void InputManager::popContext() {
    auto& state = getState();
    if (!state.contextStack.empty()) {
        state.currentContext = state.contextStack.top();
        state.contextStack.pop();
    }
}

bool InputManager::loadConfig(const std::filesystem::path& path) {
    auto& state = getState();

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("Failed to open input config: {}", path.string());
            return false;
        }

        json config = json::parse(file);

        // Load actions
        if (config.contains("actions")) {
            for (const auto& actionJson : config["actions"]) {
                ActionMapping mapping;
                mapping.name = actionJson["name"].get<String>();

                if (actionJson.contains("context")) {
                    mapping.context = stringToContext(actionJson["context"].get<String>());
                }

                for (const auto& bindingJson : actionJson["bindings"]) {
                    String type = bindingJson["type"].get<String>();

                    if (type == "key") {
                        KeyBinding binding;
                        binding.key = stringToKey(bindingJson["key"].get<String>());

                        if (bindingJson.contains("modifiers")) {
                            for (const auto& mod : bindingJson["modifiers"]) {
                                String modStr = mod.get<String>();
                                if (modStr == "Shift")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Shift;
                                if (modStr == "Control")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Control;
                                if (modStr == "Alt")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Alt;
                            }
                        }

                        mapping.bindings.push_back(binding);
                    } else if (type == "mouse") {
                        MouseButtonBinding binding;
                        binding.button = stringToMouseButton(bindingJson["button"].get<String>());

                        if (bindingJson.contains("modifiers")) {
                            for (const auto& mod : bindingJson["modifiers"]) {
                                String modStr = mod.get<String>();
                                if (modStr == "Shift")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Shift;
                                if (modStr == "Control")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Control;
                                if (modStr == "Alt")
                                    binding.requiredModifiers =
                                        binding.requiredModifiers | Modifier::Alt;
                            }
                        }

                        mapping.bindings.push_back(binding);
                    }
                }

                state.actions[mapping.name] = mapping;
            }
        }

        // Load axes
        if (config.contains("axes")) {
            for (const auto& axisJson : config["axes"]) {
                AxisMapping mapping;
                mapping.name = axisJson["name"].get<String>();

                if (axisJson.contains("context")) {
                    mapping.context = stringToContext(axisJson["context"].get<String>());
                }
                if (axisJson.contains("deadzone")) {
                    mapping.deadzone = axisJson["deadzone"].get<f32>();
                }
                if (axisJson.contains("sensitivity")) {
                    mapping.sensitivity = axisJson["sensitivity"].get<f32>();
                }

                for (const auto& bindingJson : axisJson["bindings"]) {
                    String type = bindingJson["type"].get<String>();

                    if (type == "keys") {
                        KeyAxisBinding binding;
                        binding.positiveKey = stringToKey(bindingJson["positive"].get<String>());
                        binding.negativeKey = stringToKey(bindingJson["negative"].get<String>());
                        mapping.bindings.push_back(binding);
                    } else if (type == "mouse") {
                        MouseAxisBinding binding;
                        String axis = bindingJson["axis"].get<String>();
                        if (axis == "X")
                            binding.axis = MouseAxisBinding::Axis::X;
                        else if (axis == "Y")
                            binding.axis = MouseAxisBinding::Axis::Y;
                        else if (axis == "ScrollX")
                            binding.axis = MouseAxisBinding::Axis::ScrollX;
                        else if (axis == "ScrollY")
                            binding.axis = MouseAxisBinding::Axis::ScrollY;

                        if (bindingJson.contains("sensitivity")) {
                            binding.sensitivity = bindingJson["sensitivity"].get<f32>();
                        }
                        if (bindingJson.contains("inverted")) {
                            binding.inverted = bindingJson["inverted"].get<bool>();
                        }

                        mapping.bindings.push_back(binding);
                    }
                }

                state.axes[mapping.name] = mapping;
            }
        }

        spdlog::info("Loaded input config: {} actions, {} axes", state.actions.size(),
                     state.axes.size());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Failed to parse input config: {}", e.what());
        return false;
    }
}

bool InputManager::saveConfig(const std::filesystem::path& path) {
    auto& state = getState();

    try {
        json config;

        // Save actions
        json actionsJson = json::array();
        for (const auto& [name, mapping] : state.actions) {
            json actionJson;
            actionJson["name"] = mapping.name;
            actionJson["context"] = contextToString(mapping.context);

            json bindingsJson = json::array();
            for (const auto& binding : mapping.bindings) {
                json bindingJson;

                if (const auto* key = std::get_if<KeyBinding>(&binding)) {
                    bindingJson["type"] = "key";
                    bindingJson["key"] = keyToString(key->key);

                    if (key->requiredModifiers != Modifier::None) {
                        json mods = json::array();
                        if (hasModifier(key->requiredModifiers, Modifier::Shift))
                            mods.push_back("Shift");
                        if (hasModifier(key->requiredModifiers, Modifier::Control))
                            mods.push_back("Control");
                        if (hasModifier(key->requiredModifiers, Modifier::Alt))
                            mods.push_back("Alt");
                        bindingJson["modifiers"] = mods;
                    }
                } else if (const auto* mouse = std::get_if<MouseButtonBinding>(&binding)) {
                    bindingJson["type"] = "mouse";
                    bindingJson["button"] = mouseButtonToString(mouse->button);

                    if (mouse->requiredModifiers != Modifier::None) {
                        json mods = json::array();
                        if (hasModifier(mouse->requiredModifiers, Modifier::Shift))
                            mods.push_back("Shift");
                        if (hasModifier(mouse->requiredModifiers, Modifier::Control))
                            mods.push_back("Control");
                        if (hasModifier(mouse->requiredModifiers, Modifier::Alt))
                            mods.push_back("Alt");
                        bindingJson["modifiers"] = mods;
                    }
                }

                bindingsJson.push_back(bindingJson);
            }
            actionJson["bindings"] = bindingsJson;

            actionsJson.push_back(actionJson);
        }
        config["actions"] = actionsJson;

        // Save axes
        json axesJson = json::array();
        for (const auto& [name, mapping] : state.axes) {
            json axisJson;
            axisJson["name"] = mapping.name;
            axisJson["context"] = contextToString(mapping.context);
            axisJson["deadzone"] = mapping.deadzone;
            axisJson["sensitivity"] = mapping.sensitivity;

            json bindingsJson = json::array();
            for (const auto& binding : mapping.bindings) {
                json bindingJson;

                if (const auto* keyAxis = std::get_if<KeyAxisBinding>(&binding)) {
                    bindingJson["type"] = "keys";
                    bindingJson["positive"] = keyToString(keyAxis->positiveKey);
                    bindingJson["negative"] = keyToString(keyAxis->negativeKey);
                } else if (const auto* mouseAxis = std::get_if<MouseAxisBinding>(&binding)) {
                    bindingJson["type"] = "mouse";
                    switch (mouseAxis->axis) {
                        case MouseAxisBinding::Axis::X:
                            bindingJson["axis"] = "X";
                            break;
                        case MouseAxisBinding::Axis::Y:
                            bindingJson["axis"] = "Y";
                            break;
                        case MouseAxisBinding::Axis::ScrollX:
                            bindingJson["axis"] = "ScrollX";
                            break;
                        case MouseAxisBinding::Axis::ScrollY:
                            bindingJson["axis"] = "ScrollY";
                            break;
                    }
                    bindingJson["sensitivity"] = mouseAxis->sensitivity;
                    bindingJson["inverted"] = mouseAxis->inverted;
                }

                bindingsJson.push_back(bindingJson);
            }
            axisJson["bindings"] = bindingsJson;

            axesJson.push_back(axisJson);
        }
        config["axes"] = axesJson;

        std::ofstream file(path);
        if (!file.is_open()) {
            spdlog::error("Failed to create input config: {}", path.string());
            return false;
        }

        file << config.dump(2);
        spdlog::info("Saved input config to: {}", path.string());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Failed to save input config: {}", e.what());
        return false;
    }
}

void InputManager::registerAction(const ActionMapping& mapping) {
    getState().actions[mapping.name] = mapping;
}

void InputManager::registerAxis(const AxisMapping& mapping) {
    getState().axes[mapping.name] = mapping;
}

void InputManager::clearMappings() {
    auto& state = getState();
    state.actions.clear();
    state.axes.clear();
    state.actionStates.clear();
    state.axisStates.clear();
}

std::vector<String> InputManager::getActionNames() {
    std::vector<String> names;
    for (const auto& [name, _] : getState().actions) {
        names.push_back(name);
    }
    return names;
}

std::vector<String> InputManager::getAxisNames() {
    std::vector<String> names;
    for (const auto& [name, _] : getState().axes) {
        names.push_back(name);
    }
    return names;
}

u32 InputManager::addActionCallback(ActionCallback callback) {
    auto& state = getState();
    u32 handle = state.nextCallbackHandle++;
    state.actionCallbacks[handle] = std::move(callback);
    return handle;
}

void InputManager::removeActionCallback(u32 handle) {
    getState().actionCallbacks.erase(handle);
}

u32 InputManager::addAxisCallback(AxisCallback callback) {
    auto& state = getState();
    u32 handle = state.nextCallbackHandle++;
    state.axisCallbacks[handle] = std::move(callback);
    return handle;
}

void InputManager::removeAxisCallback(u32 handle) {
    getState().axisCallbacks.erase(handle);
}

void InputManager::startRebinding(const String& action, u32 bindingIndex) {
    auto& state = getState();
    if (state.actions.find(action) == state.actions.end()) {
        spdlog::warn("Cannot rebind unknown action: {}", action);
        return;
    }
    state.rebinding = true;
    state.rebindingAction = action;
    state.rebindingIndex = bindingIndex;
}

void InputManager::cancelRebinding() {
    auto& state = getState();
    state.rebinding = false;
    state.rebindingAction.clear();
}

bool InputManager::isRebinding() {
    return getState().rebinding;
}

const String& InputManager::getRebindingAction() {
    return getState().rebindingAction;
}

}  // namespace limbo
