#include "limbo/render/2d/SpriteMaterial.hpp"

namespace limbo {

Shared<SpriteMaterial> SpriteMaterial::create() {
    return std::make_shared<SpriteMaterial>();
}

Shared<SpriteMaterial> SpriteMaterial::create(Shared<Shader> shader) {
    auto material = std::make_shared<SpriteMaterial>();
    material->m_shader = std::move(shader);
    return material;
}

void SpriteMaterial::setFloat(const String& name, f32 value) {
    m_properties[name] = value;
}

void SpriteMaterial::setInt(const String& name, i32 value) {
    m_properties[name] = value;
}

void SpriteMaterial::setVector2(const String& name, const glm::vec2& value) {
    m_properties[name] = value;
}

void SpriteMaterial::setVector3(const String& name, const glm::vec3& value) {
    m_properties[name] = value;
}

void SpriteMaterial::setVector4(const String& name, const glm::vec4& value) {
    m_properties[name] = value;
}

Optional<SpritePropertyValue> SpriteMaterial::getProperty(const String& name) const {
    auto it = m_properties.find(name);
    if (it != m_properties.end()) {
        return it->second;
    }
    return std::nullopt;
}

void SpriteMaterial::bind() const {
    if (m_shader) {
        m_shader->bind();
        applyProperties();

        // Set common uniforms
        m_shader->setVec4("u_Color", m_color);
        m_shader->setFloat("u_TilingFactor", m_tilingFactor);

        // Bind texture if present
        if (m_texture) {
            m_texture->bind(0);
            m_shader->setInt("u_Texture", 0);
        }
    }
}

void SpriteMaterial::unbind() const {
    if (m_shader) {
        Shader::unbind();
    }
}

void SpriteMaterial::applyProperties() const {
    if (!m_shader) {
        return;
    }

    for (const auto& [name, value] : m_properties) {
        std::visit(
            [this, &name](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, f32>) {
                    m_shader->setFloat(name, arg);
                } else if constexpr (std::is_same_v<T, i32>) {
                    m_shader->setInt(name, arg);
                } else if constexpr (std::is_same_v<T, glm::vec2>) {
                    m_shader->setVec2(name, arg);
                } else if constexpr (std::is_same_v<T, glm::vec3>) {
                    m_shader->setVec3(name, arg);
                } else if constexpr (std::is_same_v<T, glm::vec4>) {
                    m_shader->setVec4(name, arg);
                }
            },
            value);
    }
}

}  // namespace limbo
