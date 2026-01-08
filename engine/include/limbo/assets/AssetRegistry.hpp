#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/Asset.hpp"
#include "limbo/assets/AssetId.hpp"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace limbo {

/**
 * Import settings for different asset types
 */
struct TextureImportSettings {
    bool generateMipmaps = true;
    bool sRGB = true;
    bool premultiplyAlpha = false;
    i32 maxSize = 4096;
    String filterMode = "linear";  // "nearest", "linear"
    String wrapMode = "repeat";    // "repeat", "clamp", "mirror"
};

struct AudioImportSettings {
    bool streaming = false;
    f32 quality = 1.0f;  // 0.0 to 1.0
};

struct SpriteSheetImportSettings {
    i32 spriteWidth = 0;  // 0 = auto-detect or manual regions
    i32 spriteHeight = 0;
    i32 padding = 0;
    i32 spacing = 0;
};

/**
 * AssetMetadata - Stores metadata about a source asset
 */
struct AssetMetadata {
    AssetId id;           // Stable UUID
    String sourcePath;    // Relative path to source file
    String importedPath;  // Relative path to imported/cooked file
    AssetType type = AssetType::Unknown;
    u64 sourceHash = 0;                 // Hash of source file for change detection
    u64 importedTimestamp = 0;          // When the asset was last imported
    std::vector<AssetId> dependencies;  // Assets this asset depends on
    std::vector<AssetId> dependents;    // Assets that depend on this asset

    // Type-specific import settings (stored as variant or JSON)
    String importSettingsJson;

    /**
     * Check if the asset needs re-import
     */
    [[nodiscard]] bool needsReimport(u64 currentSourceHash) const {
        return sourceHash != currentSourceHash || importedPath.empty();
    }
};

/**
 * AssetRegistry - Central database of all assets in the project
 *
 * The registry maintains:
 * - Stable UUID for each asset
 * - Mapping between UUIDs and file paths
 * - Import settings and metadata
 * - Dependency tracking for hot-reload
 *
 * The registry is persisted to disk (typically as a JSON file) and
 * survives renames and moves of assets.
 */
class LIMBO_API AssetRegistry {
public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    // Non-copyable
    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    /**
     * Initialize the registry with project paths
     * @param projectRoot Root directory of the project
     * @param sourceDir Source assets directory (relative to projectRoot)
     * @param importedDir Imported/cooked assets directory (relative to projectRoot)
     */
    void init(const std::filesystem::path& projectRoot, const String& sourceDir = "assets",
              const String& importedDir = "build/imported");

    /**
     * Load the registry from disk
     */
    bool load();

    /**
     * Save the registry to disk
     */
    bool save() const;

    /**
     * Get the registry file path
     */
    [[nodiscard]] std::filesystem::path getRegistryPath() const;

    // ========================================================================
    // Asset Registration
    // ========================================================================

    /**
     * Register a new asset (creates a new UUID)
     * @param sourcePath Relative path to the source file
     * @param type Asset type
     * @return The new asset's ID
     */
    AssetId registerAsset(const String& sourcePath, AssetType type);

    /**
     * Unregister an asset
     */
    void unregisterAsset(AssetId id);

    /**
     * Check if an asset is registered
     */
    [[nodiscard]] bool isRegistered(AssetId id) const;

    /**
     * Check if a source path is registered
     */
    [[nodiscard]] bool isPathRegistered(const String& sourcePath) const;

    // ========================================================================
    // Asset Lookup
    // ========================================================================

    /**
     * Get asset metadata by ID
     * @return Pointer to metadata, or nullptr if not found
     */
    [[nodiscard]] AssetMetadata* getMetadata(AssetId id);
    [[nodiscard]] const AssetMetadata* getMetadata(AssetId id) const;

    /**
     * Get asset ID by source path
     * @return Asset ID, or invalid ID if not found
     */
    [[nodiscard]] AssetId getIdByPath(const String& sourcePath) const;

    /**
     * Get all registered asset IDs
     */
    [[nodiscard]] std::vector<AssetId> getAllAssetIds() const;

    /**
     * Get all assets of a specific type
     */
    [[nodiscard]] std::vector<AssetId> getAssetsByType(AssetType type) const;

    // ========================================================================
    // Path Resolution
    // ========================================================================

    /**
     * Get absolute path to a source asset
     */
    [[nodiscard]] std::filesystem::path getSourcePath(AssetId id) const;

    /**
     * Get absolute path to an imported asset
     */
    [[nodiscard]] std::filesystem::path getImportedPath(AssetId id) const;

    /**
     * Get the project root directory
     */
    [[nodiscard]] const std::filesystem::path& getProjectRoot() const { return m_projectRoot; }

    /**
     * Get the source assets directory
     */
    [[nodiscard]] std::filesystem::path getSourceDir() const { return m_projectRoot / m_sourceDir; }

    /**
     * Get the imported assets directory
     */
    [[nodiscard]] std::filesystem::path getImportedDir() const {
        return m_projectRoot / m_importedDir;
    }

    // ========================================================================
    // Dependency Tracking
    // ========================================================================

    /**
     * Add a dependency between assets
     * @param assetId The asset that depends on another
     * @param dependencyId The asset being depended upon
     */
    void addDependency(AssetId assetId, AssetId dependencyId);

    /**
     * Remove a dependency
     */
    void removeDependency(AssetId assetId, AssetId dependencyId);

    /**
     * Clear all dependencies for an asset
     */
    void clearDependencies(AssetId assetId);

    /**
     * Get all assets that depend on a given asset
     */
    [[nodiscard]] std::vector<AssetId> getDependents(AssetId id) const;

    /**
     * Get all assets that a given asset depends on
     */
    [[nodiscard]] std::vector<AssetId> getDependencies(AssetId id) const;

    // ========================================================================
    // Import State
    // ========================================================================

    /**
     * Update the source hash for an asset
     */
    void updateSourceHash(AssetId id, u64 hash);

    /**
     * Update the imported path and timestamp
     */
    void markAsImported(AssetId id, const String& importedPath);

    /**
     * Get all assets that need re-import
     */
    [[nodiscard]] std::vector<AssetId> getAssetsNeedingReimport() const;

    /**
     * Compute hash of a file for change detection
     */
    [[nodiscard]] static u64 computeFileHash(const std::filesystem::path& path);

    // ========================================================================
    // Scanning
    // ========================================================================

    /**
     * Scan source directory for new/changed/deleted assets
     * @return Number of changes detected
     */
    usize scanSourceDirectory();

    /**
     * Get assets detected as new (not yet registered)
     */
    [[nodiscard]] const std::vector<String>& getNewAssets() const { return m_newAssets; }

    /**
     * Get assets detected as deleted (registered but file missing)
     */
    [[nodiscard]] const std::vector<AssetId>& getDeletedAssets() const { return m_deletedAssets; }

    /**
     * Get assets detected as modified (hash changed)
     */
    [[nodiscard]] const std::vector<AssetId>& getModifiedAssets() const { return m_modifiedAssets; }

private:
    std::filesystem::path m_projectRoot;
    String m_sourceDir = "assets";
    String m_importedDir = "build/imported";

    std::unordered_map<AssetId, AssetMetadata> m_assets;
    std::unordered_map<String, AssetId> m_pathToId;

    // Scan results
    std::vector<String> m_newAssets;
    std::vector<AssetId> m_deletedAssets;
    std::vector<AssetId> m_modifiedAssets;

    /**
     * Detect asset type from file extension
     */
    [[nodiscard]] static AssetType detectAssetType(const std::filesystem::path& path);
};

}  // namespace limbo
