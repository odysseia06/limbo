#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limbo/physics/2d/Physics2D.hpp>

using namespace limbo;

TEST_CASE("Physics2D initialization", "[physics]") {
    Physics2D physics;

    SECTION("default state before init") {
        REQUIRE(physics.getWorld() == nullptr);
    }

    SECTION("initialization creates world") {
        physics.init({0.0f, -9.81f});
        REQUIRE(physics.getWorld() != nullptr);
    }

    SECTION("shutdown destroys world") {
        physics.init({0.0f, -9.81f});
        physics.shutdown();
        REQUIRE(physics.getWorld() == nullptr);
    }

    physics.shutdown();
}

TEST_CASE("Physics2D gravity", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    SECTION("initial gravity is correct") {
        glm::vec2 gravity = physics.getGravity();
        REQUIRE_THAT(gravity.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(gravity.y, Catch::Matchers::WithinAbs(-9.81f, 0.001f));
    }

    SECTION("can change gravity") {
        physics.setGravity({0.0f, -20.0f});
        glm::vec2 gravity = physics.getGravity();
        REQUIRE_THAT(gravity.y, Catch::Matchers::WithinAbs(-20.0f, 0.001f));
    }

    physics.shutdown();
}

TEST_CASE("Physics2D raycast", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});  // No gravity for deterministic tests

    // Create a simple box body for testing
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(5.0f, 0.0f);  // At x=5
    b2Body* body = physics.getWorld()->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(1.0f, 1.0f);  // 2x2 box

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &boxShape;
    body->CreateFixture(&fixtureDef);

    SECTION("raycast hits body") {
        // Cast from origin toward the box
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, false);

        REQUIRE(hit.hit);
        REQUIRE_THAT(hit.point.x, Catch::Matchers::WithinAbs(4.0f, 0.01f));  // Box edge at x=4
        REQUIRE_THAT(hit.distance, Catch::Matchers::WithinAbs(4.0f, 0.01f));
        REQUIRE(hit.body == body);
    }

    SECTION("raycast misses when direction wrong") {
        // Cast in opposite direction
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {-1.0f, 0.0f}, 10.0f, false);
        REQUIRE_FALSE(hit.hit);
    }

    SECTION("raycast misses when too short") {
        // Cast toward box but stop before it
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 2.0f, false);
        REQUIRE_FALSE(hit.hit);
    }

    SECTION("raycast normal points toward origin") {
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, false);
        REQUIRE(hit.hit);
        // Normal should point left (toward the raycast origin)
        REQUIRE_THAT(hit.normal.x, Catch::Matchers::WithinAbs(-1.0f, 0.01f));
        REQUIRE_THAT(hit.normal.y, Catch::Matchers::WithinAbs(0.0f, 0.01f));
    }

    physics.shutdown();
}

TEST_CASE("Physics2D raycastAll", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});

    // Create two bodies in a line
    b2BodyDef bodyDef1;
    bodyDef1.type = b2_staticBody;
    bodyDef1.position.Set(3.0f, 0.0f);
    b2Body* body1 = physics.getWorld()->CreateBody(&bodyDef1);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(0.5f, 0.5f);
    body1->CreateFixture(&boxShape, 0.0f);

    b2BodyDef bodyDef2;
    bodyDef2.type = b2_staticBody;
    bodyDef2.position.Set(6.0f, 0.0f);
    b2Body* body2 = physics.getWorld()->CreateBody(&bodyDef2);
    body2->CreateFixture(&boxShape, 0.0f);

    SECTION("raycastAll returns all hits sorted by distance") {
        auto hits = physics.raycastAll({0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, false);

        REQUIRE(hits.size() == 2);
        REQUIRE(hits[0].distance < hits[1].distance);  // Sorted by distance
        REQUIRE_THAT(hits[0].point.x, Catch::Matchers::WithinAbs(2.5f, 0.01f));  // First box edge
        REQUIRE_THAT(hits[1].point.x, Catch::Matchers::WithinAbs(5.5f, 0.01f));  // Second box edge
    }

    SECTION("raycastAll returns empty when no hits") {
        auto hits = physics.raycastAll({0.0f, 0.0f}, {-1.0f, 0.0f}, 10.0f, false);
        REQUIRE(hits.empty());
    }

    physics.shutdown();
}

