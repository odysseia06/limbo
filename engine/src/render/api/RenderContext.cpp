#include "limbo/render/RenderContext.hpp"

// Forward declaration of OpenGL context creation
namespace limbo {
Unique<RenderContext> createOpenGLContext();
}

namespace limbo {

Unique<RenderContext> RenderContext::create() {
    // Currently only OpenGL is supported
    return createOpenGLContext();
}

}  // namespace limbo
