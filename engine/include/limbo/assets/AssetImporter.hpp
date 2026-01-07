#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/assets/AssetRegistry.hpp"

#include <filesystem>
#include <functional>
#include <unordered_map>

namespace limbo {

/**
 * Import result with status and optional error message
 */
struct ImportResult {
    bool success = false;
    String error;
    String importedPath;  // Relative path to imported file

    static ImportResult ok(const String& path) {
        return {true, "", path};
    }

    static ImportResult fail(const String& error) {
        return {false, error, ""};
    }
};

/**
 * Import context passed to importers
 */
struct ImportContext {
    AssetRegistry* registry = nullptr;
    AssetId assetId;
    std::filesystem::path sourcePath;      // Absolute path to source file
    std::filesystem::path importedDir;     // Absolute path to imported directory
    const AssetMetadata* metadata = nullptr;
};

/**
 * Base class for asset importers
 */
class LIMBO_API IAssetImporter {
public:
    virtual ~IAssetImporter() = default;

    /**
     * Get the asset type this importer handles
     */
    [[nodiscard]] virtual AssetType getAssetType() const = 0;

    /**
     * Get file extensions this importer handles
     */
    [[nodiscard]] virtual std::vector<String> getSupportedExtensions() const = 0;

    /**
     * Import an asset
     */
    [[nodiscard]] virtual ImportResult import(const ImportContext& context) = 0;

    /**
     * Get default import settings as JSON
     */
    [[nodiscard]] virtual String getDefaultSettings() const { return "{}"; }
};

/**
 * Texture importer - imports image files
 */
class LIMBO_API TextureImporter : public IAssetImporter {
public:
    [[nodiscard]] AssetType getAssetType() const override { return AssetType::Texture; }

    [[nodiscard]] std::vector<String> getSupportedExtensions() const override {
        return {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif"};
    }

    [[nodiscard]] ImportResult import(const ImportContext& context) override;

    [[nodiscard]] String getDefaultSettings() const override;
};

/**
 * Shader importer - imports shader files
 */
class LIMBO_API ShaderImporter : public IAssetImporter {
public:
    [[nodiscard]] AssetType getAssetType() const override { return AssetType::Shader; }

    [[nodiscard]] std::vector<String> getSupportedExtensions() const override {
        return {".glsl", ".vert", ".frag", ".vs", ".fs", ".shader"};
    }

    [[nodiscard]] ImportResult import(const ImportContext& context) override;

    [[nodiscard]] String getDefaultSettings() const override;
};

/**
 * Audio importer - imports audio files
 */
class LIMBO_API AudioImporter : public IAssetImporter {
public:
    [[nodiscard]] AssetType getAssetType() const override { return AssetType::Audio; }

    [[nodiscard]] std::vector<String> getSupportedExtensions() const override {
        return {".wav", ".mp3", ".ogg", ".flac"};
    }

    [[nodiscard]] ImportResult import(const ImportContext& context) override;

    [[nodiscard]] String getDefaultSettings() const override;
};

/**
 * SpriteAtlas importer - imports .atlas definition files
 *
 * Atlas definition files (.atlas.json) specify:
 * - List of source images to pack
 * - Packing configuration (padding, max size, etc.)
 * - Optional per-sprite settings (pivot points, names)
 *
 * The importer builds the atlas texture and metadata.
 */
class LIMBO_API SpriteAtlasImporter : public IAssetImporter {
public:
    [[nodiscard]] AssetType getAssetType() const override { return AssetType::SpriteAtlas; }

    [[nodiscard]] std::vector<String> getSupportedExtensions() const override {
        return {".atlas.json"};
    }

    [[nodiscard]] ImportResult import(const ImportContext& context) override;

    [[nodiscard]] String getDefaultSettings() const override;
};

/**
 * AssetImporter - Manages asset importing
 *
 * Coordinates the import pipeline:
 * 1. Detects asset type from extension
 * 2. Invokes appropriate importer
 * 3. Updates registry with imported path
 * 4. Tracks dependencies
 */
class LIMBO_API AssetImporterManager {
public:
    AssetImporterManager();
    ~AssetImporterManager() = default;

    /**
     * Initialize the importer with a registry
     */
    void init(AssetRegistry& registry);

    /**
     * Import a single asset
     */
    ImportResult importAsset(AssetId id);

    /**
     * Import all assets that need reimporting
     * @return Number of assets imported
     */
    usize importAll();

    /**
     * Import all assets matching a predicate
     */
    usize importWhere(std::function<bool(const AssetMetadata&)> predicate);

    /**
     * Register a custom importer
     */
    void registerImporter(Unique<IAssetImporter> importer);

    /**
     * Get importer for an asset type
     */
    [[nodiscard]] IAssetImporter* getImporter(AssetType type);

    /**
     * Get importer for a file extension
     */
    [[nodiscard]] IAssetImporter* getImporterForExtension(const String& ext);

    /**
     * Callback for import progress
     */
    using ProgressCallback = std::function<void(usize current, usize total, const String& assetPath)>;
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = std::move(callback); }

private:
    AssetRegistry* m_registry = nullptr;
    std::unordered_map<AssetType, Unique<IAssetImporter>> m_importers;
    std::unordered_map<String, IAssetImporter*> m_extensionMap;
    ProgressCallback m_progressCallback;

    void registerDefaultImporters();
};

}  // namespace limbo
