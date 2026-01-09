#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/render/common/Shader.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>

#include <unordered_map>
#include <variant>

namespace limbo {

/**
 * Material property value types for 2D sprites
 */
using SpritePropertyValue = std::variant<f32, glm::vec2, glm::vec3, glm::vec4, i32>;

/**
 * SpriteMaterial - Lightweight material system for 2D sprites
 *
 * Allows custom shaders and uniforms for sprite rendering effects
 * like outlines, dissolve, glow, distortion, etc.
 *
 * Usage:
 *   auto material = SpriteMaterial::create();
 *   material->setColor({1, 0, 0, 1});  // Red tint
 *   material->setFloat("u_OutlineWidth", 2.0f);
 *   material->bind();
 *   // Render sprites...
 */
class LIMBO_API SpriteMaterial {
public:
    SpriteMaterial() = default;
    ~SpriteMaterial() = default;

    // Non-copyable, movable
    SpriteMaterial(const SpriteMaterial&) = delete;
    SpriteMaterial& operator=(const SpriteMaterial&) = delete;
    SpriteMaterial(SpriteMaterial&&) noexcept = default;
    SpriteMaterial& operator=(SpriteMaterial&&) noexcept = default;

    /**
     * Create a default sprite material (uses standard sprite shader)
     */
    static Shared<SpriteMaterial> create();

    /**
     * Create a sprite material with a custom shader
     */
    static Shared<SpriteMaterial> create(Shared<Shader> shader);

    // ========================================================================
    // Shader
    // ========================================================================

    /**
     * Set a custom shader for this material
     * If null, the default Renderer2D shader is used
     */
    void setShader(Shared<Shader> shader) { m_shader = std::move(shader); }

    /**
     * Get the custom shader (may be null for default)
     */
    [[nodiscard]] Shader* getShader() const { return m_shader.get(); }

    /**
     * Check if using a custom shader
     */
    [[nodiscard]] bool hasCustomShader() const { return m_shader != nullptr; }

    // ========================================================================
    // Common Properties
    // ========================================================================

    /**
     * Set the tint color (multiplied with texture color)
     */
    void setColor(const glm::vec4& color) { m_color = color; }

    /**
     * Get the tint color
     */
    [[nodiscard]] const glm::vec4& getColor() const { return m_color; }

    /**
     * Set the main texture
     */
    void setTexture(Texture2D* texture) { m_texture = texture; }

    /**
     * Get the main texture
     */
    [[nodiscard]] Texture2D* getTexture() const { return m_texture; }

    /**
     * Set texture tiling factor
     */
    void setTilingFactor(f32 factor) { m_tilingFactor = factor; }

    /**
     * Get texture tiling factor
     */
    [[nodiscard]] f32 getTilingFactor() const { return m_tilingFactor; }

    // ========================================================================
    // Custom Uniforms
    // ========================================================================

    /**
     * Set a float uniform
     */
    void setFloat(const String& name, f32 value);

    /**
     * Set an integer uniform
     */
    void setInt(const String& name, i32 value);

    /**
     * Set a vec2 uniform
     */
    void setVector2(const String& name, const glm::vec2& value);

    /**
     * Set a vec3 uniform
     */
    void setVector3(const String& name, const glm::vec3& value);

    /**
     * Set a vec4 uniform
     */
    void setVector4(const String& name, const glm::vec4& value);

    /**
     * Get a property value (returns nullopt if not found)
     */
    [[nodiscard]] Optional<SpritePropertyValue> getProperty(const String& name) const;

    // ========================================================================
    // Binding
    // ========================================================================

    /**
     * Bind the material for rendering
     * Activates shader and sets all uniforms
     */
    void bind() const;

    /**
     * Unbind the material
     */
    void unbind() const;

private:
    Shared<Shader> m_shader;  // Custom shader (null = use default)
    glm::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
    Texture2D* m_texture = nullptr;
    f32 m_tilingFactor = 1.0f;

    std::unordered_map<String, SpritePropertyValue> m_properties;

    void applyProperties() const;
};

}  // namespace limbo
