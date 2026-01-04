#include "limbo/particles/ParticleSystem.hpp"

#include <algorithm>
#include <cmath>

namespace limbo {

// ============================================================================
// ParticlePool
// ============================================================================

ParticlePool::ParticlePool(u32 maxParticles)
    : m_maxParticles(maxParticles), m_rng(std::random_device{}()) {
    m_particles.resize(maxParticles);
}

void ParticlePool::update(f32 deltaTime) {
    m_activeCount = 0;

    for (auto& particle : m_particles) {
        if (!particle.active) {
            continue;
        }

        particle.lifeRemaining -= deltaTime;

        if (particle.lifeRemaining <= 0.0f) {
            particle.active = false;
            continue;
        }

        // Update position with velocity
        particle.position += particle.velocity * deltaTime;

        // Update rotation
        particle.rotation += particle.rotationSpeed * deltaTime;

        ++m_activeCount;
    }
}

void ParticlePool::emit(const ParticleEmitterProps& props) {
    Particle& particle = getNextParticle();
    initializeParticle(particle, props);
}

void ParticlePool::burst(const ParticleEmitterProps& props, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        emit(props);
    }
}

void ParticlePool::clear() {
    for (auto& particle : m_particles) {
        particle.active = false;
    }
    m_activeCount = 0;
    m_poolIndex = 0;
}

void ParticlePool::setMaxParticles(u32 max) {
    m_maxParticles = max;
    m_particles.resize(max);
    m_poolIndex = 0;
}

Particle& ParticlePool::getNextParticle() {
    // Simple ring buffer approach
    u32 const startIndex = m_poolIndex;

    // First, try to find an inactive particle
    do {
        if (!m_particles[m_poolIndex].active) {
            u32 const index = m_poolIndex;
            m_poolIndex = (m_poolIndex + 1) % m_maxParticles;
            return m_particles[index];
        }
        m_poolIndex = (m_poolIndex + 1) % m_maxParticles;
    } while (m_poolIndex != startIndex);

    // All particles active, reuse the oldest (current index)
    u32 const index = m_poolIndex;
    m_poolIndex = (m_poolIndex + 1) % m_maxParticles;
    return m_particles[index];
}

void ParticlePool::initializeParticle(Particle& particle, const ParticleEmitterProps& props) {
    particle.active = true;

    // Position with variance
    particle.position = randomVariance(props.position, props.positionVariance);

    // Velocity with variance
    particle.velocity = randomVariance(props.velocity, props.velocityVariance);

    // Color
    particle.colorStart = randomVariance(props.colorStart, props.colorVariance);
    particle.colorEnd = randomVariance(props.colorEnd, props.colorVariance);

    // Clamp colors to valid range
    particle.colorStart = glm::clamp(particle.colorStart, glm::vec4(0.0f), glm::vec4(1.0f));
    particle.colorEnd = glm::clamp(particle.colorEnd, glm::vec4(0.0f), glm::vec4(1.0f));

    // Size
    particle.sizeStart = randomVariance(props.sizeStart, props.sizeVariance);
    particle.sizeEnd = randomVariance(props.sizeEnd, props.sizeVariance);
    particle.sizeStart = std::max(0.0f, particle.sizeStart);
    particle.sizeEnd = std::max(0.0f, particle.sizeEnd);

    // Rotation
    particle.rotation = randomVariance(props.rotation, props.rotationVariance);
    particle.rotationSpeed = randomVariance(props.rotationSpeed, props.rotationSpeedVariance);

    // Lifetime
    particle.lifetime = randomVariance(props.lifetime, props.lifetimeVariance);
    particle.lifetime = std::max(0.01f, particle.lifetime);
    particle.lifeRemaining = particle.lifetime;
}

f32 ParticlePool::randomFloat(f32 min, f32 max) {
    return min + m_dist(m_rng) * (max - min);
}

f32 ParticlePool::randomVariance(f32 base, f32 variance) {
    if (variance <= 0.0f) {
        return base;
}
    return base + randomFloat(-variance, variance);
}

glm::vec3 ParticlePool::randomVariance(const glm::vec3& base, const glm::vec3& variance) {
    return glm::vec3(randomVariance(base.x, variance.x), randomVariance(base.y, variance.y),
                     randomVariance(base.z, variance.z));
}

glm::vec4 ParticlePool::randomVariance(const glm::vec4& base, const glm::vec4& variance) {
    return glm::vec4(randomVariance(base.x, variance.x), randomVariance(base.y, variance.y),
                     randomVariance(base.z, variance.z), randomVariance(base.w, variance.w));
}

// ============================================================================
// ParticleEmitter
// ============================================================================

ParticleEmitter::ParticleEmitter(ParticlePool* pool) : m_pool(pool) {}

void ParticleEmitter::update(f32 deltaTime) {
    if (!m_emitting || !m_pool || props.emissionRate <= 0.0f) {
        return;
    }

    // Accumulate time and emit particles
    m_emitAccumulator += deltaTime;
    f32 const emitInterval = 1.0f / props.emissionRate;

    while (m_emitAccumulator >= emitInterval) {
        m_emitAccumulator -= emitInterval;
        emit();
    }
}

void ParticleEmitter::emit() {
    if (m_pool) {
        m_pool->emit(props);
    }
}

void ParticleEmitter::burst(u32 count) {
    if (m_pool) {
        m_pool->burst(props, count);
    }
}

void ParticleEmitter::start() {
    m_emitting = true;
    m_emitAccumulator = 0.0f;
}

void ParticleEmitter::stop() {
    m_emitting = false;
}

}  // namespace limbo
