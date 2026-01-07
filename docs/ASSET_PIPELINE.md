# Asset Pipeline

This document describes the Limbo Engine asset pipeline, including asset identification, the registry system, import pipeline, and the assetcooker CLI tool.

## Overview

The asset pipeline handles:

1. **Asset Identification** - Stable UUIDs that survive renames and moves
2. **Asset Registry** - Central database tracking all assets and their metadata
3. **Import Pipeline** - Converts source assets to engine-optimized formats
4. **Dependency Tracking** - Tracks relationships between assets for hot-reload
5. **CLI Tooling** - Command-line tool for asset management

## Architecture

```
Source Assets (assets/)          Imported Assets (build/imported/)
├── textures/                    ├── textures/
│   ├── player.png        -->    │   └── <uuid>.texture
│   └── tileset.png              └── audio/
├── audio/                           └── <uuid>.audio
│   └── music.ogg
└── shaders/
    └── sprite.glsl

                    Asset Registry (asset_registry.json)
                    ├── UUID -> source path mapping
                    ├── Import settings per asset
                    ├── Source file hashes
                    └── Dependency graph
```

## Asset Identification

### UUID-Based IDs

Assets are identified by 128-bit UUIDs rather than file paths:

```cpp
// Generate a new asset ID
AssetId id = AssetId::generate();

// Create from UUID
UUID uuid = UUID::generate();
AssetId id{uuid};

// Parse from string
AssetId id = AssetId::fromString("550e8400-e29b-41d4-a716-446655440000");

// Convert to string
String str = id.toString();  // "550e8400-e29b-41d4-a716-446655440000"
```

### Benefits of UUIDs

| Feature | Path-based IDs | UUID-based IDs |
|---------|---------------|----------------|
| Rename assets | Breaks references | References survive |
| Move assets | Breaks references | References survive |
| Duplicate detection | Requires path comparison | Guaranteed unique |
| Serialization | Path strings | Compact 128-bit value |
| Cross-project refs | Path conflicts | Globally unique |

### Legacy Path Support

For convenience, AssetIds can still be created from paths (generates a hash-based ID):

```cpp
// Legacy: creates hash-based ID from path
AssetId id{"textures/player.png"};

// Preferred: use registry to get stable UUID
AssetId id = registry.getIdByPath("textures/player.png");
```

## Asset Registry

The `AssetRegistry` maintains the mapping between UUIDs and file paths, along with metadata for each asset.

### Initialization

```cpp
AssetRegistry registry;

// Initialize with project paths
registry.init(
    projectRoot,           // Project root directory
    "assets",              // Source assets directory (relative)
    "build/imported"       // Imported assets directory (relative)
);

// Load existing registry from disk
registry.load();
```

### Asset Registration

```cpp
// Register a new asset (creates UUID)
AssetId id = registry.registerAsset("textures/player.png", AssetType::Texture);

// Check if registered
bool exists = registry.isRegistered(id);
bool pathExists = registry.isPathRegistered("textures/player.png");

// Unregister
registry.unregisterAsset(id);
```

### Asset Lookup

```cpp
// Get ID by path
AssetId id = registry.getIdByPath("textures/player.png");

// Get metadata
AssetMetadata* meta = registry.getMetadata(id);
if (meta) {
    String sourcePath = meta->sourcePath;
    AssetType type = meta->type;
    u64 hash = meta->sourceHash;
}

// Get all assets
std::vector<AssetId> all = registry.getAllAssetIds();

// Get assets by type
std::vector<AssetId> textures = registry.getAssetsByType(AssetType::Texture);
```

### Path Resolution

```cpp
// Get absolute paths
std::filesystem::path source = registry.getSourcePath(id);
std::filesystem::path imported = registry.getImportedPath(id);

// Get directories
std::filesystem::path sourceDir = registry.getSourceDir();      // <project>/assets
std::filesystem::path importDir = registry.getImportedDir();    // <project>/build/imported
```

### Asset Metadata

Each registered asset has associated metadata:

```cpp
struct AssetMetadata {
    AssetId id;                        // Stable UUID
    String sourcePath;                 // Relative path to source file
    String importedPath;               // Relative path to imported file
    AssetType type;                    // Texture, Shader, Audio, etc.
    u64 sourceHash;                    // Hash for change detection
    u64 importedTimestamp;             // When last imported
    std::vector<AssetId> dependencies; // Assets this depends on
    std::vector<AssetId> dependents;   // Assets depending on this
    String importSettingsJson;         // Type-specific settings
};
```

