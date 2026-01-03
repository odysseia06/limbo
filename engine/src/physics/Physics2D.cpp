#include "limbo/physics/Physics2D.hpp"

#include <spdlog/spdlog.h>

namespace limbo {

Physics2D::~Physics2D() {
    shutdown();
}

void Physics2D::init(const glm::vec2& gravity) {
    if (m_world) {
        spdlog::warn("Physics2D already initialized");
        return;
    }

    b2Vec2 b2Gravity(gravity.x, gravity.y);
    m_world = new b2World(b2Gravity);

    spdlog::info("Physics2D initialized (gravity: {}, {})", gravity.x, gravity.y);
}

void Physics2D::shutdown() {
    if (m_world) {
        delete m_world;
        m_world = nullptr;
        spdlog::info("Physics2D shutdown");
    }
}

void Physics2D::step(f32 deltaTime) {
    if (!m_world) {
        return;
    }

    m_world->Step(deltaTime, m_velocityIterations, m_positionIterations);
}

void Physics2D::setGravity(const glm::vec2& gravity) {
    if (m_world) {
        m_world->SetGravity(b2Vec2(gravity.x, gravity.y));
    }
}

glm::vec2 Physics2D::getGravity() const {
    if (m_world) {
        b2Vec2 g = m_world->GetGravity();
        return {g.x, g.y};
    }
    return {0.0f, 0.0f};
}

}  // namespace limbo
