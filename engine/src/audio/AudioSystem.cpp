#include "limbo/audio/AudioSystem.hpp"
#include "limbo/audio/AudioComponents.hpp"
#include "limbo/ecs/World.hpp"
#include <spdlog/spdlog.h>

namespace limbo {

AudioSystem::AudioSystem(AudioEngine& engine) : m_engine(engine) {}

void AudioSystem::onAttach(World& /*world*/) {
    spdlog::debug("AudioSystem initialized");
}

void AudioSystem::update(World& world, f32 /*deltaTime*/) {
    // Process all entities with AudioSourceComponent
    world.each<AudioSourceComponent>(
        [this, &world](World::EntityId entity, AudioSourceComponent& audio) {
            // Create source if not yet created
            if (!audio.runtimeSource && audio.audioAsset && audio.audioAsset->isLoaded()) {
                createSource(world, entity);

                // Handle play-on-start
                if (audio.playOnStart && audio.runtimeSource) {
                    audio.runtimeSource->play();
                }
            }

            // Sync properties
            if (audio.runtimeSource) {
                syncSourceProperties(world, entity);
            }
        });
}

void AudioSystem::onDetach(World& world) {
    // Clean up all sources
    world.each<AudioSourceComponent>(
        [this, &world](World::EntityId entity, AudioSourceComponent& audio) {
            if (audio.runtimeSource) {
                destroySource(world, entity);
            }
        });

    m_sources.clear();
    spdlog::debug("AudioSystem shutdown");
}

void AudioSystem::play(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);

    // Create source if needed
    if (!audio.runtimeSource && audio.audioAsset && audio.audioAsset->isLoaded()) {
        createSource(world, entity);
    }

    if (audio.runtimeSource) {
        audio.runtimeSource->play();
    }
}

void AudioSystem::pause(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);
    if (audio.runtimeSource) {
        audio.runtimeSource->pause();
    }
}

void AudioSystem::stop(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);
    if (audio.runtimeSource) {
        audio.runtimeSource->stop();
    }
}

void AudioSystem::createSource(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);

    if (audio.runtimeSource) {
        return;  // Already has a source
    }

    if (!audio.audioAsset || !audio.audioAsset->isLoaded()) {
        return;
    }

    // Create new source
    auto source = std::make_unique<AudioSource>();
    source->setClip(audio.audioAsset->getClip());
    source->setVolume(audio.volume);
    source->setPitch(audio.pitch);
    source->setLooping(audio.loop);

    // Register with engine
    m_engine.registerSource(source.get());

    // Store pointer in component
    audio.runtimeSource = source.get();

    // Move to our storage
    m_sources.push_back(std::move(source));
}

void AudioSystem::destroySource(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);

    if (!audio.runtimeSource) {
        return;
    }

    // Unregister from engine
    m_engine.unregisterSource(audio.runtimeSource);

    // Find and remove from our storage
    auto it =
        std::find_if(m_sources.begin(), m_sources.end(), [&audio](const Unique<AudioSource>& src) {
            return src.get() == audio.runtimeSource;
        });

    if (it != m_sources.end()) {
        m_sources.erase(it);
    }

    audio.runtimeSource = nullptr;
}

void AudioSystem::syncSourceProperties(World& world, World::EntityId entity) {
    if (!world.hasComponent<AudioSourceComponent>(entity)) {
        return;
    }

    auto& audio = world.getComponent<AudioSourceComponent>(entity);

    if (!audio.runtimeSource) {
        return;
    }

    // Sync properties from component to source
    audio.runtimeSource->setVolume(audio.volume);
    audio.runtimeSource->setPitch(audio.pitch);
    audio.runtimeSource->setLooping(audio.loop);
}

}  // namespace limbo
