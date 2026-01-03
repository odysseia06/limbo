#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/particles/ParticleSystem.hpp"

namespace limbo {

/**
 * ParticleRenderSystem - Updates and renders particles from ParticleEmitterComponents
 * 
 * This system manages a shared particle pool and handles:
 * - Emitting particles based on emitter components
 * - Updating particle physics (position, velocity, lifetime)
 * - Rendering all active particles
 */
class LIMBO_API ParticleRenderSystem : public System {
public:
    explicit ParticleRenderSystem(u32 maxParticles = 10000);
    
    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;
    
    /**
     * Render all active particles
     * Call this during the render phase (after beginScene, before endScene)
     */
    void render();
    
    /**
     * Get the particle pool for direct access
     */
    [[nodiscard]] ParticlePool& getPool() { return m_pool; }
    [[nodiscard]] const ParticlePool& getPool() const { return m_pool; }

private:
    ParticlePool m_pool;
};

} // namespace limbo
