#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

namespace limbo {

// Placeholder for Scene class - will be expanded in M7
class LIMBO_API Scene {
public:
    Scene() = default;
    ~Scene() = default;

    void update(f32 deltaTime);
};

}  // namespace limbo