### Import Settings

Type-specific import settings are stored per asset:

```cpp
// Texture settings
struct TextureImportSettings {
    bool generateMipmaps = true;
    bool sRGB = true;
    bool premultiplyAlpha = false;
    i32 maxSize = 4096;
    String filterMode = "linear";   // "nearest", "linear"
    String wrapMode = "repeat";     // "repeat", "clamp", "mirror"
};

// Audio settings
struct AudioImportSettings {
    bool streaming = false;
    f32 quality = 1.0f;  // 0.0 to 1.0
};

// Sprite sheet settings
struct SpriteSheetImportSettings {
    i32 spriteWidth = 0;   // 0 = auto-detect
    i32 spriteHeight = 0;
    i32 padding = 0;
    i32 spacing = 0;
};
```

### Dependency Tracking

Track relationships between assets for hot-reload:

```cpp
// Add dependency (material depends on texture)
registry.addDependency(materialId, textureId);

// Query dependencies
std::vector<AssetId> deps = registry.getDependencies(materialId);

// Query dependents (what depends on this asset)
std::vector<AssetId> dependents = registry.getDependents(textureId);

// Clear dependencies
registry.clearDependencies(materialId);
```

### Source Directory Scanning

Detect new, modified, and deleted assets:

```cpp
// Scan for changes
usize changeCount = registry.scanSourceDirectory();

// Get scan results
const std::vector<String>& newAssets = registry.getNewAssets();
const std::vector<AssetId>& deletedAssets = registry.getDeletedAssets();
const std::vector<AssetId>& modifiedAssets = registry.getModifiedAssets();

// Process changes
for (const String& path : newAssets) {
    registry.registerAsset(path, detectType(path));
}

for (AssetId id : deletedAssets) {
    registry.unregisterAsset(id);
}
```

### Persistence

The registry is saved to `asset_registry.json`:

```cpp
// Save registry to disk
registry.save();

// Load registry from disk
registry.load();

// Get registry file path
std::filesystem::path path = registry.getRegistryPath();
```

## Import Pipeline

The import pipeline converts source assets to engine-optimized formats.

### AssetImporterManager

```cpp
AssetImporterManager importer;
importer.init(registry);

// Import a single asset
ImportResult result = importer.importAsset(assetId);
if (!result.success) {
    spdlog::error("Import failed: {}", result.error);
}

// Import all assets needing update
usize count = importer.importAll();

// Import with predicate
usize count = importer.importWhere([](const AssetMetadata& meta) {
    return meta.type == AssetType::Texture;
});
```

### Progress Tracking

```cpp
importer.setProgressCallback([](usize current, usize total, const String& path) {
    spdlog::info("[{}/{}] Importing: {}", current, total, path);
});
```

### Built-in Importers

| Importer | Asset Type | Extensions |
|----------|-----------|------------|
| TextureImporter | Texture | .png, .jpg, .jpeg, .bmp, .tga, .gif |
| ShaderImporter | Shader | .glsl, .vert, .frag, .vs, .fs, .shader |
| AudioImporter | Audio | .wav, .mp3, .ogg, .flac |

### Custom Importers

Implement `IAssetImporter` for custom asset types:

```cpp
class FontImporter : public IAssetImporter {
public:
    AssetType getAssetType() const override { 
        return AssetType::Font; 
    }

    std::vector<String> getSupportedExtensions() const override {
        return {".ttf", ".otf"};
    }

    ImportResult import(const ImportContext& context) override {
        // Read source file
        std::filesystem::path source = context.sourcePath;
        
        // Process font...
        
        // Write imported file
        std::filesystem::path output = context.importedDir / 
            (context.assetId.toString() + ".font");
        
        // Return result
        return ImportResult::ok(output.filename().string());
    }

    String getDefaultSettings() const override {
        return R"({"fontSize": 16, "charset": "ascii"})";
    }
};

// Register custom importer
importer.registerImporter(std::make_unique<FontImporter>());
```

### Import Context

Importers receive context about the asset being imported:

