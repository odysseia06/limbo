#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/tilemap/Tilemap.hpp"
#include "limbo/tilemap/Tileset.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace limbo {

/**
 * TilemapComponent - Attaches a tilemap to an entity
 *
 * The tilemap will be positioned relative to the entity's transform.
 */
struct LIMBO_API TilemapComponent {
    Shared<Tilemap> tilemap;
    Shared<Tileset> tileset;
    glm::vec2 offset{0.0f};  // Offset from entity position
    bool visible = true;

    TilemapComponent() = default;

    explicit TilemapComponent(Shared<Tilemap> map, Shared<Tileset> set)
        : tilemap(std::move(map)), tileset(std::move(set)) {}
};

}  // namespace limbo
