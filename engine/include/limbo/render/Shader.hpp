#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>
#include <filesystem>
#include <unordered_map>

namespace limbo {

class LIMBO_API Shader {
public:
    Shader() = default;
    ~Shader();

    // Non-copyable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Movable
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Load shader from source strings
    [[nodiscard]] Result<void, String> loadFromSource(StringView vertexSource,
                                                      StringView fragmentSource);

    // Load shader from files
    [[nodiscard]] Result<void, String> loadFromFiles(const std::filesystem::path& vertexPath,
                                                     const std::filesystem::path& fragmentPath);

    // Bind/unbind the shader program
    void bind() const;
    static void unbind();

    // Check if shader is valid
    [[nodiscard]] bool isValid() const { return m_programId != 0; }

    // Get uniform location (cached)
    [[nodiscard]] i32 getUniformLocation(StringView name) const;

    // Set uniforms
    void setInt(StringView name, i32 value) const;
    void setFloat(StringView name, f32 value) const;
    void setVec2(StringView name, const glm::vec2& value) const;
    void setVec3(StringView name, const glm::vec3& value) const;
    void setVec4(StringView name, const glm::vec4& value) const;
    void setMat3(StringView name, const glm::mat3& value) const;
    void setMat4(StringView name, const glm::mat4& value) const;

    // Get native handle
    [[nodiscard]] u32 getNativeHandle() const { return m_programId; }

private:
    [[nodiscard]] Result<u32, String> compileShader(u32 type, StringView source);
    void destroy();

    u32 m_programId = 0;
    mutable std::unordered_map<String, i32> m_uniformLocationCache;
};

}  // namespace limbo