TEST_CASE("Physics2D overlapCircle", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});

    // Create a body at origin
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(0.0f, 0.0f);
    b2Body* body = physics.getWorld()->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(1.0f, 1.0f);
    body->CreateFixture(&boxShape, 0.0f);

    SECTION("overlapCircle finds overlapping body") {
        auto bodies = physics.overlapCircle({0.0f, 0.0f}, 0.5f, false);
        REQUIRE(bodies.size() == 1);
        REQUIRE(bodies[0] == body);
    }

    SECTION("overlapCircle finds nothing when no overlap") {
        auto bodies = physics.overlapCircle({10.0f, 10.0f}, 0.5f, false);
        REQUIRE(bodies.empty());
    }

    SECTION("overlapCircle works at edge") {
        // Circle touching the box edge
        auto bodies = physics.overlapCircle({1.5f, 0.0f}, 0.6f, false);
        REQUIRE(bodies.size() == 1);
    }

    physics.shutdown();
}

TEST_CASE("Physics2D overlapBox", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});

    // Create a body at origin
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(0.0f, 0.0f);
    b2Body* body = physics.getWorld()->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(1.0f, 1.0f);
    body->CreateFixture(&boxShape, 0.0f);

    SECTION("overlapBox finds overlapping body") {
        auto bodies = physics.overlapBox({0.0f, 0.0f}, {0.5f, 0.5f}, false);
        REQUIRE(bodies.size() == 1);
        REQUIRE(bodies[0] == body);
    }

    SECTION("overlapBox finds nothing when no overlap") {
        auto bodies = physics.overlapBox({10.0f, 10.0f}, {0.5f, 0.5f}, false);
        REQUIRE(bodies.empty());
    }

    physics.shutdown();
}

TEST_CASE("Physics2D trigger filtering", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, 0.0f});

    // Create a trigger (sensor) body
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(5.0f, 0.0f);
    b2Body* body = physics.getWorld()->CreateBody(&bodyDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(1.0f, 1.0f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &boxShape;
    fixtureDef.isSensor = true;  // This is a trigger
    body->CreateFixture(&fixtureDef);

    SECTION("raycast excludes triggers by default") {
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, false);
        REQUIRE_FALSE(hit.hit);
    }

    SECTION("raycast includes triggers when requested") {
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, true);
        REQUIRE(hit.hit);
    }

    SECTION("overlapCircle excludes triggers by default") {
        auto bodies = physics.overlapCircle({5.0f, 0.0f}, 0.5f, false);
        REQUIRE(bodies.empty());
    }

    SECTION("overlapCircle includes triggers when requested") {
        auto bodies = physics.overlapCircle({5.0f, 0.0f}, 0.5f, true);
        REQUIRE(bodies.size() == 1);
    }

    physics.shutdown();
}

TEST_CASE("Physics2D edge cases", "[physics]") {
    Physics2D physics;
    physics.init({0.0f, -9.81f});

    SECTION("raycast with zero distance returns no hit") {
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {1.0f, 0.0f}, 0.0f, false);
        REQUIRE_FALSE(hit.hit);
    }

    SECTION("raycast with zero direction returns no hit") {
        RaycastHit2D hit = physics.raycast({0.0f, 0.0f}, {0.0f, 0.0f}, 10.0f, false);
        REQUIRE_FALSE(hit.hit);
    }

    SECTION("overlapCircle with zero radius returns empty") {
        auto bodies = physics.overlapCircle({0.0f, 0.0f}, 0.0f, false);
        REQUIRE(bodies.empty());
    }

    SECTION("overlapBox with zero size returns empty") {
        auto bodies = physics.overlapBox({0.0f, 0.0f}, {0.0f, 0.0f}, false);
        REQUIRE(bodies.empty());
    }

    physics.shutdown();
}
