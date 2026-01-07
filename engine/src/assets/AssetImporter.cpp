#include "limbo/assets/AssetImporter.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>

namespace limbo {

using json = nlohmann::json;

// ============================================================================
// TextureImporter
// ============================================================================

ImportResult TextureImporter::import(const ImportContext& context) {
    // For now, textures are loaded directly at runtime without pre-processing
    // In a more advanced pipeline, we might:
    // - Generate mipmaps
    // - Convert to compressed formats (BC1-7, ETC, ASTC)
    // - Create texture atlases
    // - Apply import settings

    // Create imported directory structure
    std::filesystem::path importedPath = context.importedDir / "textures";
    std::filesystem::create_directories(importedPath);

    // Generate imported filename using asset ID
    String filename = context.assetId.uuid().toCompactString() + ".tex";
    std::filesystem::path outputPath = importedPath / filename;

    // For now, just copy the source file
    // In a real pipeline, we'd process and write a custom format
    try {
        std::filesystem::copy_file(context.sourcePath, outputPath,
                                   std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        return ImportResult::fail(String("Failed to copy texture: ") + e.what());
    }

    // Return relative path from imported dir
    String relativePath = "textures/" + filename;
    spdlog::debug("Imported texture: {} -> {}", context.sourcePath.string(), relativePath);

    return ImportResult::ok(relativePath);
}

String TextureImporter::getDefaultSettings() const {
    json settings;
    settings["generateMipmaps"] = true;
    settings["sRGB"] = true;
    settings["premultiplyAlpha"] = false;
    settings["maxSize"] = 4096;
    settings["filterMode"] = "linear";
    settings["wrapMode"] = "repeat";
    return settings.dump();
}

// ============================================================================
// ShaderImporter
// ============================================================================

ImportResult ShaderImporter::import(const ImportContext& context) {
    // Create imported directory structure
    std::filesystem::path importedPath = context.importedDir / "shaders";
    std::filesystem::create_directories(importedPath);

    // Generate imported filename using asset ID
    String filename = context.assetId.uuid().toCompactString() + ".shader";
    std::filesystem::path outputPath = importedPath / filename;

    // For now, just copy the source file
    // In a real pipeline, we might:
    // - Precompile to SPIR-V
    // - Validate shader syntax
    // - Extract uniform metadata
    try {
        std::filesystem::copy_file(context.sourcePath, outputPath,
                                   std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        return ImportResult::fail(String("Failed to copy shader: ") + e.what());
    }

    String relativePath = "shaders/" + filename;
    spdlog::debug("Imported shader: {} -> {}", context.sourcePath.string(), relativePath);

    return ImportResult::ok(relativePath);
}

String ShaderImporter::getDefaultSettings() const {
    json settings;
    settings["precompile"] = false;
    settings["optimize"] = true;
    return settings.dump();
}

// ============================================================================
// AudioImporter
// ============================================================================

ImportResult AudioImporter::import(const ImportContext& context) {
    // Create imported directory structure
    std::filesystem::path importedPath = context.importedDir / "audio";
    std::filesystem::create_directories(importedPath);

    // Generate imported filename using asset ID
    String filename = context.assetId.uuid().toCompactString() + ".audio";
    std::filesystem::path outputPath = importedPath / filename;

    // For now, just copy the source file
    // In a real pipeline, we might:
    // - Convert to a consistent format
    // - Compress audio
    // - Extract metadata (duration, sample rate, etc.)
    try {
        std::filesystem::copy_file(context.sourcePath, outputPath,
                                   std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        return ImportResult::fail(String("Failed to copy audio: ") + e.what());
    }

    String relativePath = "audio/" + filename;
    spdlog::debug("Imported audio: {} -> {}", context.sourcePath.string(), relativePath);

    return ImportResult::ok(relativePath);
}

String AudioImporter::getDefaultSettings() const {
    json settings;
    settings["streaming"] = false;
    settings["quality"] = 1.0;
    return settings.dump();
}

// ============================================================================
// AssetImporterManager
// ============================================================================

AssetImporterManager::AssetImporterManager() {
    registerDefaultImporters();
}

void AssetImporterManager::init(AssetRegistry& registry) {
    m_registry = &registry;
}

void AssetImporterManager::registerDefaultImporters() {
    registerImporter(make_unique<TextureImporter>());
    registerImporter(make_unique<ShaderImporter>());
    registerImporter(make_unique<AudioImporter>());
}

void AssetImporterManager::registerImporter(Unique<IAssetImporter> importer) {
    AssetType type = importer->getAssetType();
    IAssetImporter* ptr = importer.get();

    // Register by type
    m_importers[type] = std::move(importer);

    // Register by extension
    for (const auto& ext : ptr->getSupportedExtensions()) {
        String lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
        m_extensionMap[lowerExt] = ptr;
    }
}

IAssetImporter* AssetImporterManager::getImporter(AssetType type) {
    auto it = m_importers.find(type);
    if (it != m_importers.end()) {
        return it->second.get();
    }
    return nullptr;
}

IAssetImporter* AssetImporterManager::getImporterForExtension(const String& ext) {
    String lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

    auto it = m_extensionMap.find(lowerExt);
    if (it != m_extensionMap.end()) {
        return it->second;
    }
    return nullptr;
}

ImportResult AssetImporterManager::importAsset(AssetId id) {
    if (!m_registry) {
        return ImportResult::fail("AssetImporterManager not initialized");
    }

    AssetMetadata* metadata = m_registry->getMetadata(id);
    if (!metadata) {
        return ImportResult::fail("Asset not found in registry");
    }

    // Get importer for this asset type
    IAssetImporter* importer = getImporter(metadata->type);
    if (!importer) {
        return ImportResult::fail("No importer registered for asset type");
    }

    // Build import context
    ImportContext context;
    context.registry = m_registry;
    context.assetId = id;
    context.sourcePath = m_registry->getSourcePath(id);
    context.importedDir = m_registry->getImportedDir();
    context.metadata = metadata;

    // Check if source exists
    if (!std::filesystem::exists(context.sourcePath)) {
        return ImportResult::fail("Source file not found: " + context.sourcePath.string());
    }

    // Perform import
    ImportResult result = importer->import(context);

    if (result.success) {
        // Update registry
        u64 sourceHash = AssetRegistry::computeFileHash(context.sourcePath);
        m_registry->updateSourceHash(id, sourceHash);
        m_registry->markAsImported(id, result.importedPath);
    } else {
        spdlog::error("Failed to import asset {}: {}", metadata->sourcePath, result.error);
    }

    return result;
}

usize AssetImporterManager::importAll() {
    if (!m_registry) {
        return 0;
    }

    std::vector<AssetId> assetsToImport = m_registry->getAssetsNeedingReimport();
    usize total = assetsToImport.size();
    usize imported = 0;

    for (usize i = 0; i < assetsToImport.size(); ++i) {
        AssetId id = assetsToImport[i];
        const AssetMetadata* metadata = m_registry->getMetadata(id);

        if (m_progressCallback && metadata) {
            m_progressCallback(i + 1, total, metadata->sourcePath);
        }

        ImportResult result = importAsset(id);
        if (result.success) {
            ++imported;
        }
    }

    if (imported > 0) {
        m_registry->save();
    }

    spdlog::info("Imported {} of {} assets", imported, total);
    return imported;
}

usize AssetImporterManager::importWhere(std::function<bool(const AssetMetadata&)> predicate) {
    if (!m_registry) {
        return 0;
    }

    std::vector<AssetId> allAssets = m_registry->getAllAssetIds();
    std::vector<AssetId> assetsToImport;

    for (AssetId id : allAssets) {
        const AssetMetadata* metadata = m_registry->getMetadata(id);
        if (metadata && predicate(*metadata)) {
            assetsToImport.push_back(id);
        }
    }

    usize total = assetsToImport.size();
    usize imported = 0;

    for (usize i = 0; i < assetsToImport.size(); ++i) {
        AssetId id = assetsToImport[i];
        const AssetMetadata* metadata = m_registry->getMetadata(id);

        if (m_progressCallback && metadata) {
            m_progressCallback(i + 1, total, metadata->sourcePath);
        }

        ImportResult result = importAsset(id);
        if (result.success) {
            ++imported;
        }
    }

    if (imported > 0) {
        m_registry->save();
    }

    return imported;
}

}  // namespace limbo
