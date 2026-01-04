#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/render/3d/Mesh.hpp"
#include "limbo/render/3d/Material.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * @brief Node in a model hierarchy
 */
struct ModelNode {
    String name;
    glm::mat4 localTransform{1.0f};
    std::vector<u32> meshIndices;
    std::vector<Unique<ModelNode>> children;

    /// Get world transform (requires parent transform)
    [[nodiscard]] glm::mat4 getWorldTransform(const glm::mat4& parentTransform) const {
        return parentTransform * localTransform;
    }
};

/**
 * @brief 3D model containing mesh hierarchy and materials
 *
 * A Model represents a complete 3D asset, potentially containing multiple
 * meshes organized in a hierarchy with associated materials. Models are
 * typically loaded from files (glTF, OBJ, FBX, etc.).
 */
class Model {
public:
    Model() = default;
    ~Model() = default;

    // Non-copyable, movable
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    /// Load model from file
    bool load(const String& filepath);

    /// Get all meshes in the model
    [[nodiscard]] const std::vector<Shared<Mesh>>& getMeshes() const { return m_meshes; }

    /// Get all materials in the model
    [[nodiscard]] const std::vector<Shared<Material>>& getMaterials() const { return m_materials; }

    /// Get the root node of the hierarchy
    [[nodiscard]] const ModelNode* getRootNode() const { return m_rootNode.get(); }

    /// Get the bounding box encompassing the entire model
    [[nodiscard]] const AABB& getBoundingBox() const { return m_boundingBox; }

    /// Get the file path this model was loaded from
    [[nodiscard]] const String& getFilePath() const { return m_filepath; }

    /// Check if model is valid (loaded successfully)
    [[nodiscard]] bool isValid() const { return !m_meshes.empty(); }

    // Supported file formats
    static std::vector<String> getSupportedExtensions();
    static bool isFormatSupported(const String& extension);

private:
    std::vector<Shared<Mesh>> m_meshes;
    std::vector<Shared<Material>> m_materials;
    Unique<ModelNode> m_rootNode;
    AABB m_boundingBox;
    String m_filepath;

    bool loadOBJ(const String& filepath);
    bool loadGLTF(const String& filepath);
    // Future: bool loadFBX(const String& filepath);

    void calculateBoundingBox();
};

}  // namespace limbo
