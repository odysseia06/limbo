#include "limbo/physics/2d/Physics2D.hpp"

#include "limbo/debug/Log.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace limbo {

// ============================================================================
// Raycast Callback - First Hit Only
// ============================================================================

class RaycastFirstCallback : public b2RayCastCallback {
public:
    RaycastFirstCallback(bool includeTriggers) : m_includeTriggers(includeTriggers) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal,
                        float fraction) override {
        // Skip sensors if not including triggers
        if (!m_includeTriggers && fixture->IsSensor()) {
            return -1.0f;  // Continue, ignore this fixture
        }

        m_hit.hit = true;
        m_hit.point = {point.x, point.y};
        m_hit.normal = {normal.x, normal.y};
        m_hit.fraction = fraction;
        m_hit.body = fixture->GetBody();
        m_hit.fixture = fixture;

        return fraction;  // Clip ray to this hit
    }

    RaycastHit2D m_hit;

private:
    bool m_includeTriggers;
};

// ============================================================================
// Raycast Callback - All Hits
// ============================================================================

class RaycastAllCallback : public b2RayCastCallback {
public:
    RaycastAllCallback(bool includeTriggers) : m_includeTriggers(includeTriggers) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal,
                        float fraction) override {
        // Skip sensors if not including triggers
        if (!m_includeTriggers && fixture->IsSensor()) {
            return -1.0f;  // Continue, ignore this fixture
        }

        RaycastHit2D hit;
        hit.hit = true;
        hit.point = {point.x, point.y};
        hit.normal = {normal.x, normal.y};
        hit.fraction = fraction;
        hit.body = fixture->GetBody();
        hit.fixture = fixture;
        m_hits.push_back(hit);

        return 1.0f;  // Continue to find all hits
    }

    std::vector<RaycastHit2D> m_hits;

private:
    bool m_includeTriggers;
};

// ============================================================================
// AABB Query Callback - Collects candidates for narrow-phase
// ============================================================================

class AABBQueryCallback : public b2QueryCallback {
public:
    AABBQueryCallback(bool includeTriggers) : m_includeTriggers(includeTriggers) {}

    bool ReportFixture(b2Fixture* fixture) override {
        // Skip sensors if not including triggers
        if (!m_includeTriggers && fixture->IsSensor()) {
            return true;  // Continue query
        }

        m_fixtures.push_back(fixture);
        return true;  // Continue to find all
    }

    std::vector<b2Fixture*> m_fixtures;

private:
    bool m_includeTriggers;
};

Physics2D::~Physics2D() {
    shutdown();
}

void Physics2D::init(const glm::vec2& gravity) {
    if (m_world) {
        LIMBO_LOG_PHYSICS_WARN("Physics2D already initialized");
        return;
    }

    b2Vec2 const b2Gravity(gravity.x, gravity.y);
    m_world = new b2World(b2Gravity);

    LIMBO_LOG_PHYSICS_INFO("Physics2D initialized (gravity: {}, {})", gravity.x, gravity.y);
}

void Physics2D::shutdown() {
    if (m_world) {
        delete m_world;
        m_world = nullptr;
        LIMBO_LOG_PHYSICS_INFO("Physics2D shutdown");
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
        b2Vec2 const g = m_world->GetGravity();
        return {g.x, g.y};
    }
    return {0.0f, 0.0f};
}

// ============================================================================
// Physics Query Implementations
// ============================================================================

RaycastHit2D Physics2D::raycast(const glm::vec2& origin, const glm::vec2& direction,
                                f32 maxDistance, bool includeTriggers) const {
    RaycastHit2D result;
    if (!m_world || maxDistance <= 0.0f) {
        return result;
    }

    // Normalize direction
    f32 const length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.0001f) {
        return result;
    }
    glm::vec2 const dir = direction / length;

    b2Vec2 const p1(origin.x, origin.y);
    b2Vec2 const p2(origin.x + dir.x * maxDistance, origin.y + dir.y * maxDistance);

    RaycastFirstCallback callback(includeTriggers);
    m_world->RayCast(&callback, p1, p2);

    if (callback.m_hit.hit) {
        result = callback.m_hit;
        result.distance = result.fraction * maxDistance;
    }

    return result;
}

