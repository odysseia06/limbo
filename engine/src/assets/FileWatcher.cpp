#include "limbo/assets/FileWatcher.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

void FileWatcher::watch(const std::filesystem::path& path, Callback callback) {
    if (!std::filesystem::exists(path)) {
        spdlog::warn("FileWatcher: Cannot watch non-existent file: {}", path.string());
        return;
    }

    String key = path.generic_string();

    WatchedFile watched;
    watched.path = path;
    watched.callback = std::move(callback);
    watched.lastModified = std::filesystem::last_write_time(path);
    watched.lastChecked = Clock::now();

    m_watchedFiles[key] = std::move(watched);
    spdlog::debug("FileWatcher: Now watching: {}", path.string());
}

void FileWatcher::unwatch(const std::filesystem::path& path) {
    String key = path.generic_string();
    auto it = m_watchedFiles.find(key);
    if (it != m_watchedFiles.end()) {
        m_watchedFiles.erase(it);
        spdlog::debug("FileWatcher: Stopped watching: {}", path.string());
    }
}

void FileWatcher::unwatchAll() {
    m_watchedFiles.clear();
    spdlog::debug("FileWatcher: Stopped watching all files");
}

void FileWatcher::poll() {
    auto now = Clock::now();

    for (auto& [key, watched] : m_watchedFiles) {
        // Skip if we checked too recently
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - watched.lastChecked);
        if (elapsed < m_pollInterval) {
            continue;
        }

        watched.lastChecked = now;

        // Check if file still exists
        if (!std::filesystem::exists(watched.path)) {
            continue;
        }

        // Check modification time
        std::error_code ec;
        auto currentModTime = std::filesystem::last_write_time(watched.path, ec);
        if (ec) {
            continue;
        }

        if (currentModTime != watched.lastModified) {
            watched.lastModified = currentModTime;
            spdlog::info("FileWatcher: File changed: {}", watched.path.string());

            // Trigger callback
            if (watched.callback) {
                watched.callback(watched.path);
            }
        }
    }
}

bool FileWatcher::isWatching(const std::filesystem::path& path) const {
    String key = path.generic_string();
    return m_watchedFiles.find(key) != m_watchedFiles.end();
}

}  // namespace limbo
