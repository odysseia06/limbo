#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/particles/ParticleSystem.hpp"

namespace limbo {

/**
 * ParticleEmitterComponent - Attaches a particle emitter to an entity
 * 
 * The emitter's position will follow the entity's transform.
 */
struct LIMBO_API ParticleEmitterComponent {
    ParticleEmitterProps props;
    bool emitting = true;
    bool localSpace = false;    // If true, particles move with the entity
    
    // Runtime state (managed by ParticleRenderSystem)
    f32 emitAccumulator = 0.0f;
    
    ParticleEmitterComponent() = default;
    explicit ParticleEmitterComponent(const ParticleEmitterProps& emitterProps)
        : props(emitterProps) {}
};

} // namespace limbo
