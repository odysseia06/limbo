#include <limbo/assets/AssetRegistry.hpp>
#include <limbo/assets/AssetImporter.hpp>
#include <limbo/debug/Log.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace limbo;

void printUsage(const char* programName) {
    LIMBO_LOG_ASSET_INFO("Limbo Asset Cooker v0.1.0");
    LIMBO_LOG_ASSET_INFO("");
    LIMBO_LOG_ASSET_INFO("Usage: {} <command> [options]", programName);
    LIMBO_LOG_ASSET_INFO("");
    LIMBO_LOG_ASSET_INFO("Commands:");
    LIMBO_LOG_ASSET_INFO("  scan      Scan source directory for new/changed/deleted assets");
    LIMBO_LOG_ASSET_INFO("  import    Import all assets that need importing");
    LIMBO_LOG_ASSET_INFO("  rebuild   Force reimport of all assets");
    LIMBO_LOG_ASSET_INFO("  status    Show registry status");
    LIMBO_LOG_ASSET_INFO("  clean     Remove all imported assets");
    LIMBO_LOG_ASSET_INFO("");
    LIMBO_LOG_ASSET_INFO("Options:");
    LIMBO_LOG_ASSET_INFO(
        "  --project <path>   Project root directory (default: current directory)");
    LIMBO_LOG_ASSET_INFO("  --source <dir>     Source assets directory (default: assets)");
    LIMBO_LOG_ASSET_INFO(
        "  --output <dir>     Imported assets directory (default: build/imported)");
    LIMBO_LOG_ASSET_INFO("  --verbose          Enable verbose logging");
}

struct CookerOptions {
    std::string command;
    std::filesystem::path projectRoot;
    std::string sourceDir = "assets";
    std::string outputDir = "build/imported";
    bool verbose = false;
};

CookerOptions parseArgs(int argc, char* argv[]) {
    CookerOptions options;
    options.projectRoot = std::filesystem::current_path();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--project" && i + 1 < argc) {
            options.projectRoot = argv[++i];
        } else if (arg == "--source" && i + 1 < argc) {
            options.sourceDir = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.outputDir = argv[++i];
        } else if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        } else if (arg[0] != '-' && options.command.empty()) {
            options.command = arg;
        }
    }

    return options;
}

int cmdScan(AssetRegistry& registry) {
    LIMBO_LOG_ASSET_INFO("Scanning source directory...");

    usize changes = registry.scanSourceDirectory();

    const auto& newAssets = registry.getNewAssets();
    const auto& deletedAssets = registry.getDeletedAssets();
    const auto& modifiedAssets = registry.getModifiedAssets();

    if (changes == 0) {
        LIMBO_LOG_ASSET_INFO("No changes detected.");
        return EXIT_SUCCESS;
    }

    LIMBO_LOG_ASSET_INFO("Found {} changes:", changes);

    if (!newAssets.empty()) {
        LIMBO_LOG_ASSET_INFO("  New assets ({}):", newAssets.size());
        for (const auto& path : newAssets) {
            LIMBO_LOG_ASSET_INFO("    + {}", path);
        }
    }

    if (!modifiedAssets.empty()) {
        LIMBO_LOG_ASSET_INFO("  Modified assets ({}):", modifiedAssets.size());
        for (AssetId id : modifiedAssets) {
            const AssetMetadata* meta = registry.getMetadata(id);
            if (meta) {
                LIMBO_LOG_ASSET_INFO("    ~ {}", meta->sourcePath);
            }
        }
    }

    if (!deletedAssets.empty()) {
        LIMBO_LOG_ASSET_INFO("  Deleted assets ({}):", deletedAssets.size());
        for (AssetId id : deletedAssets) {
            const AssetMetadata* meta = registry.getMetadata(id);
            if (meta) {
                LIMBO_LOG_ASSET_INFO("    - {}", meta->sourcePath);
            }
        }
    }

    // Auto-register new assets
    if (!newAssets.empty()) {
        LIMBO_LOG_ASSET_INFO("Registering new assets...");
        for (const auto& path : newAssets) {
            std::filesystem::path fullPath = registry.getSourceDir() / path;
            AssetType type = AssetType::Unknown;

            std::string ext = fullPath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                ext == ".tga") {
                type = AssetType::Texture;
            } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
                type = AssetType::Shader;
            } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
                type = AssetType::Audio;
            } else if (fullPath.string().ends_with(".atlas.json")) {
                type = AssetType::SpriteAtlas;
            }

            if (type != AssetType::Unknown) {
                AssetId id = registry.registerAsset(path, type);
                LIMBO_LOG_ASSET_DEBUG("  Registered: {} -> {}", path, id.toString());
            }
        }
        registry.save();
    }

    // Handle deleted assets
    if (!deletedAssets.empty()) {
        LIMBO_LOG_ASSET_INFO("Unregistering deleted assets...");
        for (AssetId id : deletedAssets) {
            registry.unregisterAsset(id);
        }
        registry.save();
    }

    return EXIT_SUCCESS;
}

int cmdImport(AssetRegistry& registry, AssetImporterManager& importer) {
    // First scan for changes
    registry.scanSourceDirectory();

    // Register new assets
    for (const auto& path : registry.getNewAssets()) {
        std::filesystem::path fullPath = registry.getSourceDir() / path;
        AssetType type = AssetType::Unknown;

        std::string ext = fullPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
            type = AssetType::Texture;
        } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
            type = AssetType::Shader;
        } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
            type = AssetType::Audio;
        }

        if (type != AssetType::Unknown) {
            registry.registerAsset(path, type);
        }
    }

    // Import all assets that need it
    LIMBO_LOG_ASSET_INFO("Importing assets...");

    importer.setProgressCallback([](usize current, usize total, const String& path) {
        LIMBO_LOG_ASSET_INFO("[{}/{}] Importing: {}", current, total, path);
    });

    usize imported = importer.importAll();

    if (imported > 0) {
        LIMBO_LOG_ASSET_INFO("Successfully imported {} assets.", imported);
    } else {
        LIMBO_LOG_ASSET_INFO("No assets needed importing.");
    }

    return EXIT_SUCCESS;
}

