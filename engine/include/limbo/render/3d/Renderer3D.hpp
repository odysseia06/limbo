#pragma once

/**
 * @file Renderer3D.hpp
 * @brief EXPERIMENTAL: 3D rendering API (not yet implemented)
 *
 * This header defines the planned API for 3D rendering. The implementation
 * does not exist yet - these are placeholder declarations for future work.
 * Do not use in production code.
 */

#include "limbo/core/Types.hpp"
#include "limbo/render/common/Camera.hpp"

#include <glm/glm.hpp>

namespace limbo {

// Forward declarations
class Mesh;
class Material;
class Model;

/**
 * @brief Static 3D renderer for mesh-based rendering
 * @warning EXPERIMENTAL: Not yet implemented. API subject to change.
 *
 * Renderer3D provides a forward rendering pipeline for 3D meshes with
 * material support and lighting. It parallels Renderer2D for 2D sprites.
 *
 * Usage:
 *   Renderer3D::init();
 *   Renderer3D::beginScene(camera);
 *   Renderer3D::submit(mesh, material, transform);
 *   Renderer3D::endScene();
 */
class Renderer3D {
public:
    Renderer3D() = delete;

    /// Initialize the 3D renderer (call once at startup)
    static void init();

    /// Shutdown the 3D renderer (call once at shutdown)
    static void shutdown();

    /// Begin a new 3D scene with a perspective camera
    static void beginScene(const PerspectiveCamera& camera);

    /// Begin a new 3D scene with explicit view-projection matrix
    static void beginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition);

    /// End the current scene and flush all draw calls
    static void endScene();

    /// Submit a mesh for rendering with a material and transform
    static void submit(const Mesh& mesh, const Material& material, const glm::mat4& transform);

    /// Submit a model (mesh hierarchy with materials) for rendering
    static void submit(const Model& model, const glm::mat4& transform);

    /// Submit multiple instances of the same mesh (instanced rendering)
    static void submitInstanced(const Mesh& mesh, const Material& material,
                                const std::vector<glm::mat4>& transforms);

    // Lighting
    static void setAmbientLight(const glm::vec3& color, float intensity = 1.0f);

    // Debug rendering
    static void drawLine(const glm::vec3& start, const glm::vec3& end,
                         const glm::vec4& color = glm::vec4(1.0f));
    static void drawWireBox(const glm::vec3& center, const glm::vec3& size,
                            const glm::vec4& color = glm::vec4(1.0f));
    static void drawWireSphere(const glm::vec3& center, float radius,
                               const glm::vec4& color = glm::vec4(1.0f));

    // Statistics
    struct Statistics {
        u32 drawCalls = 0;
        u32 triangleCount = 0;
        u32 meshCount = 0;
    };

    static Statistics getStatistics();
    static void resetStatistics();

private:
    static void flush();
};

}  // namespace limbo
