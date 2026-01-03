#include <spdlog/spdlog.h>
#include <fmt/core.h>

#include <cstdlib>

int main(int argc, char* argv[]) {
    spdlog::info("Limbo Asset Cooker v0.1.0");

    if (argc < 2) {
        spdlog::info("Usage: assetcooker <input_dir> [output_dir]");
        spdlog::info("  input_dir   - Directory containing raw assets");
        spdlog::info("  output_dir  - Directory for cooked assets (default: ./cooked)");
        return EXIT_SUCCESS;
    }

    const char* inputDir = argv[1];
    const char* outputDir = argc > 2 ? argv[2] : "cooked";

    spdlog::info(fmt::runtime("Input:  {}"), inputDir);
    spdlog::info(fmt::runtime("Output: {}"), outputDir);

    // TODO: Implement asset cooking pipeline
    // - Texture cooking (PNG -> engine format)
    // - Shader preprocessing
    // - Model conversion (glTF -> engine format)
    // - Registry generation

    spdlog::info("Asset cooking not yet implemented");

    return EXIT_SUCCESS;
}
