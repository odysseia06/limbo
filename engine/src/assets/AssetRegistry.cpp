#include "limbo/assets/AssetRegistry.hpp"
#include "limbo/debug/Log.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>

namespace limbo {

using json = nlohmann::json;

namespace {

// FNV-1a hash for file content
u64 fnv1aHash(const u8* data, usize size) {
    constexpr u64 kFnvOffset = 14695981039346656037ULL;
    constexpr u64 kFnvPrime = 1099511628211ULL;

    u64 hash = kFnvOffset;
    for (usize i = 0; i < size; ++i) {
        hash ^= static_cast<u64>(data[i]);
        hash *= kFnvPrime;
    }
    return hash;
}

String assetTypeToString(AssetType type) {
    switch (type) {
    case AssetType::Texture:
        return "texture";
    case AssetType::Shader:
        return "shader";
    case AssetType::Audio:
        return "audio";
    default:
        return "unknown";
    }
}

AssetType stringToAssetType(const String& str) {
    if (str == "texture")
        return AssetType::Texture;
    if (str == "shader")
        return AssetType::Shader;
    if (str == "audio")
        return AssetType::Audio;
    return AssetType::Unknown;
}

u64 getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}

}  // namespace

void AssetRegistry::init(const std::filesystem::path& projectRoot, const String& sourceDir,
                         const String& importedDir) {
    m_projectRoot = projectRoot;
    m_sourceDir = sourceDir;
    m_importedDir = importedDir;

    // Create imported directory if it doesn't exist
    std::filesystem::path importedPath = m_projectRoot / m_importedDir;
    if (!std::filesystem::exists(importedPath)) {
        std::filesystem::create_directories(importedPath);
    }

    LIMBO_LOG_ASSET_DEBUG("AssetRegistry initialized: project={}, source={}, imported={}",
                          m_projectRoot.string(), m_sourceDir, m_importedDir);
}

std::filesystem::path AssetRegistry::getRegistryPath() const {
    return m_projectRoot / m_importedDir / "asset_registry.json";
}

bool AssetRegistry::load() {
    std::filesystem::path registryPath = getRegistryPath();

    if (!std::filesystem::exists(registryPath)) {
        LIMBO_LOG_ASSET_INFO("Asset registry not found, creating new: {}", registryPath.string());
        return true;  // Not an error, just empty
    }

    try {
        std::ifstream file(registryPath);
        if (!file.is_open()) {
            LIMBO_LOG_ASSET_ERROR("Failed to open asset registry: {}", registryPath.string());
            return false;
        }

        json data = json::parse(file);

        m_assets.clear();
        m_pathToId.clear();

        if (data.contains("assets")) {
            for (const auto& assetJson : data["assets"]) {
                AssetMetadata metadata;
                metadata.id = AssetId::fromString(assetJson["id"].get<String>());
                metadata.sourcePath = assetJson["sourcePath"].get<String>();
                metadata.importedPath = assetJson.value("importedPath", "");
                metadata.type = stringToAssetType(assetJson.value("type", "unknown"));
                metadata.sourceHash = assetJson.value("sourceHash", 0ULL);
                metadata.sourceModTime = assetJson.value("sourceModTime", 0ULL);
                metadata.sourceSize = assetJson.value("sourceSize", 0ULL);
                metadata.importedTimestamp = assetJson.value("importedTimestamp", 0ULL);
                metadata.importSettingsJson = assetJson.value("importSettings", "{}");

                if (assetJson.contains("dependencies")) {
                    for (const auto& depId : assetJson["dependencies"]) {
                        metadata.dependencies.push_back(AssetId::fromString(depId.get<String>()));
                    }
                }

                m_assets[metadata.id] = metadata;
                m_pathToId[metadata.sourcePath] = metadata.id;
            }
        }

        // Rebuild dependents from dependencies
        for (auto& [id, metadata] : m_assets) {
            for (const auto& depId : metadata.dependencies) {
                auto it = m_assets.find(depId);
                if (it != m_assets.end()) {
                    it->second.dependents.push_back(id);
                }
            }
        }

        LIMBO_LOG_ASSET_INFO("Asset registry loaded: {} assets", m_assets.size());
        return true;

    } catch (const std::exception& e) {
        LIMBO_LOG_ASSET_ERROR("Failed to parse asset registry: {}", e.what());
        return false;
    }
}

