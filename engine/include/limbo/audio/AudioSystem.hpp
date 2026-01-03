#pragma once

#include "limbo/core/Types.hpp"
#include "limbo/core/Base.hpp"
#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"
#include "limbo/audio/AudioEngine.hpp"

namespace limbo {

/**
 * AudioSystem - ECS system that manages audio playback
 *
 * This system:
 * - Creates AudioSource instances for entities with AudioSourceComponent
 * - Syncs component properties to the AudioSource
 * - Handles play-on-start behavior
 * - Cleans up audio sources when entities are destroyed
 */
class LIMBO_API AudioSystem : public System {
public:
    explicit AudioSystem(AudioEngine& engine);
    ~AudioSystem() override = default;

    void onAttach(World& world) override;
    void update(World& world, f32 deltaTime) override;
    void onDetach(World& world) override;

    /// Play audio on a specific entity
    void play(World& world, World::EntityId entity);

    /// Pause audio on a specific entity
    void pause(World& world, World::EntityId entity);

    /// Stop audio on a specific entity
    void stop(World& world, World::EntityId entity);

private:
    void createSource(World& world, World::EntityId entity);
    void destroySource(World& world, World::EntityId entity);
    void syncSourceProperties(World& world, World::EntityId entity);

    AudioEngine& m_engine;
    std::vector<Unique<AudioSource>> m_sources;
};

}  // namespace limbo