```cpp
struct ImportContext {
    AssetRegistry* registry;           // Registry for dependency tracking
    AssetId assetId;                   // ID of asset being imported
    std::filesystem::path sourcePath;  // Absolute source path
    std::filesystem::path importedDir; // Absolute import directory
    const AssetMetadata* metadata;     // Asset metadata
};
```

### Import Results

```cpp
struct ImportResult {
    bool success;
    String error;        // Error message if failed
    String importedPath; // Relative path to imported file

    static ImportResult ok(const String& path);
    static ImportResult fail(const String& error);
};
```

## Asset Cooker CLI

The `assetcooker` tool provides command-line asset management.

### Usage

```bash
assetcooker <command> [options]
```

### Commands

| Command | Description |
|---------|-------------|
| `scan` | Scan source directory for new/changed/deleted assets |
| `import` | Import all assets that need importing |
| `rebuild` | Force reimport of all assets |
| `status` | Show registry status |
| `clean` | Remove all imported assets |

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `--project <path>` | Project root directory | Current directory |
| `--source <dir>` | Source assets directory | `assets` |
| `--output <dir>` | Imported assets directory | `build/imported` |
| `--verbose`, `-v` | Enable verbose logging | Off |

### Examples

```bash
# Scan for new assets
assetcooker scan

# Import modified assets
assetcooker import

# Force rebuild all assets
assetcooker rebuild

# Show registry status
assetcooker status

# Clean imported directory
assetcooker clean

# Use custom project path
assetcooker import --project /path/to/game --verbose
```

### Sample Output

```
$ assetcooker status
Asset Registry Status
=====================
Project Root: C:/dev/mygame
Source Dir:   C:/dev/mygame/assets
Imported Dir: C:/dev/mygame/build/imported

Total Assets: 42
  Textures:    28
  Shaders:     8
  Audio:       6

Needs Import:  3
```

## Workflow

### Initial Setup

1. Place source assets in `assets/` directory
2. Run `assetcooker scan` to register new assets
3. Run `assetcooker import` to import all assets

### Development Cycle

1. Modify source assets
2. Run `assetcooker import` (only imports changed assets)
3. Hot-reload picks up changes in editor

### Clean Rebuild

```bash
assetcooker clean
assetcooker rebuild
```

## Best Practices

### Asset Organization

```
assets/
├── textures/
│   ├── characters/
│   │   ├── player.png
│   │   └── enemy.png
│   ├── environment/
│   │   └── tileset.png
│   └── ui/
│       └── buttons.png
├── audio/
│   ├── music/
│   │   └── theme.ogg
│   └── sfx/
│       └── jump.wav
└── shaders/
    ├── sprite.glsl
    └── postprocess.glsl
```

### Asset References

Always use AssetIds for references in game data:

```json
{
  "player": {
    "texture": "550e8400-e29b-41d4-a716-446655440000",
    "shader": "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
  }
}
```

### Import Settings

Store import settings in `.meta` files alongside assets:

```
textures/player.png
textures/player.png.meta  <-- Import settings JSON
```

### Version Control

- Commit source assets (`assets/`)
- Commit registry (`asset_registry.json`)
- Ignore imported assets (`build/imported/`)

Example `.gitignore`:
```
build/imported/
```

## File Formats

### asset_registry.json

```json
{
  "version": 1,
  "assets": {
    "550e8400-e29b-41d4-a716-446655440000": {
      "sourcePath": "textures/player.png",
      "importedPath": "550e8400-e29b-41d4-a716-446655440000.texture",
      "type": "Texture",
      "sourceHash": 12345678901234567890,
      "importedTimestamp": 1704672000,
      "dependencies": [],
      "importSettings": "{\"generateMipmaps\":true,\"sRGB\":true}"
    }
  }
}
```

### Imported Asset Format

Imported assets use a simple binary header:

```
[4 bytes] Magic number
[4 bytes] Version
[4 bytes] Asset type
[4 bytes] Data size
[N bytes] Asset data
```

## Future Enhancements

- **Sprite Sheet Building** - Automatic atlas generation
- **Hot Reload** - Real-time asset reloading in editor
- **Async Import** - Background import with progress UI
- **Compression** - LZ4/Zstd compression for imported assets
- **Streaming** - Large asset streaming support
- **Variants** - Platform-specific asset variants
