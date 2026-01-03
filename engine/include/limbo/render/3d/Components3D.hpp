#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/assets/AssetId.hpp"
#include "limbo/render/3d/Lighting.hpp"

#include <glm/glm.hpp>

namespace limbo {

// Note: MeshRendererComponent is defined in limbo/ecs/Components.hpp
// This file provides additional 3D-specific components

/**
 * @brief Component for mesh filtering (submesh selection)
 */
struct MeshFilterComponent {
    AssetId meshId;
    u32 submeshIndex = 0;
};

/**
 * @brief Component for skybox rendering
 */
struct SkyboxComponent {
    AssetId cubemapId;
    float intensity = 1.0f;
};

// LightComponent is defined in Lighting.hpp

}  // namespace limbo