bool AssetRegistry::save() const {
    std::filesystem::path registryPath = getRegistryPath();

    // Ensure directory exists
    std::filesystem::create_directories(registryPath.parent_path());

    try {
        json data;
        json assetsArray = json::array();

        for (const auto& [id, metadata] : m_assets) {
            json assetJson;
            assetJson["id"] = metadata.id.toString();
            assetJson["sourcePath"] = metadata.sourcePath;
            assetJson["importedPath"] = metadata.importedPath;
            assetJson["type"] = assetTypeToString(metadata.type);
            assetJson["sourceHash"] = metadata.sourceHash;
            assetJson["sourceModTime"] = metadata.sourceModTime;
            assetJson["sourceSize"] = metadata.sourceSize;
            assetJson["importedTimestamp"] = metadata.importedTimestamp;
            assetJson["importSettings"] = metadata.importSettingsJson;

            json depsArray = json::array();
            for (const auto& depId : metadata.dependencies) {
                depsArray.push_back(depId.toString());
            }
            assetJson["dependencies"] = depsArray;

            assetsArray.push_back(assetJson);
        }

        data["assets"] = assetsArray;
        data["version"] = 1;

        std::ofstream file(registryPath);
        if (!file.is_open()) {
            LIMBO_LOG_ASSET_ERROR("Failed to create asset registry: {}", registryPath.string());
            return false;
        }

        file << data.dump(2);
        LIMBO_LOG_ASSET_DEBUG("Asset registry saved: {} assets", m_assets.size());
        return true;

    } catch (const std::exception& e) {
        LIMBO_LOG_ASSET_ERROR("Failed to save asset registry: {}", e.what());
        return false;
    }
}

AssetId AssetRegistry::registerAsset(const String& sourcePath, AssetType type) {
    // Check if already registered
    auto it = m_pathToId.find(sourcePath);
    if (it != m_pathToId.end()) {
        return it->second;
    }

    // Generate new UUID
    AssetId id = AssetId::generate();

    AssetMetadata metadata;
    metadata.id = id;
    metadata.sourcePath = sourcePath;
    metadata.type = type;

    // Store initial file metadata for fast change detection
    std::filesystem::path fullPath = m_projectRoot / m_sourceDir / sourcePath;
    u64 modTime = 0;
    u64 fileSize = 0;
    if (getFileMetadata(fullPath, modTime, fileSize)) {
        metadata.sourceModTime = modTime;
        metadata.sourceSize = fileSize;
        // Note: We don't compute the full hash here - it's done during import
        // This keeps registration fast for large files
    }

    m_assets[id] = metadata;
    m_pathToId[sourcePath] = id;

    LIMBO_LOG_ASSET_DEBUG("Registered asset: {} -> {}", sourcePath, id.toString());
    return id;
}

void AssetRegistry::unregisterAsset(AssetId id) {
    auto it = m_assets.find(id);
    if (it == m_assets.end()) {
        return;
    }

    // Remove from path mapping
    m_pathToId.erase(it->second.sourcePath);

    // Remove from dependents of dependencies
    for (const auto& depId : it->second.dependencies) {
        auto depIt = m_assets.find(depId);
        if (depIt != m_assets.end()) {
            auto& deps = depIt->second.dependents;
            deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
        }
    }

    // Remove from dependencies of dependents
    for (const auto& depId : it->second.dependents) {
        auto depIt = m_assets.find(depId);
        if (depIt != m_assets.end()) {
            auto& deps = depIt->second.dependencies;
            deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
        }
    }

    m_assets.erase(it);
    LIMBO_LOG_ASSET_DEBUG("Unregistered asset: {}", id.toString());
}

bool AssetRegistry::isRegistered(AssetId id) const {
    return m_assets.contains(id);
}

bool AssetRegistry::isPathRegistered(const String& sourcePath) const {
    return m_pathToId.contains(sourcePath);
}

