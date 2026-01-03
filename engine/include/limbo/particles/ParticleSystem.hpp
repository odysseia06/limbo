#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <random>

namespace limbo {

/**
 * Particle - A single particle in the system
 */
struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec4 colorStart{1.0f};
    glm::vec4 colorEnd{1.0f, 1.0f, 1.0f, 0.0f};
    f32 sizeStart = 0.1f;
    f32 sizeEnd = 0.0f;
    f32 rotation = 0.0f;
    f32 rotationSpeed = 0.0f;
    f32 lifetime = 1.0f;
    f32 lifeRemaining = 0.0f;
    bool active = false;
};

/**
 * ParticleEmitterProps - Configuration for particle emission
 */
struct ParticleEmitterProps {
    // Emission
    glm::vec3 position{0.0f};
    glm::vec3 positionVariance{0.0f};       // Random offset from position
    
    // Velocity
    glm::vec3 velocity{0.0f, 1.0f, 0.0f};
    glm::vec3 velocityVariance{0.5f, 0.5f, 0.0f};
    
    // Acceleration (gravity, etc)
    glm::vec3 acceleration{0.0f, -2.0f, 0.0f};
    
    // Color
    glm::vec4 colorStart{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 colorEnd{1.0f, 1.0f, 1.0f, 0.0f};
    glm::vec4 colorVariance{0.0f};          // Random color variation
    
    // Size
    f32 sizeStart = 0.1f;
    f32 sizeEnd = 0.0f;
    f32 sizeVariance = 0.0f;
    
    // Rotation
    f32 rotation = 0.0f;
    f32 rotationVariance = 0.0f;
    f32 rotationSpeed = 0.0f;
    f32 rotationSpeedVariance = 0.0f;
    
    // Lifetime
    f32 lifetime = 1.0f;
    f32 lifetimeVariance = 0.0f;
    
    // Emission rate
    f32 emissionRate = 10.0f;               // Particles per second
    u32 burstCount = 0;                     // Emit this many at once (0 = continuous)
};

/**
 * ParticlePool - Manages a pool of reusable particles
 */
class LIMBO_API ParticlePool {
public:
    explicit ParticlePool(u32 maxParticles = 1000);
    
    void update(f32 deltaTime);
    void emit(const ParticleEmitterProps& props);
    void burst(const ParticleEmitterProps& props, u32 count);
    void clear();
    
    [[nodiscard]] const std::vector<Particle>& getParticles() const { return m_particles; }
    [[nodiscard]] u32 getActiveCount() const { return m_activeCount; }
    [[nodiscard]] u32 getMaxParticles() const { return m_maxParticles; }
    
    void setMaxParticles(u32 max);

private:
    Particle& getNextParticle();
    void initializeParticle(Particle& particle, const ParticleEmitterProps& props);
    
    f32 randomFloat(f32 min, f32 max);
    f32 randomVariance(f32 base, f32 variance);
    glm::vec3 randomVariance(const glm::vec3& base, const glm::vec3& variance);
    glm::vec4 randomVariance(const glm::vec4& base, const glm::vec4& variance);

    std::vector<Particle> m_particles;
    u32 m_maxParticles;
    u32 m_activeCount = 0;
    u32 m_poolIndex = 0;
    
    std::mt19937 m_rng;
    std::uniform_real_distribution<f32> m_dist{0.0f, 1.0f};
};

/**
 * ParticleEmitter - Emits particles over time
 */
class LIMBO_API ParticleEmitter {
public:
    ParticleEmitter() = default;
    explicit ParticleEmitter(ParticlePool* pool);
    
    void update(f32 deltaTime);
    void emit();
    void burst(u32 count);
    void start();
    void stop();
    
    void setPool(ParticlePool* pool) { m_pool = pool; }
    [[nodiscard]] ParticlePool* getPool() const { return m_pool; }
    
    [[nodiscard]] bool isEmitting() const { return m_emitting; }
    
    ParticleEmitterProps props;

private:
    ParticlePool* m_pool = nullptr;
    f32 m_emitAccumulator = 0.0f;
    bool m_emitting = false;
};

} // namespace limbo
