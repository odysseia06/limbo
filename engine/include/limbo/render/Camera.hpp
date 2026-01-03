#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <glm/glm.hpp>

namespace limbo {

// Orthographic camera for 2D rendering
class LIMBO_API OrthographicCamera {
public:
    OrthographicCamera() = default;
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top);

    void setProjection(f32 left, f32 right, f32 bottom, f32 top);

    void setPosition(const glm::vec3& position);
    void setRotation(f32 rotation); // Z-axis rotation in radians

    [[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
    [[nodiscard]] f32 getRotation() const { return m_rotation; }

    [[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
    [[nodiscard]] const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    [[nodiscard]] const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

    [[nodiscard]] f32 getLeft() const { return m_left; }
    [[nodiscard]] f32 getRight() const { return m_right; }
    [[nodiscard]] f32 getBottom() const { return m_bottom; }
    [[nodiscard]] f32 getTop() const { return m_top; }

private:
    void recalculateViewMatrix();

    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_viewProjectionMatrix{1.0f};

    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    f32 m_rotation = 0.0f;
    
    f32 m_left = -1.0f;
    f32 m_right = 1.0f;
    f32 m_bottom = -1.0f;
    f32 m_top = 1.0f;
};

// Perspective camera for 3D rendering
class LIMBO_API PerspectiveCamera {
public:
    PerspectiveCamera() = default;
    PerspectiveCamera(f32 fovY, f32 aspectRatio, f32 nearClip, f32 farClip);

    void setProjection(f32 fovY, f32 aspectRatio, f32 nearClip, f32 farClip);

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation); // Euler angles (pitch, yaw, roll) in radians

    // Look at a target point
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    [[nodiscard]] const glm::vec3& getPosition() const { return m_position; }
    [[nodiscard]] const glm::vec3& getRotation() const { return m_rotation; }

    [[nodiscard]] glm::vec3 getForward() const;
    [[nodiscard]] glm::vec3 getRight() const;
    [[nodiscard]] glm::vec3 getUp() const;

    [[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
    [[nodiscard]] const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
    [[nodiscard]] const glm::mat4& getViewProjectionMatrix() const { return m_viewProjectionMatrix; }

    [[nodiscard]] f32 getFovY() const { return m_fovY; }
    [[nodiscard]] f32 getAspectRatio() const { return m_aspectRatio; }
    [[nodiscard]] f32 getNearClip() const { return m_nearClip; }
    [[nodiscard]] f32 getFarClip() const { return m_farClip; }

private:
    void recalculateViewMatrix();

    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_viewProjectionMatrix{1.0f};

    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    glm::vec3 m_rotation{0.0f, 0.0f, 0.0f}; // pitch, yaw, roll

    f32 m_fovY = glm::radians(45.0f);
    f32 m_aspectRatio = 16.0f / 9.0f;
    f32 m_nearClip = 0.1f;
    f32 m_farClip = 1000.0f;
};

} // namespace limbo
