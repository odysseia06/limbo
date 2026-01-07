#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limbo/ecs/Components.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("TransformComponent default construction", "[math][transform]") {
    limbo::TransformComponent transform;

    REQUIRE(transform.position == glm::vec3(0.0f));
    REQUIRE(transform.rotation == glm::vec3(0.0f));
    REQUIRE(transform.scale == glm::vec3(1.0f));
}

TEST_CASE("TransformComponent position-only construction", "[math][transform]") {
    limbo::TransformComponent transform(glm::vec3(1.0f, 2.0f, 3.0f));

    REQUIRE(transform.position == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(transform.rotation == glm::vec3(0.0f));
    REQUIRE(transform.scale == glm::vec3(1.0f));
}

TEST_CASE("TransformComponent full construction", "[math][transform]") {
    limbo::TransformComponent transform(
        glm::vec3(1.0f, 2.0f, 3.0f),
        glm::vec3(0.1f, 0.2f, 0.3f),
        glm::vec3(2.0f, 2.0f, 2.0f));

    REQUIRE(transform.position == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(transform.rotation == glm::vec3(0.1f, 0.2f, 0.3f));
    REQUIRE(transform.scale == glm::vec3(2.0f, 2.0f, 2.0f));
}

TEST_CASE("TransformComponent getMatrix identity", "[math][transform]") {
    limbo::TransformComponent transform;
    glm::mat4 matrix = transform.getMatrix();

    // Should be identity matrix
    glm::mat4 identity(1.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            REQUIRE_THAT(matrix[i][j], WithinAbs(identity[i][j], 0.0001));
        }
    }
}

TEST_CASE("TransformComponent getMatrix translation", "[math][transform]") {
    limbo::TransformComponent transform(glm::vec3(5.0f, 10.0f, 15.0f));
    glm::mat4 matrix = transform.getMatrix();

    // Translation should be in the last column
    REQUIRE_THAT(matrix[3][0], WithinAbs(5.0f, 0.0001));
    REQUIRE_THAT(matrix[3][1], WithinAbs(10.0f, 0.0001));
    REQUIRE_THAT(matrix[3][2], WithinAbs(15.0f, 0.0001));
    REQUIRE_THAT(matrix[3][3], WithinAbs(1.0f, 0.0001));
}

TEST_CASE("TransformComponent getMatrix scale", "[math][transform]") {
    limbo::TransformComponent transform;
    transform.scale = glm::vec3(2.0f, 3.0f, 4.0f);
    glm::mat4 matrix = transform.getMatrix();

    // Scale should affect diagonal elements
    REQUIRE_THAT(matrix[0][0], WithinAbs(2.0f, 0.0001));
    REQUIRE_THAT(matrix[1][1], WithinAbs(3.0f, 0.0001));
    REQUIRE_THAT(matrix[2][2], WithinAbs(4.0f, 0.0001));
}

TEST_CASE("TransformComponent transforms point correctly", "[math][transform]") {
    limbo::TransformComponent transform;
    transform.position = glm::vec3(10.0f, 0.0f, 0.0f);
    transform.scale = glm::vec3(2.0f, 2.0f, 2.0f);

    glm::mat4 matrix = transform.getMatrix();
    glm::vec4 point(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 result = matrix * point;

    // Point at (1,0,0) scaled by 2 and translated by 10 should be at (12,0,0)
    REQUIRE_THAT(result.x, WithinAbs(12.0f, 0.0001));
    REQUIRE_THAT(result.y, WithinAbs(0.0f, 0.0001));
    REQUIRE_THAT(result.z, WithinAbs(0.0f, 0.0001));
}

TEST_CASE("CameraComponent perspective projection", "[math][camera]") {
    limbo::CameraComponent camera;
    camera.projectionType = limbo::CameraComponent::ProjectionType::Perspective;
    camera.fov = glm::radians(90.0f);
    camera.nearClip = 0.1f;
    camera.farClip = 100.0f;

    glm::mat4 proj = camera.getProjectionMatrix(1.0f);

    // For 90 degree FOV and aspect 1:1, the projection should have specific properties
    // The [0][0] and [1][1] elements should be equal to cot(fov/2) = 1.0
    REQUIRE_THAT(proj[0][0], WithinAbs(1.0f, 0.0001));
    REQUIRE_THAT(proj[1][1], WithinAbs(1.0f, 0.0001));
}

TEST_CASE("CameraComponent orthographic projection", "[math][camera]") {
    limbo::CameraComponent camera;
    camera.projectionType = limbo::CameraComponent::ProjectionType::Orthographic;
    camera.orthoSize = 5.0f;
    camera.nearClip = -1.0f;
    camera.farClip = 1.0f;

    glm::mat4 proj = camera.getProjectionMatrix(1.0f);

    // For orthographic with size 5 and aspect 1:1
    // left=-5, right=5, bottom=-5, top=5
    // [0][0] = 2/(right-left) = 2/10 = 0.2
    // [1][1] = 2/(top-bottom) = 2/10 = 0.2
    REQUIRE_THAT(proj[0][0], WithinAbs(0.2f, 0.0001));
    REQUIRE_THAT(proj[1][1], WithinAbs(0.2f, 0.0001));
}
