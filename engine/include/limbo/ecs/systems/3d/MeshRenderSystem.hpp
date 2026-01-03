#pragma once

#include "limbo/ecs/System.hpp"
#include "limbo/render/common/Camera.hpp"

namespace limbo {

/**
 * @brief System for rendering 3D meshes
 *
 * MeshRenderSystem iterates over entities with TransformComponent and
 * MeshRendererComponent, rendering them using Renderer3D.
 */
class MeshRenderSystem : public System {
public:
    MeshRenderSystem() = default;
    ~MeshRenderSystem() override = default;

    void onAttach(World& world) override;
    void onDetach(World& world) override;
    void update(World& world, f32 deltaTime) override;

    /// Render all meshes using the provided camera
    void render(const PerspectiveCamera& camera);

    /// Render all meshes using explicit view-projection matrix
    void render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition);

private:
    World* m_world = nullptr;
};

}  // namespace limbo
