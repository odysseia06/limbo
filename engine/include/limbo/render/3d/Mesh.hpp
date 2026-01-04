#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/render/common/Buffer.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * @brief Vertex structure for 3D meshes
 */
struct Vertex3D {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 texCoord{0.0f};
    glm::vec3 tangent{1.0f, 0.0f, 0.0f};
    glm::vec3 bitangent{0.0f, 0.0f, 1.0f};

    // For skeletal animation (future)
    glm::ivec4 boneIds{-1, -1, -1, -1};
    glm::vec4 boneWeights{0.0f};

    static BufferLayout getLayout();
};

/**
 * @brief Axis-aligned bounding box
 */
struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    [[nodiscard]] glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    [[nodiscard]] glm::vec3 getSize() const { return max - min; }
    [[nodiscard]] glm::vec3 getExtents() const { return getSize() * 0.5f; }

    void expand(const glm::vec3& point);
    void expand(const AABB& other);
    [[nodiscard]] bool contains(const glm::vec3& point) const;
    [[nodiscard]] bool intersects(const AABB& other) const;
};

/**
 * @brief Submesh definition for multi-part meshes
 */
struct Submesh {
    u32 baseVertex = 0;
    u32 baseIndex = 0;
    u32 indexCount = 0;
    u32 materialIndex = 0;
    AABB boundingBox;
    String name;
};

/**
 * @brief 3D mesh containing vertex and index data
 *
 * A Mesh represents renderable 3D geometry with support for multiple
 * submeshes (e.g., for models with multiple materials).
 */
class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex3D>& vertices, const std::vector<u32>& indices);
    ~Mesh() = default;

    // Non-copyable, movable
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    /// Create mesh from vertex and index data
    void create(const std::vector<Vertex3D>& vertices, const std::vector<u32>& indices);

    /// Add a submesh definition
    void addSubmesh(const Submesh& submesh);

    /// Get all submeshes
    [[nodiscard]] const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

    /// Get the bounding box encompassing all geometry
    [[nodiscard]] const AABB& getBoundingBox() const { return m_boundingBox; }

    /// Get vertex count
    [[nodiscard]] u32 getVertexCount() const { return m_vertexCount; }

    /// Get index count
    [[nodiscard]] u32 getIndexCount() const { return m_indexCount; }

    /// Bind for rendering
    void bind() const;

    /// Unbind after rendering
    void unbind() const;

    // Primitive mesh generators
    static Shared<Mesh> createCube(float size = 1.0f);
    static Shared<Mesh> createSphere(float radius = 0.5f, u32 segments = 32, u32 rings = 16);
    static Shared<Mesh> createPlane(float width = 1.0f, float height = 1.0f, u32 subdivisions = 1);
    static Shared<Mesh> createCylinder(float radius = 0.5f, float height = 1.0f, u32 segments = 32);
    static Shared<Mesh> createCapsule(float radius = 0.5f, float height = 2.0f, u32 segments = 32);
    static Shared<Mesh> createCone(float radius = 0.5f, float height = 1.0f, u32 segments = 32);
    static Shared<Mesh> createTorus(float majorRadius = 0.5f, float minorRadius = 0.2f,
                                    u32 majorSegments = 32, u32 minorSegments = 16);

private:
    Unique<VertexArray> m_vertexArray;
    std::vector<Submesh> m_submeshes;
    AABB m_boundingBox;
    u32 m_vertexCount = 0;
    u32 m_indexCount = 0;

    void calculateBoundingBox(const std::vector<Vertex3D>& vertices);
};

}  // namespace limbo
