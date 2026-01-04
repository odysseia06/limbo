#pragma once

#include "limbo/assets/Asset.hpp"
#include "limbo/render/common/Shader.hpp"

namespace limbo {

/**
 * ShaderAsset - Managed shader asset
 *
 * Loads vertex and fragment shaders from disk. The path should be
 * the base name without extension - the loader will look for
 * .vert and .frag files (or .vs/.fs, .glsl variants).
 *
 * Example: load("shaders/basic") will look for:
 *   - shaders/basic.vert + shaders/basic.frag
 *   - shaders/basic.vs + shaders/basic.fs
 */
class LIMBO_API ShaderAsset : public Asset {
public:
    ShaderAsset() = default;
    ~ShaderAsset() override = default;

    [[nodiscard]] AssetType getType() const override { return AssetType::Shader; }

    /**
     * Get the underlying shader
     */
    [[nodiscard]] Shader* getShader() { return m_shader.get(); }
    [[nodiscard]] const Shader* getShader() const { return m_shader.get(); }

    /**
     * Bind the shader for use
     */
    void bind() const {
        if (m_shader) {
            m_shader->bind();
        }
    }

    /**
     * Set shader uniforms (convenience methods)
     */
    void setInt(StringView name, i32 value) const {
        if (m_shader) {
            m_shader->setInt(name, value);
        }
    }

    void setFloat(StringView name, f32 value) const {
        if (m_shader) {
            m_shader->setFloat(name, value);
        }
    }

    void setVec2(StringView name, const glm::vec2& value) const {
        if (m_shader) {
            m_shader->setVec2(name, value);
        }
    }

    void setVec3(StringView name, const glm::vec3& value) const {
        if (m_shader) {
            m_shader->setVec3(name, value);
        }
    }

    void setVec4(StringView name, const glm::vec4& value) const {
        if (m_shader) {
            m_shader->setVec4(name, value);
        }
    }

    void setMat3(StringView name, const glm::mat3& value) const {
        if (m_shader) {
            m_shader->setMat3(name, value);
        }
    }

    void setMat4(StringView name, const glm::mat4& value) const {
        if (m_shader) {
            m_shader->setMat4(name, value);
        }
    }

    /**
     * Get shader file dependencies (vertex + fragment shader files)
     */
    [[nodiscard]] std::vector<std::filesystem::path> getDependencies() const override {
        return m_shaderFiles;
    }

protected:
    friend class AssetManager;

    bool load() override;
    void unload() override;

private:
    Unique<Shader> m_shader;
    std::vector<std::filesystem::path> m_shaderFiles;
};

}  // namespace limbo
