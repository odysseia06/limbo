#include <limbo/assets/AssetRegistry.hpp>
#include <limbo/assets/AssetImporter.hpp>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace limbo;

void printUsage(const char* programName) {
    spdlog::info("Limbo Asset Cooker v0.1.0");
    spdlog::info("");
    spdlog::info("Usage: {} <command> [options]", programName);
    spdlog::info("");
    spdlog::info("Commands:");
    spdlog::info("  scan      Scan source directory for new/changed/deleted assets");
    spdlog::info("  import    Import all assets that need importing");
    spdlog::info("  rebuild   Force reimport of all assets");
    spdlog::info("  status    Show registry status");
    spdlog::info("  clean     Remove all imported assets");
    spdlog::info("");
    spdlog::info("Options:");
    spdlog::info("  --project <path>   Project root directory (default: current directory)");
    spdlog::info("  --source <dir>     Source assets directory (default: assets)");
    spdlog::info("  --output <dir>     Imported assets directory (default: build/imported)");
    spdlog::info("  --verbose          Enable verbose logging");
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
    spdlog::info("Scanning source directory...");

    usize changes = registry.scanSourceDirectory();

    const auto& newAssets = registry.getNewAssets();
    const auto& deletedAssets = registry.getDeletedAssets();
    const auto& modifiedAssets = registry.getModifiedAssets();

    if (changes == 0) {
        spdlog::info("No changes detected.");
        return EXIT_SUCCESS;
    }

    spdlog::info("Found {} changes:", changes);

    if (!newAssets.empty()) {
        spdlog::info("  New assets ({}):", newAssets.size());
        for (const auto& path : newAssets) {
            spdlog::info("    + {}", path);
        }
    }

    if (!modifiedAssets.empty()) {
        spdlog::info("  Modified assets ({}):", modifiedAssets.size());
        for (AssetId id : modifiedAssets) {
            const AssetMetadata* meta = registry.getMetadata(id);
            if (meta) {
                spdlog::info("    ~ {}", meta->sourcePath);
            }
        }
    }

    if (!deletedAssets.empty()) {
        spdlog::info("  Deleted assets ({}):", deletedAssets.size());
        for (AssetId id : deletedAssets) {
            const AssetMetadata* meta = registry.getMetadata(id);
            if (meta) {
                spdlog::info("    - {}", meta->sourcePath);
            }
        }
    }

    // Auto-register new assets
    if (!newAssets.empty()) {
        spdlog::info("Registering new assets...");
        for (const auto& path : newAssets) {
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
            } else if (fullPath.string().ends_with(".atlas.json")) {
                type = AssetType::SpriteAtlas;
            }

            if (type != AssetType::Unknown) {
                AssetId id = registry.registerAsset(path, type);
                spdlog::debug("  Registered: {} -> {}", path, id.toString());
            }
        }
        registry.save();
    }

    // Handle deleted assets
    if (!deletedAssets.empty()) {
        spdlog::info("Unregistering deleted assets...");
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
    spdlog::info("Importing assets...");

    importer.setProgressCallback([](usize current, usize total, const String& path) {
        spdlog::info("[{}/{}] Importing: {}", current, total, path);
    });

    usize imported = importer.importAll();

    if (imported > 0) {
        spdlog::info("Successfully imported {} assets.", imported);
    } else {
        spdlog::info("No assets needed importing.");
    }

    return EXIT_SUCCESS;
}

int cmdRebuild(AssetRegistry& registry, AssetImporterManager& importer) {
    spdlog::info("Rebuilding all assets...");

    // Force reimport by clearing all source hashes
    std::vector<AssetId> allAssets = registry.getAllAssetIds();

    for (AssetId id : allAssets) {
        registry.updateSourceHash(id, 0);  // Reset hash to force reimport
    }

    importer.setProgressCallback([](usize current, usize total, const String& path) {
        spdlog::info("[{}/{}] Importing: {}", current, total, path);
    });

    usize imported = importer.importAll();
    spdlog::info("Rebuilt {} assets.", imported);

    return EXIT_SUCCESS;
}

int cmdStatus(AssetRegistry& registry) {
    std::vector<AssetId> allAssets = registry.getAllAssetIds();

    spdlog::info("Asset Registry Status");
    spdlog::info("=====================");
    spdlog::info("Project Root: {}", registry.getProjectRoot().string());
    spdlog::info("Source Dir:   {}", registry.getSourceDir().string());
    spdlog::info("Imported Dir: {}", registry.getImportedDir().string());
    spdlog::info("");
    spdlog::info("Total Assets: {}", allAssets.size());

    // Count by type
    usize textureCount = 0;
    usize shaderCount = 0;
    usize audioCount = 0;
    usize unknownCount = 0;
    usize needsImport = 0;

    for (AssetId id : allAssets) {
        const AssetMetadata* meta = registry.getMetadata(id);
        if (!meta) continue;

        switch (meta->type) {
            case AssetType::Texture: ++textureCount; break;
            case AssetType::SpriteAtlas: ++textureCount; break;  // Count with textures
            case AssetType::Shader: ++shaderCount; break;
            case AssetType::Audio: ++audioCount; break;
            default: ++unknownCount; break;
        }

        if (meta->importedPath.empty()) {
            ++needsImport;
        }
    }

    spdlog::info("  Textures:    {}", textureCount);
    spdlog::info("  Shaders:     {}", shaderCount);
    spdlog::info("  Audio:       {}", audioCount);
    if (unknownCount > 0) {
        spdlog::info("  Unknown:     {}", unknownCount);
    }
    spdlog::info("");
    spdlog::info("Needs Import:  {}", needsImport);

    return EXIT_SUCCESS;
}

int cmdClean(AssetRegistry& registry) {
    spdlog::info("Cleaning imported assets...");

    std::filesystem::path importedDir = registry.getImportedDir();

    if (std::filesystem::exists(importedDir)) {
        usize count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(importedDir)) {
            if (entry.path().filename() != "asset_registry.json") {
                std::filesystem::remove_all(entry.path());
                ++count;
            }
        }
        spdlog::info("Removed {} items from imported directory.", count);
    }

    // Clear imported paths in registry
    std::vector<AssetId> allAssets = registry.getAllAssetIds();
    for (AssetId id : allAssets) {
        registry.updateSourceHash(id, 0);
        registry.markAsImported(id, "");
    }
    registry.save();

    spdlog::info("Clean complete.");
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

    spdlog::info("Limbo Asset Cooker v0.1.0");
    spdlog::debug("Project: {}", options.projectRoot.string());
    spdlog::debug("Source:  {}", options.sourceDir);
    spdlog::debug("Output:  {}", options.outputDir);

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
        spdlog::error("Unknown command: {}", options.command);
        printUsage(argv[0]);
        result = EXIT_FAILURE;
    }

    return result;
}
