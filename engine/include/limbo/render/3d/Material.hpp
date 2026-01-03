#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/render/common/Shader.hpp"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <variant>

namespace limbo {

/**
 * @brief Material property value types
 */
using MaterialPropertyValue =
    std::variant<float, glm::vec2, glm::vec3, glm::vec4, i32, glm::mat3, glm::mat4>;

/**
 * @brief Texture slot assignment for materials
 */
struct TextureSlot {
    AssetId textureId;
    u32 slot = 0;
    String samplerName;
};

/**
 * @brief Render state for materials
 */
struct RenderState {
    enum class CullMode { None, Front, Back };
    enum class BlendMode { Opaque, Transparent, Additive };
    enum class DepthTest { Less, LessEqual, Equal, Greater, Always, Never };

    CullMode cullMode = CullMode::Back;
    BlendMode blendMode = BlendMode::Opaque;
    DepthTest depthTest = DepthTest::Less;
    bool depthWrite = true;
};

/**
 * @brief Material defining visual properties for 3D rendering
 *
 * A Material combines a shader program with uniform values and textures
 * to define how a mesh surface should be rendered.
 */
class Material {
public:
    Material() = default;
    explicit Material(Shared<Shader> shader);
    ~Material() = default;

    // Non-copyable, movable
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    /// Set the shader program
    void setShader(Shared<Shader> shader);

    /// Get the shader program
    Shader* getShader() const { return m_shader.get(); }

    // Property setters
    void setFloat(const String& name, float value);
    void setInt(const String& name, i32 value);
    void setVector2(const String& name, const glm::vec2& value);
    void setVector3(const String& name, const glm::vec3& value);
    void setVector4(const String& name, const glm::vec4& value);
    void setMatrix3(const String& name, const glm::mat3& value);
    void setMatrix4(const String& name, const glm::mat4& value);

    // Texture setters
    void setTexture(const String& samplerName, AssetId textureId, u32 slot = 0);

    // Render state
    void setRenderState(const RenderState& state) { m_renderState = state; }
    const RenderState& getRenderState() const { return m_renderState; }

    /// Bind material for rendering (shader + uniforms + textures)
    void bind() const;

    /// Unbind material
    void unbind() const;

    // Common material presets
    static Shared<Material> createUnlit();  // No lighting, just color/texture
    static Shared<Material> createPhong();  // Classic Phong shading
    static Shared<Material> createPBR();    // Physically-based rendering

private:
    Shared<Shader> m_shader;
    std::unordered_map<String, MaterialPropertyValue> m_properties;
    std::vector<TextureSlot> m_textures;
    RenderState m_renderState;

    void applyProperties() const;
    void bindTextures() const;
};

/**
 * @brief PBR material properties helper
 */
struct PBRMaterialProperties {
    glm::vec4 albedo{1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    AssetId albedoMap;
    AssetId normalMap;
    AssetId metallicMap;
    AssetId roughnessMap;
    AssetId aoMap;

    void applyTo(Material& material) const;
};

}  // namespace limbo