AssetMetadata* AssetRegistry::getMetadata(AssetId id) {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return &it->second;
    }
    return nullptr;
}

const AssetMetadata* AssetRegistry::getMetadata(AssetId id) const {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return &it->second;
    }
    return nullptr;
}

AssetId AssetRegistry::getIdByPath(const String& sourcePath) const {
    auto it = m_pathToId.find(sourcePath);
    if (it != m_pathToId.end()) {
        return it->second;
    }
    return AssetId::invalid();
}

std::vector<AssetId> AssetRegistry::getAllAssetIds() const {
    std::vector<AssetId> ids;
    ids.reserve(m_assets.size());
    for (const auto& [id, _] : m_assets) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<AssetId> AssetRegistry::getAssetsByType(AssetType type) const {
    std::vector<AssetId> ids;
    for (const auto& [id, metadata] : m_assets) {
        if (metadata.type == type) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::filesystem::path AssetRegistry::getSourcePath(AssetId id) const {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return m_projectRoot / m_sourceDir / it->second.sourcePath;
    }
    return {};
}

std::filesystem::path AssetRegistry::getImportedPath(AssetId id) const {
    auto it = m_assets.find(id);
    if (it != m_assets.end() && !it->second.importedPath.empty()) {
        return m_projectRoot / m_importedDir / it->second.importedPath;
    }
    return {};
}

void AssetRegistry::addDependency(AssetId assetId, AssetId dependencyId) {
    auto it = m_assets.find(assetId);
    if (it == m_assets.end()) {
        return;
    }

    // Add to dependencies
    auto& deps = it->second.dependencies;
    if (std::find(deps.begin(), deps.end(), dependencyId) == deps.end()) {
        deps.push_back(dependencyId);
    }

    // Add to dependents of the dependency
    auto depIt = m_assets.find(dependencyId);
    if (depIt != m_assets.end()) {
        auto& dependents = depIt->second.dependents;
        if (std::find(dependents.begin(), dependents.end(), assetId) == dependents.end()) {
            dependents.push_back(assetId);
        }
    }
}

void AssetRegistry::removeDependency(AssetId assetId, AssetId dependencyId) {
    auto it = m_assets.find(assetId);
    if (it != m_assets.end()) {
        auto& deps = it->second.dependencies;
        deps.erase(std::remove(deps.begin(), deps.end(), dependencyId), deps.end());
    }

    auto depIt = m_assets.find(dependencyId);
    if (depIt != m_assets.end()) {
        auto& dependents = depIt->second.dependents;
        dependents.erase(std::remove(dependents.begin(), dependents.end(), assetId),
                         dependents.end());
    }
}

void AssetRegistry::clearDependencies(AssetId assetId) {
    auto it = m_assets.find(assetId);
    if (it == m_assets.end()) {
        return;
    }

    // Remove this asset from dependents lists
    for (const auto& depId : it->second.dependencies) {
        auto depIt = m_assets.find(depId);
        if (depIt != m_assets.end()) {
            auto& dependents = depIt->second.dependents;
            dependents.erase(std::remove(dependents.begin(), dependents.end(), assetId),
                             dependents.end());
        }
    }

    it->second.dependencies.clear();
}

std::vector<AssetId> AssetRegistry::getDependents(AssetId id) const {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return it->second.dependents;
    }
    return {};
}

std::vector<AssetId> AssetRegistry::getDependencies(AssetId id) const {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        return it->second.dependencies;
    }
    return {};
}

void AssetRegistry::updateSourceHash(AssetId id, u64 hash) {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        it->second.sourceHash = hash;
    }
}

void AssetRegistry::updateSourceMetadata(AssetId id, u64 modTime, u64 size) {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        it->second.sourceModTime = modTime;
        it->second.sourceSize = size;
    }
}

void AssetRegistry::markAsImported(AssetId id, const String& importedPath) {
    auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        it->second.importedPath = importedPath;
        it->second.importedTimestamp = getCurrentTimestamp();
    }
}

