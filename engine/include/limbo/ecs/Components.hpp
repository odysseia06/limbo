#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace limbo {

// Name component - gives an entity a human-readable name
struct NameComponent {
    String name;

    NameComponent() = default;
    explicit NameComponent(String n) : name(std::move(n)) {}
};

// Transform component - position, rotation, scale in 3D space
struct TransformComponent {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};  // Euler angles in radians
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(const glm::vec3& pos) : position(pos) {}
    TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl)
        : position(pos), rotation(rot), scale(scl) {}

    [[nodiscard]] glm::mat4 getMatrix() const {
        glm::mat4 mat(1.0f);

        // Translation
        mat = glm::translate(mat, position);

        // Rotation (Z * Y * X order)
        mat = glm::rotate(mat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

        // Scale
        mat = glm::scale(mat, scale);

        return mat;
    }
};

// Sprite renderer component - for 2D rendering
struct SpriteRendererComponent {
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    AssetId textureId = AssetId::invalid();
    i32 sortingOrder = 0;

    // UV coordinates for sprite sheet support
    glm::vec2 uvMin{0.0f, 0.0f};
    glm::vec2 uvMax{1.0f, 1.0f};

    SpriteRendererComponent() = default;
    explicit SpriteRendererComponent(const glm::vec4& col) : color(col) {}
    SpriteRendererComponent(const glm::vec4& col, AssetId texId) : color(col), textureId(texId) {}
};

// Mesh renderer component - for 3D rendering
struct MeshRendererComponent {
    AssetId meshId = AssetId::invalid();
    AssetId materialId = AssetId::invalid();
    bool castShadows = true;
    bool receiveShadows = true;

    MeshRendererComponent() = default;
    MeshRendererComponent(AssetId mesh, AssetId material) : meshId(mesh), materialId(material) {}
};

// Camera component - defines a camera for rendering
struct CameraComponent {
    enum class ProjectionType : u8 { Perspective, Orthographic };

    ProjectionType projectionType = ProjectionType::Perspective;
    f32 fov = glm::radians(45.0f);  // For perspective
    f32 orthoSize = 5.0f;           // For orthographic (half-height)
    f32 nearClip = 0.1f;
    f32 farClip = 1000.0f;
    bool primary = true;  // Is this the main camera?

    CameraComponent() = default;

    [[nodiscard]] glm::mat4 getProjectionMatrix(f32 aspectRatio) const {
        if (projectionType == ProjectionType::Perspective) {
            return glm::perspective(fov, aspectRatio, nearClip, farClip);
        } else {
            f32 left = -orthoSize * aspectRatio;
            f32 right = orthoSize * aspectRatio;
            f32 bottom = -orthoSize;
            f32 top = orthoSize;
            return glm::ortho(left, right, bottom, top, nearClip, farClip);
        }
    }
};

// Tag components for entity categorization
struct ActiveComponent {};  // Entity is active/enabled

struct StaticComponent {};  // Entity doesn't move (optimization hint)

}  // namespace limbo
