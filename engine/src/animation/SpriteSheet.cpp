#include "limbo/animation/SpriteSheet.hpp"
#include <spdlog/spdlog.h>

namespace limbo {

void SpriteSheet::setTexture(Texture2D* texture) {
    m_texture = texture;
}

void SpriteSheet::createFromGrid(u32 cellWidth, u32 cellHeight, u32 frameCount, u32 startIndex) {
    if (!m_texture) {
        spdlog::warn("SpriteSheet::createFromGrid: No texture set");
        return;
    }

    const u32 texWidth = m_texture->getWidth();
    const u32 texHeight = m_texture->getHeight();

    if (cellWidth == 0 || cellHeight == 0) {
        spdlog::warn("SpriteSheet::createFromGrid: Invalid cell dimensions");
        return;
    }

    const u32 cols = texWidth / cellWidth;
    const u32 rows = texHeight / cellHeight;
    const u32 totalCells = cols * rows;

    if (frameCount == 0) {
        frameCount = totalCells - startIndex;
    }

    m_frames.clear();
    m_frames.reserve(frameCount);

    for (u32 i = 0; i < frameCount; ++i) {
        u32 cellIndex = startIndex + i;
        if (cellIndex >= totalCells) {
            break;
        }

        u32 col = cellIndex % cols;
        u32 row = cellIndex / cols;

        SpriteFrame frame;
        frame.uvMin.x = static_cast<f32>(col * cellWidth) / static_cast<f32>(texWidth);
        frame.uvMin.y = static_cast<f32>(row * cellHeight) / static_cast<f32>(texHeight);
        frame.uvMax.x = static_cast<f32>((col + 1) * cellWidth) / static_cast<f32>(texWidth);
        frame.uvMax.y = static_cast<f32>((row + 1) * cellHeight) / static_cast<f32>(texHeight);
        frame.size = glm::vec2(static_cast<f32>(cellWidth), static_cast<f32>(cellHeight));
        frame.pivot = glm::vec2(0.5f, 0.5f);
        frame.name = "frame_" + std::to_string(i);

        m_frames.push_back(frame);
    }

    spdlog::debug("SpriteSheet: Created {} frames from {}x{} grid", m_frames.size(), cols, rows);
}

void SpriteSheet::addFrame(u32 x, u32 y, u32 width, u32 height, const String& name) {
    addFrame(x, y, width, height, glm::vec2(0.5f, 0.5f), name);
}

void SpriteSheet::addFrame(u32 x, u32 y, u32 width, u32 height, const glm::vec2& pivot,
                           const String& name) {
    if (!m_texture) {
        spdlog::warn("SpriteSheet::addFrame: No texture set");
        return;
    }

    const f32 texWidth = static_cast<f32>(m_texture->getWidth());
    const f32 texHeight = static_cast<f32>(m_texture->getHeight());

    SpriteFrame frame;
    frame.uvMin.x = static_cast<f32>(x) / texWidth;
    frame.uvMin.y = static_cast<f32>(y) / texHeight;
    frame.uvMax.x = static_cast<f32>(x + width) / texWidth;
    frame.uvMax.y = static_cast<f32>(y + height) / texHeight;
    frame.size = glm::vec2(static_cast<f32>(width), static_cast<f32>(height));
    frame.pivot = pivot;
    frame.name = name.empty() ? "frame_" + std::to_string(m_frames.size()) : name;

    m_frames.push_back(frame);
}

const SpriteFrame& SpriteSheet::getFrame(usize index) const {
    static SpriteFrame defaultFrame;
    if (index >= m_frames.size()) {
        spdlog::warn("SpriteSheet::getFrame: Index {} out of bounds", index);
        return defaultFrame;
    }
    return m_frames[index];
}

const SpriteFrame* SpriteSheet::getFrameByName(const String& name) const {
    for (const auto& frame : m_frames) {
        if (frame.name == name) {
            return &frame;
        }
    }
    return nullptr;
}

i32 SpriteSheet::getFrameIndex(const String& name) const {
    for (usize i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].name == name) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

void SpriteSheet::clear() {
    m_frames.clear();
}

}  // namespace limbo
