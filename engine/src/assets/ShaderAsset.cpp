#include "limbo/assets/ShaderAsset.hpp"

#include "limbo/debug/Log.hpp"

#include <filesystem>

namespace limbo {

bool ShaderAsset::load() {
    m_shader = make_unique<Shader>();
    m_shaderFiles.clear();

    const auto& basePath = getPath();

    // Try different shader file extension combinations
    struct ShaderExtensions {
        const char* vertex;
        const char* fragment;
    };

    static const ShaderExtensions s_extensionPairs[] = {
        {".vert", ".frag"},
        {".vs", ".fs"},
        {".vertex.glsl", ".fragment.glsl"},
        {".vert.glsl", ".frag.glsl"},
    };

    for (const auto& ext : s_extensionPairs) {
        std::filesystem::path vertPath = basePath;
        std::filesystem::path fragPath = basePath;

        vertPath += ext.vertex;
        fragPath += ext.fragment;

        if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
            auto result = m_shader->loadFromFiles(vertPath, fragPath);
            if (result) {
                // Track dependencies for hot-reload
                m_shaderFiles.push_back(vertPath);
                m_shaderFiles.push_back(fragPath);

                LIMBO_LOG_ASSET_DEBUG("Loaded shader: {} ({}, {})", basePath.string(), ext.vertex,
                                      ext.fragment);
                return true;
            } else {
                setError(result.error());
                LIMBO_LOG_ASSET_ERROR("Failed to compile shader '{}': {}", basePath.string(),
                                      result.error());
                m_shader.reset();
                return false;
            }
        }
    }

    // No shader files found
    String error = "Could not find shader files for: " + basePath.string();
    setError(error);
    LIMBO_LOG_ASSET_ERROR("{}", error);
    m_shader.reset();
    return false;
}

void ShaderAsset::unload() {
    m_shader.reset();
}

}  // namespace limbo
