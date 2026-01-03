#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace limbo {

/**
 * FileWatcher - Monitors files for changes and triggers callbacks
 *
 * Uses polling-based file modification time checking for cross-platform
 * compatibility. Call poll() regularly (e.g., once per frame) to check
 * for file changes.
 *
 * Usage:
 *   FileWatcher watcher;
 *   watcher.watch("assets/texture.png", [](const auto& path) {
 *       // Reload texture
 *   });
 *   
 *   // In update loop:
 *   watcher.poll();
 */
class LIMBO_API FileWatcher {
public:
    using Callback = std::function<void(const std::filesystem::path&)>;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    FileWatcher() = default;
    ~FileWatcher() = default;

    // Non-copyable
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /**
     * Start watching a file for changes
     * @param path Path to the file to watch
     * @param callback Function to call when file changes
     */
    void watch(const std::filesystem::path& path, Callback callback);

    /**
     * Stop watching a file
     * @param path Path to stop watching
     */
    void unwatch(const std::filesystem::path& path);

    /**
     * Stop watching all files
     */
    void unwatchAll();

    /**
     * Check for file changes and trigger callbacks
     * Call this regularly (e.g., once per frame or on a timer)
     */
    void poll();

    /**
     * Set the minimum interval between checks for the same file
     * Default is 500ms to avoid excessive disk access
     */
    void setPollInterval(std::chrono::milliseconds interval) {
        m_pollInterval = interval;
    }

    /**
     * Get the number of files being watched
     */
    [[nodiscard]] usize watchCount() const { return m_watchedFiles.size(); }

    /**
     * Check if a file is being watched
     */
    [[nodiscard]] bool isWatching(const std::filesystem::path& path) const;

private:
    struct WatchedFile {
        std::filesystem::path path;
        Callback callback;
        std::filesystem::file_time_type lastModified;
        TimePoint lastChecked;
    };

    std::unordered_map<String, WatchedFile> m_watchedFiles;
    std::chrono::milliseconds m_pollInterval{500};
};

} // namespace limbo
