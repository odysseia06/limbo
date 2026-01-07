#include "limbo/ecs/World.hpp"
#include "limbo/ecs/Entity.hpp"
#include "limbo/ecs/Components.hpp"

namespace limbo {

Entity World::createEntity() {
    EntityId const id = m_registry.create();
    return Entity(id, this);
}

Entity World::createEntity(StringView name) {
    Entity entity = createEntity();
    entity.addComponent<NameComponent>(String(name));
    return entity;
}

void World::destroyEntity(EntityId entity) {
    if (isValid(entity)) {
        m_registry.destroy(entity);
    }
}

bool World::isValid(EntityId entity) const {
    return m_registry.valid(entity);
}

usize World::entityCount() const {
    return static_cast<usize>(m_registry.storage<EntityId>()->in_use());
}

void World::clear() {
    m_registry.clear();
}

}  // namespace limbo
