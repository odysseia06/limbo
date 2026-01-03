#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

namespace limbo {

class Window;
class VertexArray;

class LIMBO_API RenderContext {
public:
    virtual ~RenderContext() = default;

    virtual bool init(Window& window) = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void clear(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void setViewport(i32 x, i32 y, i32 width, i32 height) = 0;

    // Draw indexed geometry from a vertex array
    virtual void drawIndexed(const VertexArray& vao, u32 indexCount = 0) = 0;

    // Draw non-indexed geometry
    virtual void drawArrays(const VertexArray& vao, u32 vertexCount) = 0;

    [[nodiscard]] static Unique<RenderContext> create();
};

} // namespace limbo
