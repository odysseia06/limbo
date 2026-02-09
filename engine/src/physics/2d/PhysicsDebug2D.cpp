#include "limbo/physics/2d/PhysicsDebug2D.hpp"
#include "limbo/physics/2d/Physics2D.hpp"
#include "limbo/render/2d/Renderer2D.hpp"

#include <box2d/box2d.h>
#include <glm/gtc/constants.hpp>

namespace limbo {

void PhysicsDebug2D::draw(const Physics2D& physics) {
    if (!physics.isInitialized()) {
        return;
    }
    draw(physics.getWorld());
}

void PhysicsDebug2D::draw(const b2World* world) {
    if (!m_enabled || !world) {
        return;
    }

    // Draw physics debug behind sprites (negative z)
    constexpr f32 debugZ = -0.5f;

    // Iterate through all bodies in the world
    for (const b2Body* body = world->GetBodyList(); body != nullptr; body = body->GetNext()) {
        b2BodyType const bodyType = body->GetType();

        // Check if we should draw this body type
        if (bodyType == b2_staticBody && !m_drawStaticBodies) {
            continue;
        }
        if (bodyType == b2_kinematicBody && !m_drawKinematicBodies) {
            continue;
        }
        if (bodyType == b2_dynamicBody && !m_drawDynamicBodies) {
            continue;
        }

        // Get body transform
        const b2Transform& xf = body->GetTransform();

        // Draw center of mass if enabled
        if (m_drawCenterOfMass) {
            b2Vec2 const worldCenter = body->GetWorldCenter();
            f32 const crossSize = 0.1f;
            Renderer2D::drawLine(glm::vec3{worldCenter.x - crossSize, worldCenter.y, debugZ},
                                 glm::vec3{worldCenter.x + crossSize, worldCenter.y, debugZ},
                                 m_centerOfMassColor);
            Renderer2D::drawLine(glm::vec3{worldCenter.x, worldCenter.y - crossSize, debugZ},
                                 glm::vec3{worldCenter.x, worldCenter.y + crossSize, debugZ},
                                 m_centerOfMassColor);
        }

        // Iterate through all fixtures on this body
        for (const b2Fixture* fixture = body->GetFixtureList(); fixture != nullptr;
             fixture = fixture->GetNext()) {
            bool const isSensor = fixture->IsSensor();

            // Check if we should draw sensors
            if (isSensor && !m_drawSensors) {
                continue;
            }

            // Determine color based on body type and sensor state
            glm::vec4 color;
            if (isSensor) {
                color = m_sensorColor;
            } else {
                switch (bodyType) {
                case b2_staticBody:
                    color = m_staticBodyColor;
                    break;
                case b2_kinematicBody:
                    color = m_kinematicBodyColor;
                    break;
                case b2_dynamicBody:
                    color = m_dynamicBodyColor;
                    break;
                default:
                    color = m_dynamicBodyColor;
                    break;
                }
            }

            // Draw the shape
            const b2Shape* shape = fixture->GetShape();
            switch (shape->GetType()) {
            case b2Shape::e_circle: {
                const auto* circle = static_cast<const b2CircleShape*>(shape);
                b2Vec2 const center = b2Mul(xf, circle->m_p);
                f32 const radius = circle->m_radius;

                Renderer2D::drawCircle(glm::vec3{center.x, center.y, debugZ}, radius, color);

                // Draw a line to show rotation
                b2Vec2 const axis = b2Mul(xf.q, b2Vec2(1.0f, 0.0f));
                Renderer2D::drawLine(
                    glm::vec3{center.x, center.y, debugZ},
                    glm::vec3{center.x + radius * axis.x, center.y + radius * axis.y, debugZ},
                    color);
                break;
            }
            case b2Shape::e_polygon: {
                const auto* poly = static_cast<const b2PolygonShape*>(shape);
                i32 const vertexCount = poly->m_count;

                // Draw polygon edges
                for (i32 i = 0; i < vertexCount; ++i) {
                    b2Vec2 const v1 = b2Mul(xf, poly->m_vertices[i]);
                    b2Vec2 const v2 = b2Mul(xf, poly->m_vertices[(i + 1) % vertexCount]);
                    Renderer2D::drawLine(glm::vec3{v1.x, v1.y, debugZ},
                                         glm::vec3{v2.x, v2.y, debugZ}, color);
                }
                break;
            }
            case b2Shape::e_edge: {
                const auto* edge = static_cast<const b2EdgeShape*>(shape);
                b2Vec2 const v1 = b2Mul(xf, edge->m_vertex1);
                b2Vec2 const v2 = b2Mul(xf, edge->m_vertex2);
                Renderer2D::drawLine(glm::vec3{v1.x, v1.y, debugZ}, glm::vec3{v2.x, v2.y, debugZ},
                                     color);
                break;
            }
            case b2Shape::e_chain: {
                const auto* chain = static_cast<const b2ChainShape*>(shape);
                i32 const count = chain->m_count;
                for (i32 i = 0; i < count - 1; ++i) {
                    b2Vec2 const v1 = b2Mul(xf, chain->m_vertices[i]);
                    b2Vec2 const v2 = b2Mul(xf, chain->m_vertices[i + 1]);
                    Renderer2D::drawLine(glm::vec3{v1.x, v1.y, debugZ},
                                         glm::vec3{v2.x, v2.y, debugZ}, color);
                }
                break;
            }
            default:
                break;
            }

            // Draw AABB if enabled
            if (m_drawAABBs) {
                b2AABB aabb;
                fixture->GetShape()->ComputeAABB(&aabb, xf, 0);
                glm::vec2 const min{aabb.lowerBound.x, aabb.lowerBound.y};
                glm::vec2 const max{aabb.upperBound.x, aabb.upperBound.y};
                glm::vec2 const center = (min + max) * 0.5f;
                glm::vec2 const size = max - min;
                Renderer2D::drawRect(glm::vec3{center, debugZ}, size, 0.0f, m_aabbColor);
            }
        }
    }
}

}  // namespace limbo
