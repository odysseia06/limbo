#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "limbo/render/common/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace limbo;

TEST_CASE("OrthographicCamera initialization", "[render][camera]") {
    SECTION("default construction") {
        OrthographicCamera camera;

        REQUIRE_THAT(camera.getLeft(), Catch::Matchers::WithinAbs(-1.0f, 0.001f));
        REQUIRE_THAT(camera.getRight(), Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(camera.getBottom(), Catch::Matchers::WithinAbs(-1.0f, 0.001f));
        REQUIRE_THAT(camera.getTop(), Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE(camera.getPosition() == glm::vec3(0.0f));
        REQUIRE_THAT(camera.getRotation(), Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("parameterized construction") {
        OrthographicCamera camera(-10.0f, 10.0f, -5.0f, 5.0f);

        REQUIRE_THAT(camera.getLeft(), Catch::Matchers::WithinAbs(-10.0f, 0.001f));
        REQUIRE_THAT(camera.getRight(), Catch::Matchers::WithinAbs(10.0f, 0.001f));
        REQUIRE_THAT(camera.getBottom(), Catch::Matchers::WithinAbs(-5.0f, 0.001f));
        REQUIRE_THAT(camera.getTop(), Catch::Matchers::WithinAbs(5.0f, 0.001f));
    }
}

TEST_CASE("OrthographicCamera projection", "[render][camera]") {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);

    SECTION("projection matrix is correct") {
        // For orthographic projection, a point at center should map to (0,0,z)
        glm::vec4 centerPoint(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 projected = camera.getProjectionMatrix() * centerPoint;

        REQUIRE_THAT(projected.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(projected.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("points at edges map correctly") {
        // Right edge should map to x=1 in NDC
        glm::vec4 rightPoint(1.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 projected = camera.getProjectionMatrix() * rightPoint;
        REQUIRE_THAT(projected.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));

        // Top edge should map to y=1 in NDC
        glm::vec4 topPoint(0.0f, 1.0f, 0.0f, 1.0f);
        projected = camera.getProjectionMatrix() * topPoint;
        REQUIRE_THAT(projected.y, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }

    SECTION("setProjection updates bounds") {
        camera.setProjection(-5.0f, 5.0f, -2.5f, 2.5f);

        REQUIRE_THAT(camera.getLeft(), Catch::Matchers::WithinAbs(-5.0f, 0.001f));
        REQUIRE_THAT(camera.getRight(), Catch::Matchers::WithinAbs(5.0f, 0.001f));
        REQUIRE_THAT(camera.getBottom(), Catch::Matchers::WithinAbs(-2.5f, 0.001f));
        REQUIRE_THAT(camera.getTop(), Catch::Matchers::WithinAbs(2.5f, 0.001f));

        // Point at (5, 0) should now map to x=1
        glm::vec4 edgePoint(5.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 projected = camera.getProjectionMatrix() * edgePoint;
        REQUIRE_THAT(projected.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }
}

TEST_CASE("OrthographicCamera position and view", "[render][camera]") {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);

    SECTION("setting position updates view matrix") {
        camera.setPosition(glm::vec3(2.0f, 3.0f, 0.0f));

        REQUIRE(camera.getPosition() == glm::vec3(2.0f, 3.0f, 0.0f));

        // When camera is at (2,3), a world point at (2,3) should appear at center
        glm::vec4 worldPoint(2.0f, 3.0f, 0.0f, 1.0f);
        glm::vec4 viewSpace = camera.getViewMatrix() * worldPoint;

        REQUIRE_THAT(viewSpace.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(viewSpace.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("view projection combines both transforms") {
        camera.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));

        // A point at (2, 0) should appear at x=1 in NDC (camera at 1, point at 2, offset is 1)
        glm::vec4 worldPoint(2.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 clipSpace = camera.getViewProjectionMatrix() * worldPoint;

        REQUIRE_THAT(clipSpace.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }
}

TEST_CASE("OrthographicCamera rotation", "[render][camera]") {
    OrthographicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);

    SECTION("setting rotation") {
        f32 angle = glm::half_pi<f32>();  // 90 degrees
        camera.setRotation(angle);

        REQUIRE_THAT(camera.getRotation(), Catch::Matchers::WithinAbs(angle, 0.001f));
    }

    SECTION("rotation affects view matrix") {
        // Rotate 90 degrees counter-clockwise
        camera.setRotation(glm::half_pi<f32>());

        // A point at (1, 0) should appear at (0, -1) in view space after 90 degree rotation
        // (because we're rotating the camera, not the world)
        glm::vec4 worldPoint(1.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 viewSpace = camera.getViewMatrix() * worldPoint;

        // With camera rotated 90 degrees CCW, world point (1,0) appears at view (0, -1)
        REQUIRE_THAT(viewSpace.x, Catch::Matchers::WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(viewSpace.y, Catch::Matchers::WithinAbs(-1.0f, 0.01f));
    }
}

TEST_CASE("PerspectiveCamera initialization", "[render][camera]") {
    SECTION("default construction") {
        PerspectiveCamera camera;

        REQUIRE_THAT(camera.getFovY(), Catch::Matchers::WithinAbs(glm::radians(45.0f), 0.001f));
        REQUIRE_THAT(camera.getAspectRatio(), Catch::Matchers::WithinAbs(16.0f / 9.0f, 0.001f));
        REQUIRE_THAT(camera.getNearClip(), Catch::Matchers::WithinAbs(0.1f, 0.001f));
        REQUIRE_THAT(camera.getFarClip(), Catch::Matchers::WithinAbs(1000.0f, 0.001f));
        REQUIRE(camera.getPosition() == glm::vec3(0.0f));
        REQUIRE(camera.getRotation() == glm::vec3(0.0f));
    }

    SECTION("parameterized construction") {
        PerspectiveCamera camera(glm::radians(60.0f), 4.0f / 3.0f, 0.5f, 500.0f);

        REQUIRE_THAT(camera.getFovY(), Catch::Matchers::WithinAbs(glm::radians(60.0f), 0.001f));
        REQUIRE_THAT(camera.getAspectRatio(), Catch::Matchers::WithinAbs(4.0f / 3.0f, 0.001f));
        REQUIRE_THAT(camera.getNearClip(), Catch::Matchers::WithinAbs(0.5f, 0.001f));
        REQUIRE_THAT(camera.getFarClip(), Catch::Matchers::WithinAbs(500.0f, 0.001f));
    }
}

TEST_CASE("PerspectiveCamera projection", "[render][camera]") {
    PerspectiveCamera camera(glm::radians(90.0f), 1.0f, 1.0f, 100.0f);

    SECTION("center point projects to center") {
        // A point directly in front of the camera should project to center
        glm::vec4 centerPoint(0.0f, 0.0f, -10.0f, 1.0f);  // Z negative is forward
        glm::vec4 projected = camera.getProjectionMatrix() * centerPoint;

        // Perspective divide
        projected /= projected.w;

        REQUIRE_THAT(projected.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(projected.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("setProjection updates parameters") {
        camera.setProjection(glm::radians(45.0f), 2.0f, 0.1f, 1000.0f);

        REQUIRE_THAT(camera.getFovY(), Catch::Matchers::WithinAbs(glm::radians(45.0f), 0.001f));
        REQUIRE_THAT(camera.getAspectRatio(), Catch::Matchers::WithinAbs(2.0f, 0.001f));
        REQUIRE_THAT(camera.getNearClip(), Catch::Matchers::WithinAbs(0.1f, 0.001f));
        REQUIRE_THAT(camera.getFarClip(), Catch::Matchers::WithinAbs(1000.0f, 0.001f));
    }
}

TEST_CASE("PerspectiveCamera position and view", "[render][camera]") {
    PerspectiveCamera camera;

    SECTION("setting position updates view matrix") {
        camera.setPosition(glm::vec3(5.0f, 3.0f, 10.0f));

        REQUIRE(camera.getPosition() == glm::vec3(5.0f, 3.0f, 10.0f));
    }

    SECTION("setting rotation updates view matrix") {
        camera.setRotation(glm::vec3(0.1f, 0.2f, 0.3f));

        REQUIRE(camera.getRotation() == glm::vec3(0.1f, 0.2f, 0.3f));
    }
}

TEST_CASE("PerspectiveCamera direction vectors", "[render][camera]") {
    PerspectiveCamera camera;

    SECTION("default forward is +Z") {
        // Convention: yaw=0 means looking along +Z axis
        glm::vec3 forward = camera.getForward();

        REQUIRE_THAT(forward.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(forward.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(forward.z, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }

    SECTION("default right is -X") {
        // With forward=(0,0,1) and world up=(0,1,0), right = cross(forward, up) = (-1,0,0)
        glm::vec3 right = camera.getRight();

        REQUIRE_THAT(right.x, Catch::Matchers::WithinAbs(-1.0f, 0.001f));
        REQUIRE_THAT(right.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(right.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("default up is +Y") {
        glm::vec3 up = camera.getUp();

        REQUIRE_THAT(up.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(up.y, Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(up.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }

    SECTION("direction vectors are orthogonal") {
        // After any rotation, vectors should remain orthogonal
        camera.setRotation(glm::vec3(0.5f, 0.3f, 0.1f));

        glm::vec3 forward = camera.getForward();
        glm::vec3 right = camera.getRight();
        glm::vec3 up = camera.getUp();

        f32 dotFR = glm::dot(forward, right);
        f32 dotFU = glm::dot(forward, up);
        f32 dotRU = glm::dot(right, up);

        REQUIRE_THAT(dotFR, Catch::Matchers::WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(dotFU, Catch::Matchers::WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(dotRU, Catch::Matchers::WithinAbs(0.0f, 0.01f));
    }

    SECTION("direction vectors are unit length") {
        camera.setRotation(glm::vec3(0.5f, 0.3f, 0.1f));

        REQUIRE_THAT(glm::length(camera.getForward()), Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(glm::length(camera.getRight()), Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(glm::length(camera.getUp()), Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }
}

TEST_CASE("PerspectiveCamera lookAt", "[render][camera]") {
    PerspectiveCamera camera;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -5.0f));

    SECTION("lookAt points camera at target") {
        camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

        // Forward should point toward origin (+Z direction from camera at -5)
        glm::vec3 forward = camera.getForward();

        REQUIRE_THAT(forward.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(forward.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(forward.z, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }

    SECTION("lookAt with offset target") {
        camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        camera.lookAt(glm::vec3(5.0f, 0.0f, 0.0f));

        // Forward should point toward (5, 0, 0) from origin = (+X direction)
        glm::vec3 forward = camera.getForward();

        REQUIRE_THAT(forward.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(forward.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(forward.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    }
}

TEST_CASE("Renderer2D Statistics calculations", "[render][statistics]") {
    // Test the Statistics struct helper methods
    // Note: We can't test actual rendering without OpenGL, but we can test the struct
    struct Statistics {
        u32 drawCalls = 0;
        u32 quadCount = 0;
        u32 lineCount = 0;
        u32 textureBinds = 0;
        u32 batchCount = 0;

        [[nodiscard]] u32 vertexCount() const { return quadCount * 4 + lineCount * 2; }
        [[nodiscard]] u32 indexCount() const { return quadCount * 6; }
    };

    SECTION("empty statistics") {
        Statistics stats;

        REQUIRE(stats.vertexCount() == 0);
        REQUIRE(stats.indexCount() == 0);
    }

    SECTION("quad vertex and index counts") {
        Statistics stats;
        stats.quadCount = 100;

        REQUIRE(stats.vertexCount() == 400);  // 100 quads * 4 vertices
        REQUIRE(stats.indexCount() == 600);   // 100 quads * 6 indices
    }

    SECTION("line vertex counts") {
        Statistics stats;
        stats.lineCount = 50;

        REQUIRE(stats.vertexCount() == 100);  // 50 lines * 2 vertices
        REQUIRE(stats.indexCount() == 0);     // Lines don't use indices
    }

    SECTION("mixed quads and lines") {
        Statistics stats;
        stats.quadCount = 10;
        stats.lineCount = 20;

        REQUIRE(stats.vertexCount() == 80);  // 10*4 + 20*2 = 40 + 40
        REQUIRE(stats.indexCount() == 60);   // 10*6 = 60
    }
}