int cmdRebuild(AssetRegistry& registry, AssetImporterManager& importer) {
    LIMBO_LOG_ASSET_INFO("Rebuilding all assets...");

    // Force reimport by clearing all source hashes
    std::vector<AssetId> allAssets = registry.getAllAssetIds();

    for (AssetId id : allAssets) {
        registry.updateSourceHash(id, 0);  // Reset hash to force reimport
    }

    importer.setProgressCallback([](usize current, usize total, const String& path) {
        LIMBO_LOG_ASSET_INFO("[{}/{}] Importing: {}", current, total, path);
    });

    usize imported = importer.importAll();
    LIMBO_LOG_ASSET_INFO("Rebuilt {} assets.", imported);

    return EXIT_SUCCESS;
}

int cmdStatus(AssetRegistry& registry) {
    std::vector<AssetId> allAssets = registry.getAllAssetIds();

    LIMBO_LOG_ASSET_INFO("Asset Registry Status");
    LIMBO_LOG_ASSET_INFO("=====================");
    LIMBO_LOG_ASSET_INFO("Project Root: {}", registry.getProjectRoot().string());
    LIMBO_LOG_ASSET_INFO("Source Dir:   {}", registry.getSourceDir().string());
    LIMBO_LOG_ASSET_INFO("Imported Dir: {}", registry.getImportedDir().string());
    LIMBO_LOG_ASSET_INFO("");
    LIMBO_LOG_ASSET_INFO("Total Assets: {}", allAssets.size());

    // Count by type
    usize textureCount = 0;
    usize shaderCount = 0;
    usize audioCount = 0;
    usize unknownCount = 0;
    usize needsImport = 0;

    for (AssetId id : allAssets) {
        const AssetMetadata* meta = registry.getMetadata(id);
        if (!meta)
            continue;

        switch (meta->type) {
        case AssetType::Texture:
            ++textureCount;
            break;
        case AssetType::SpriteAtlas:
            ++textureCount;
            break;  // Count with textures
        case AssetType::Shader:
            ++shaderCount;
            break;
        case AssetType::Audio:
            ++audioCount;
            break;
        default:
            ++unknownCount;
            break;
        }

        if (meta->importedPath.empty()) {
            ++needsImport;
        }
    }

    LIMBO_LOG_ASSET_INFO("  Textures:    {}", textureCount);
    LIMBO_LOG_ASSET_INFO("  Shaders:     {}", shaderCount);
    LIMBO_LOG_ASSET_INFO("  Audio:       {}", audioCount);
    if (unknownCount > 0) {
        LIMBO_LOG_ASSET_INFO("  Unknown:     {}", unknownCount);
    }
    LIMBO_LOG_ASSET_INFO("");
    LIMBO_LOG_ASSET_INFO("Needs Import:  {}", needsImport);

    return EXIT_SUCCESS;
}

int cmdClean(AssetRegistry& registry) {
    LIMBO_LOG_ASSET_INFO("Cleaning imported assets...");

    std::filesystem::path importedDir = registry.getImportedDir();

    if (std::filesystem::exists(importedDir)) {
        usize count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(importedDir)) {
            if (entry.path().filename() != "asset_registry.json") {
                std::filesystem::remove_all(entry.path());
                ++count;
            }
        }
        LIMBO_LOG_ASSET_INFO("Removed {} items from imported directory.", count);
    }

    // Clear imported paths in registry
    std::vector<AssetId> allAssets = registry.getAllAssetIds();
    for (AssetId id : allAssets) {
        registry.updateSourceHash(id, 0);
        registry.markAsImported(id, "");
    }
    registry.save();

    LIMBO_LOG_ASSET_INFO("Clean complete.");
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return EXIT_SUCCESS;
    }

    CookerOptions options = parseArgs(argc, argv);

    if (options.command.empty()) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (options.verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    LIMBO_LOG_ASSET_INFO("Limbo Asset Cooker v0.1.0");
    LIMBO_LOG_ASSET_DEBUG("Project: {}", options.projectRoot.string());
    LIMBO_LOG_ASSET_DEBUG("Source:  {}", options.sourceDir);
    LIMBO_LOG_ASSET_DEBUG("Output:  {}", options.outputDir);

    // Initialize registry
    AssetRegistry registry;
    registry.init(options.projectRoot, options.sourceDir, options.outputDir);
    registry.load();

    // Initialize importer
    AssetImporterManager importer;
    importer.init(registry);

    // Execute command
    int result = EXIT_FAILURE;

    if (options.command == "scan") {
        result = cmdScan(registry);
    } else if (options.command == "import") {
        result = cmdImport(registry, importer);
    } else if (options.command == "rebuild") {
        result = cmdRebuild(registry, importer);
    } else if (options.command == "status") {
        result = cmdStatus(registry);
    } else if (options.command == "clean") {
        result = cmdClean(registry);
    } else {
        LIMBO_LOG_ASSET_ERROR("Unknown command: {}", options.command);
        printUsage(argv[0]);
        result = EXIT_FAILURE;
    }

    return result;
}
