#include "limbo/ecs/System.hpp"
#include "limbo/ecs/World.hpp"

#include <algorithm>
#include <ranges>

namespace limbo {

SystemManager::~SystemManager() = default;

void SystemManager::init(World& world) {
    sortSystems();
    for (auto& system : m_systems) {
        system->onAttach(world);
    }
}

void SystemManager::shutdown(World& world) {
    // Shutdown in reverse order
    for (auto& m_system : std::ranges::reverse_view(m_systems)) {
        m_system->onDetach(world);
    }
}

void SystemManager::update(World& world, f32 deltaTime) {
    if (!m_sorted) {
        sortSystems();
    }

    for (auto& system : m_systems) {
        if (system->isEnabled()) {
            system->update(world, deltaTime);
        }
    }
}

void SystemManager::fixedUpdate(World& world, f32 fixedDeltaTime) {
    if (!m_sorted) {
        sortSystems();
    }

    for (auto& system : m_systems) {
        if (system->isEnabled()) {
            system->fixedUpdate(world, fixedDeltaTime);
        }
    }
}

void SystemManager::sortSystems() {
    std::stable_sort(m_systems.begin(), m_systems.end(),
                     [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
                         return a->priority() < b->priority();
                     });
    m_sorted = true;
}

}  // namespace limbo
