#include "limbo/render/common/RenderContext.hpp"
#include "limbo/render/common/Buffer.hpp"
#include "limbo/platform/Platform.hpp"
#include "limbo/core/Base.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <fmt/core.h>

namespace limbo {

class OpenGLContext : public RenderContext {
public:
    OpenGLContext() = default;
    ~OpenGLContext() override { shutdown(); }

    bool init(Window& window) override {
        m_window = &window;

        // Load OpenGL functions via GLAD2
        int version = gladLoadGL(glfwGetProcAddress);
        if (version == 0) {
            spdlog::critical("Failed to initialize GLAD");
            return false;
        }

        spdlog::info("OpenGL Context initialized");
        spdlog::info("  Vendor:   {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        spdlog::info("  Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        spdlog::info("  Version:  {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

#ifdef LIMBO_DEBUG
        // Enable debug output if available
        GLint flags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(debugCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            spdlog::info("OpenGL debug output enabled");
        }
#endif

        // Set initial viewport
        setViewport(0, 0, window.getWidth(), window.getHeight());

        return true;
    }

    void shutdown() override { m_window = nullptr; }

    void beginFrame() override {
        // Nothing specific needed for OpenGL
    }

    void endFrame() override {
        // Nothing specific needed for OpenGL
    }

    void clear(f32 r, f32 g, f32 b, f32 a) override {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void setViewport(i32 x, i32 y, i32 width, i32 height) override {
        glViewport(x, y, width, height);
    }

    void drawIndexed(const VertexArray& vao, u32 indexCount) override {
        vao.bind();
        u32 count = indexCount > 0 ? indexCount : vao.getIndexBuffer().getCount();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
    }

    void drawArrays(const VertexArray& vao, u32 vertexCount) override {
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    }

private:
#ifdef LIMBO_DEBUG
    static void LIMBO_GL_CALLBACK debugCallback(GLenum source, GLenum type, GLuint id,
                                                GLenum severity, GLsizei /*length*/,
                                                const GLchar* message, const void* /*userParam*/
    ) {
        // Ignore non-significant messages
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
            return;
        }

        const char* sourceStr = "Unknown";
        switch (source) {
        case GL_DEBUG_SOURCE_API:
            sourceStr = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            sourceStr = "Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            sourceStr = "Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            sourceStr = "Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            sourceStr = "Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            sourceStr = "Other";
            break;
        default:
            break;
        }

        const char* typeStr = "Unknown";
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeStr = "Deprecated";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeStr = "Undefined Behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeStr = "Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeStr = "Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            typeStr = "Marker";
            break;
        case GL_DEBUG_TYPE_OTHER:
            typeStr = "Other";
            break;
        default:
            break;
        }

        switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            spdlog::error(fmt::runtime("GL {} [{}] ({}): {}"), typeStr, sourceStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            spdlog::warn(fmt::runtime("GL {} [{}] ({}): {}"), typeStr, sourceStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            spdlog::info(fmt::runtime("GL {} [{}] ({}): {}"), typeStr, sourceStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            spdlog::debug(fmt::runtime("GL {} [{}] ({}): {}"), typeStr, sourceStr, id, message);
            break;
        default:
            spdlog::debug(fmt::runtime("GL {} [{}] ({}): {}"), typeStr, sourceStr, id, message);
            break;
        }
    }
#endif

    Window* m_window = nullptr;
};

Unique<RenderContext> createOpenGLContext() {
    return make_unique<OpenGLContext>();
}

}  // namespace limbo
