#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/render/common/Texture.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace limbo {

/**
 * Represents a single frame/sprite within a sprite sheet
 */
struct LIMBO_API SpriteFrame {
    /// UV coordinates (normalized 0-1)
    glm::vec2 uvMin{0.0f, 0.0f};
    glm::vec2 uvMax{1.0f, 1.0f};

    /// Frame size in pixels
    glm::vec2 size{0.0f, 0.0f};

    /// Pivot point (normalized, 0.5 = center)
    glm::vec2 pivot{0.5f, 0.5f};

    /// Optional frame name
    String name;
};

/**
 * SpriteSheet - A texture atlas containing multiple sprite frames
 *
 * Supports:
 * - Grid-based layouts (uniform cell sizes)
 * - Custom frame definitions
 * - Named frame lookup
 */
class LIMBO_API SpriteSheet {
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    // Non-copyable, movable
    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;
    SpriteSheet(SpriteSheet&&) = default;
    SpriteSheet& operator=(SpriteSheet&&) = default;

    /**
     * Set the texture for this sprite sheet
     */
    void setTexture(Texture2D* texture);
    [[nodiscard]] Texture2D* getTexture() const { return m_texture; }

    /**
     * Create frames from a grid layout
     * @param cellWidth Width of each cell in pixels
     * @param cellHeight Height of each cell in pixels
     * @param frameCount Number of frames to create (0 = auto-calculate)
     * @param startIndex Starting cell index (for partial sheets)
     */
    void createFromGrid(u32 cellWidth, u32 cellHeight, u32 frameCount = 0, u32 startIndex = 0);

    /**
     * Add a custom frame
     * @param x X position in pixels
     * @param y Y position in pixels
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param name Optional frame name
     */
    void addFrame(u32 x, u32 y, u32 width, u32 height, const String& name = "");

    /**
     * Add a frame with custom pivot
     */
    void addFrame(u32 x, u32 y, u32 width, u32 height, const glm::vec2& pivot,
                  const String& name = "");

    /**
     * Get frame by index
     */
    [[nodiscard]] const SpriteFrame& getFrame(usize index) const;

    /**
     * Get frame by name
     */
    [[nodiscard]] const SpriteFrame* getFrameByName(const String& name) const;

    /**
     * Get frame index by name
     */
    [[nodiscard]] i32 getFrameIndex(const String& name) const;

    /**
     * Get total frame count
     */
    [[nodiscard]] usize getFrameCount() const { return m_frames.size(); }

    /**
     * Clear all frames
     */
    void clear();

    /**
     * Check if sprite sheet is valid (has texture and frames)
     */
    [[nodiscard]] bool isValid() const { return m_texture != nullptr && !m_frames.empty(); }

private:
    Texture2D* m_texture = nullptr;
    std::vector<SpriteFrame> m_frames;
};

}  // namespace limbo
