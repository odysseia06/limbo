#include "limbo/render/Shader.hpp"
#include "limbo/util/FileIO.hpp"
#include "limbo/core/Assert.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace limbo {

Shader::~Shader() {
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId)
    , m_uniformLocationCache(std::move(other.m_uniformLocationCache))
{
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        destroy();
        m_programId = other.m_programId;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        other.m_programId = 0;
    }
    return *this;
}

Result<void, String> Shader::loadFromSource(StringView vertexSource, StringView fragmentSource) {
    // Compile vertex shader
    auto vertexResult = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexResult) {
        return unexpected<String>("Vertex shader: " + vertexResult.error());
    }
    u32 vertexShader = vertexResult.value();

    // Compile fragment shader
    auto fragmentResult = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentResult) {
        glDeleteShader(vertexShader);
        return unexpected<String>("Fragment shader: " + fragmentResult.error());
    }
    u32 fragmentShader = fragmentResult.value();

    // Create and link program
    u32 program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check link status
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        String infoLog(static_cast<usize>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return unexpected<String>("Program link failed: " + infoLog);
    }

    // Clean up shaders (they're linked into the program now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Destroy old program if exists
    destroy();

    m_programId = program;
    m_uniformLocationCache.clear();

    spdlog::debug("Shader program created (ID: {})", m_programId);
    return {};
}

Result<void, String> Shader::loadFromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
) {
    auto vertexSource = util::readFileText(vertexPath);
    if (!vertexSource) {
        return unexpected<String>("Failed to load vertex shader: " + vertexSource.error());
    }

    auto fragmentSource = util::readFileText(fragmentPath);
    if (!fragmentSource) {
        return unexpected<String>("Failed to load fragment shader: " + fragmentSource.error());
    }

    return loadFromSource(vertexSource.value(), fragmentSource.value());
}

void Shader::bind() const {
    LIMBO_ASSERT(m_programId != 0, "Attempting to bind invalid shader");
    glUseProgram(m_programId);
}

void Shader::unbind() {
    glUseProgram(0);
}

i32 Shader::getUniformLocation(StringView name) const {
    String nameStr(name);

    auto it = m_uniformLocationCache.find(nameStr);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    i32 location = glGetUniformLocation(m_programId, nameStr.c_str());
    if (location == -1) {
        spdlog::warn("Uniform '{}' not found in shader", name);
    }

    m_uniformLocationCache[nameStr] = location;
    return location;
}

void Shader::setInt(StringView name, i32 value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(StringView name, f32 value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(StringView name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(StringView name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(StringView name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(StringView name, const glm::mat3& value) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(StringView name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

Result<u32, String> Shader::compileShader(u32 type, StringView source) {
    u32 shader = glCreateShader(type);

    const char* src = source.data();
    auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        String infoLog(static_cast<usize>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());

        glDeleteShader(shader);

        const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        return unexpected<String>(String(typeName) + " compilation failed: " + infoLog);
    }

    return shader;
}

void Shader::destroy() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        spdlog::debug("Shader program destroyed (ID: {})", m_programId);
        m_programId = 0;
    }
    m_uniformLocationCache.clear();
}

} // namespace limbo
