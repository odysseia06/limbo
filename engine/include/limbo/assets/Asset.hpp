#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"

#include <filesystem>

namespace limbo {

/**
 * Asset loading state
 */
enum class AssetState : u8 {
    Unloaded,    // Asset not yet loaded
    Queued,      // Asset queued for async loading
    LoadingIO,   // Asset loading from disk (can be off main thread)
    LoadingGPU,  // Asset uploading to GPU (must be on main thread)
    Loading,     // Asset currently loading (legacy/sync)
    Loaded,      // Asset loaded and ready to use
    Failed       // Asset failed to load
};

/**
 * Asset type identifier
 */
enum class AssetType : u8 {
    Unknown = 0,
    Texture,
    SpriteAtlas,
    Shader,
    Audio,
    // Future types:
    // Mesh,
    // Material,
    // Font,
    // Scene,
};

/**
 * Asset - Base class for all managed assets
 *
 * Assets are resources loaded from disk that can be shared across
 * the engine. They are managed by the AssetManager which handles
 * loading, caching, and hot-reloading.
 */
class LIMBO_API Asset {
public:
    Asset() = default;
    virtual ~Asset() = default;

    // Non-copyable, movable
    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;
    Asset(Asset&&) = default;
    Asset& operator=(Asset&&) = default;

    /**
     * Get the asset's unique ID
     */
    [[nodiscard]] AssetId getId() const { return m_id; }

    /**
     * Get the asset's file path
     */
    [[nodiscard]] const std::filesystem::path& getPath() const { return m_path; }

    /**
     * Get the asset's current loading state
     */
    [[nodiscard]] AssetState getState() const { return m_state; }

    /**
     * Check if the asset is ready to use
     */
    [[nodiscard]] bool isLoaded() const { return m_state == AssetState::Loaded; }

    /**
     * Check if the asset failed to load
     */
    [[nodiscard]] bool hasFailed() const { return m_state == AssetState::Failed; }

    /**
     * Get the asset type
     */
    [[nodiscard]] virtual AssetType getType() const = 0;

    /**
     * Get the error message if loading failed
     */
    [[nodiscard]] const String& getError() const { return m_errorMessage; }

    /**
     * Get all file paths this asset depends on (for hot-reloading)
     * Default implementation returns just the main path
     */
    [[nodiscard]] virtual std::vector<std::filesystem::path> getDependencies() const {
        return {m_path};
    }

protected:
    friend class AssetManager;
    friend class AssetLoader;

    /**
     * Load the asset from disk
     * @return true on success, false on failure
     */
    virtual bool load() = 0;

    /**
     * Unload the asset and free resources
     */
    virtual void unload() = 0;

    /**
     * Reload the asset (for hot-reloading)
     * @return true on success, false on failure
     */
    virtual bool reload() {
        unload();
        return load();
    }

    void setId(AssetId id) { m_id = id; }
    void setPath(const std::filesystem::path& path) { m_path = path; }
    void setState(AssetState state) { m_state = state; }
    void setError(const String& error) { m_errorMessage = error; }

private:
    AssetId m_id;
    std::filesystem::path m_path;
    AssetState m_state = AssetState::Unloaded;
    String m_errorMessage;
};

}  // namespace limbo