std::vector<AssetId> AssetRegistry::getAssetsNeedingReimport() const {
    std::vector<AssetId> result;

    for (const auto& [id, metadata] : m_assets) {
        std::filesystem::path sourcePath = m_projectRoot / m_sourceDir / metadata.sourcePath;

        // Quick check: if not imported yet, definitely needs import
        if (metadata.importedPath.empty()) {
            if (std::filesystem::exists(sourcePath)) {
                result.push_back(id);
            }
            continue;
        }

        // Use fast metadata check first (mod time + size)
        u64 modTime = 0;
        u64 fileSize = 0;
        if (!getFileMetadata(sourcePath, modTime, fileSize)) {
            continue;  // Source missing, can't reimport
        }

        // If metadata changed, asset needs reimport
        if (metadata.metadataChanged(modTime, fileSize)) {
            result.push_back(id);
        }
    }

    return result;
}

u64 AssetRegistry::computeFileHash(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 0;
    }

    auto size = file.tellg();
    if (size <= 0) {
        return 0;
    }

    file.seekg(0);

    std::vector<u8> buffer(static_cast<usize>(size));
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    return fnv1aHash(buffer.data(), buffer.size());
}

bool AssetRegistry::getFileMetadata(const std::filesystem::path& path, u64& outModTime,
                                    u64& outSize) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec) {
        return false;
    }

    auto fileSize = std::filesystem::file_size(path, ec);
    if (ec) {
        return false;
    }

    auto modTime = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return false;
    }

    outSize = static_cast<u64>(fileSize);
    outModTime = static_cast<u64>(modTime.time_since_epoch().count());

    return true;
}

usize AssetRegistry::scanSourceDirectory() {
    m_newAssets.clear();
    m_deletedAssets.clear();
    m_modifiedAssets.clear();

    std::filesystem::path sourceDir = m_projectRoot / m_sourceDir;

    if (!std::filesystem::exists(sourceDir)) {
        LIMBO_LOG_ASSET_WARN("Source directory does not exist: {}", sourceDir.string());
        return 0;
    }

    // Track which registered assets we've seen
    std::unordered_set<AssetId> seenAssets;

    // Scan directory recursively
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        // Get relative path
        String relativePath = std::filesystem::relative(entry.path(), sourceDir).generic_string();

        // Skip hidden files and directories
        if (relativePath[0] == '.' || relativePath.find("/.") != String::npos) {
            continue;
        }

        // Check if registered
        auto it = m_pathToId.find(relativePath);
        if (it == m_pathToId.end()) {
            // New asset
            AssetType type = detectAssetType(entry.path());
            if (type != AssetType::Unknown) {
                m_newAssets.push_back(relativePath);
            }
        } else {
            // Existing asset - check for modifications using fast metadata check
            AssetId id = it->second;
            seenAssets.insert(id);

            auto metaIt = m_assets.find(id);
            if (metaIt != m_assets.end()) {
                // Use file metadata (mod time + size) for fast change detection
                // This avoids reading the entire file just to check for changes
                u64 modTime = 0;
                u64 fileSize = 0;
                if (getFileMetadata(entry.path(), modTime, fileSize)) {
                    if (metaIt->second.metadataChanged(modTime, fileSize)) {
                        m_modifiedAssets.push_back(id);
                    }
                }
            }
        }
    }

    // Find deleted assets
    for (const auto& [id, metadata] : m_assets) {
        if (!seenAssets.contains(id)) {
            m_deletedAssets.push_back(id);
        }
    }

    usize totalChanges = m_newAssets.size() + m_deletedAssets.size() + m_modifiedAssets.size();

    if (totalChanges > 0) {
        LIMBO_LOG_ASSET_INFO("Asset scan: {} new, {} deleted, {} modified", m_newAssets.size(),
                             m_deletedAssets.size(), m_modifiedAssets.size());
    }

    return totalChanges;
}

AssetType AssetRegistry::detectAssetType(const std::filesystem::path& path) {
    String ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Textures
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" ||
        ext == ".gif") {
        return AssetType::Texture;
    }

    // Shaders
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".vs" || ext == ".fs" ||
        ext == ".shader") {
        return AssetType::Shader;
    }

    // Audio
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac" || ext == ".aiff") {
        return AssetType::Audio;
    }

    return AssetType::Unknown;
}

}  // namespace limbo
