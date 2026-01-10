#include "limbo/assets/FileWatcher.hpp"

#include "limbo/debug/Log.hpp"

namespace limbo {

void FileWatcher::watch(const std::filesystem::path& path, Callback callback) {
    if (!std::filesystem::exists(path)) {
        LIMBO_LOG_ASSET_WARN("FileWatcher: Cannot watch non-existent file: {}", path.string());
        return;
    }

    String const key = path.generic_string();

    WatchedFile watched;
    watched.path = path;
    watched.callback = std::move(callback);
    watched.lastModified = std::filesystem::last_write_time(path);
    watched.lastChecked = Clock::now();

    m_watchedFiles[key] = std::move(watched);
    LIMBO_LOG_ASSET_DEBUG("FileWatcher: Now watching: {}", path.string());
}

void FileWatcher::unwatch(const std::filesystem::path& path) {
    String const key = path.generic_string();
    auto it = m_watchedFiles.find(key);
    if (it != m_watchedFiles.end()) {
        m_watchedFiles.erase(it);
        LIMBO_LOG_ASSET_DEBUG("FileWatcher: Stopped watching: {}", path.string());
    }
}

void FileWatcher::unwatchAll() {
    m_watchedFiles.clear();
    LIMBO_LOG_ASSET_DEBUG("FileWatcher: Stopped watching all files");
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
            LIMBO_LOG_ASSET_INFO("FileWatcher: File changed: {}", watched.path.string());

            // Trigger callback
            if (watched.callback) {
                watched.callback(watched.path);
            }
        }
    }
}

bool FileWatcher::isWatching(const std::filesystem::path& path) const {
    String const key = path.generic_string();
    return m_watchedFiles.contains(key);
}

}  // namespace limbo
