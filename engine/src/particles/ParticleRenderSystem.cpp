#include "limbo/particles/ParticleRenderSystem.hpp"
#include "limbo/particles/ParticleComponents.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Components.hpp"
#include "limbo/render/2d/Renderer2D.hpp"
#include "limbo/debug/Log.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace limbo {

ParticleRenderSystem::ParticleRenderSystem(u32 maxParticles) : m_pool(maxParticles) {}

void ParticleRenderSystem::onAttach(World& world) {
    (void)world;
    LIMBO_LOG_RENDER_DEBUG("ParticleRenderSystem attached (max particles: {})",
                           m_pool.getMaxParticles());
}

void ParticleRenderSystem::update(World& world, f32 deltaTime) {
    // Process all emitter components
    world.each<TransformComponent, ParticleEmitterComponent>(
        [this, deltaTime](World::EntityId, TransformComponent& transform,
                          ParticleEmitterComponent& emitter) {
            if (!emitter.emitting) {
                return;
            }

            // Update emitter position from transform
            ParticleEmitterProps props = emitter.props;
            props.position = transform.position;

            // Emit particles based on rate
            if (props.emissionRate > 0.0f) {
                emitter.emitAccumulator += deltaTime;
                f32 const emitInterval = 1.0f / props.emissionRate;

                while (emitter.emitAccumulator >= emitInterval) {
                    emitter.emitAccumulator -= emitInterval;
                    m_pool.emit(props);
                }
            }
        });

    // Update all particles (physics, lifetime)
    m_pool.update(deltaTime);

    // Apply acceleration to particles
    for (auto& particle : const_cast<std::vector<Particle>&>(m_pool.getParticles())) {
        if (particle.active) {
            // Find the emitter props for this particle's acceleration
            // For simplicity, we apply a default gravity-like acceleration
            // In a more complex system, each particle would store its own acceleration
            particle.velocity.y -= 2.0f * deltaTime;  // Simple gravity
        }
    }
}

void ParticleRenderSystem::onDetach(World& world) {
    (void)world;
    m_pool.clear();
    LIMBO_LOG_RENDER_DEBUG("ParticleRenderSystem detached");
}

void ParticleRenderSystem::render() {
    const auto& particles = m_pool.getParticles();

    for (const auto& particle : particles) {
        if (!particle.active) {
            continue;
        }

        // Calculate life progress (0 = just born, 1 = about to die)
        f32 const lifeProgress = 1.0f - (particle.lifeRemaining / particle.lifetime);

        // Interpolate color
        glm::vec4 const color = glm::mix(particle.colorStart, particle.colorEnd, lifeProgress);

        // Interpolate size
        f32 const size = glm::mix(particle.sizeStart, particle.sizeEnd, lifeProgress);

        // Render the particle
        Renderer2D::drawRotatedQuad(particle.position, glm::vec2(size), particle.rotation, color);
    }
}

}  // namespace limbo