std::vector<RaycastHit2D> Physics2D::raycastAll(const glm::vec2& origin, const glm::vec2& direction,
                                                f32 maxDistance, bool includeTriggers) const {
    std::vector<RaycastHit2D> results;
    if (!m_world || maxDistance <= 0.0f) {
        return results;
    }

    // Normalize direction
    f32 const length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.0001f) {
        return results;
    }
    glm::vec2 const dir = direction / length;

    b2Vec2 const p1(origin.x, origin.y);
    b2Vec2 const p2(origin.x + dir.x * maxDistance, origin.y + dir.y * maxDistance);

    RaycastAllCallback callback(includeTriggers);
    m_world->RayCast(&callback, p1, p2);

    // Compute distances and sort by distance
    for (auto& hit : callback.m_hits) {
        hit.distance = hit.fraction * maxDistance;
    }

    results = std::move(callback.m_hits);
    std::sort(results.begin(), results.end(),
              [](const RaycastHit2D& a, const RaycastHit2D& b) { return a.distance < b.distance; });

    return results;
}

std::vector<b2Body*> Physics2D::overlapCircle(const glm::vec2& center, f32 radius,
                                              bool includeTriggers) const {
    std::vector<b2Body*> results;
    if (!m_world || radius <= 0.0f) {
        return results;
    }

    // Broad-phase: Query AABB containing the circle
    b2AABB aabb;
    aabb.lowerBound = b2Vec2(center.x - radius, center.y - radius);
    aabb.upperBound = b2Vec2(center.x + radius, center.y + radius);

    AABBQueryCallback callback(includeTriggers);
    m_world->QueryAABB(&callback, aabb);

    // Narrow-phase: Test each fixture against the circle shape
    b2CircleShape queryCircle;
    queryCircle.m_radius = radius;
    queryCircle.m_p = b2Vec2(center.x, center.y);

    b2Transform queryTransform;
    queryTransform.SetIdentity();

    std::unordered_set<b2Body*> uniqueBodies;

    for (b2Fixture* fixture : callback.m_fixtures) {
        b2Body* body = fixture->GetBody();

        // Get fixture's transform
        b2Transform const fixtureTransform = body->GetTransform();

        // Narrow-phase test
        if (b2TestOverlap(&queryCircle, 0, fixture->GetShape(), 0, queryTransform,
                          fixtureTransform)) {
            if (uniqueBodies.find(body) == uniqueBodies.end()) {
                uniqueBodies.insert(body);
                results.push_back(body);
            }
        }
    }

    return results;
}

std::vector<b2Body*> Physics2D::overlapBox(const glm::vec2& center, const glm::vec2& halfExtents,
                                           bool includeTriggers) const {
    std::vector<b2Body*> results;
    if (!m_world || halfExtents.x <= 0.0f || halfExtents.y <= 0.0f) {
        return results;
    }

    // Broad-phase: Query AABB
    b2AABB aabb;
    aabb.lowerBound = b2Vec2(center.x - halfExtents.x, center.y - halfExtents.y);
    aabb.upperBound = b2Vec2(center.x + halfExtents.x, center.y + halfExtents.y);

    AABBQueryCallback callback(includeTriggers);
    m_world->QueryAABB(&callback, aabb);

    // Narrow-phase: Test each fixture against the box shape
    b2PolygonShape queryBox;
    queryBox.SetAsBox(halfExtents.x, halfExtents.y, b2Vec2(center.x, center.y), 0.0f);

    b2Transform queryTransform;
    queryTransform.SetIdentity();

    std::unordered_set<b2Body*> uniqueBodies;

    for (b2Fixture* fixture : callback.m_fixtures) {
        b2Body* body = fixture->GetBody();

        // Get fixture's transform
        b2Transform const fixtureTransform = body->GetTransform();

        // Narrow-phase test
        if (b2TestOverlap(&queryBox, 0, fixture->GetShape(), 0, queryTransform, fixtureTransform)) {
            if (uniqueBodies.find(body) == uniqueBodies.end()) {
                uniqueBodies.insert(body);
                results.push_back(body);
            }
        }
    }

    return results;
}

}  // namespace limbo
