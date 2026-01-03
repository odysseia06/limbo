#include "EditorApp.hpp"

#include <spdlog/spdlog.h>

int main() {
    // Initialize debug/logging
    limbo::debug::init();

    limbo::editor::EditorApp editor;

    limbo::ApplicationConfig config;
    config.appName = "Limbo Editor";
    config.window.title = "Limbo Editor";
    config.window.width = 1600;
    config.window.height = 900;
    config.window.resizable = true;

    auto result = editor.init(config);
    if (!result) {
        spdlog::critical("Failed to initialize editor: {}", result.error());
        limbo::debug::shutdown();
        return 1;
    }

    editor.run();
    editor.shutdown();

    limbo::debug::shutdown();
    return 0;
}
