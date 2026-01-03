#include "limbo/render/common/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace limbo {

// ============================================================================
// OrthographicCamera
// ============================================================================

OrthographicCamera::OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top) {
    setProjection(left, right, bottom, top);
}

void OrthographicCamera::setProjection(f32 left, f32 right, f32 bottom, f32 top) {
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

void OrthographicCamera::setPosition(const glm::vec3& position) {
    m_position = position;
    recalculateViewMatrix();
}

void OrthographicCamera::setRotation(f32 rotation) {
    m_rotation = rotation;
    recalculateViewMatrix();
}

void OrthographicCamera::recalculateViewMatrix() {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
                          glm::rotate(glm::mat4(1.0f), m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    m_viewMatrix = glm::inverse(transform);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

// ============================================================================
// PerspectiveCamera
// ============================================================================

PerspectiveCamera::PerspectiveCamera(f32 fovY, f32 aspectRatio, f32 nearClip, f32 farClip) {
    setProjection(fovY, aspectRatio, nearClip, farClip);
}

void PerspectiveCamera::setProjection(f32 fovY, f32 aspectRatio, f32 nearClip, f32 farClip) {
    m_fovY = fovY;
    m_aspectRatio = aspectRatio;
    m_nearClip = nearClip;
    m_farClip = farClip;

    m_projectionMatrix = glm::perspective(fovY, aspectRatio, nearClip, farClip);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

void PerspectiveCamera::setPosition(const glm::vec3& position) {
    m_position = position;
    recalculateViewMatrix();
}

void PerspectiveCamera::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    recalculateViewMatrix();
}

void PerspectiveCamera::lookAt(const glm::vec3& target, const glm::vec3& up) {
    m_viewMatrix = glm::lookAt(m_position, target, up);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

    // Extract rotation from view matrix (optional, for getRotation() consistency)
    glm::vec3 direction = glm::normalize(target - m_position);
    m_rotation.x = glm::asin(-direction.y);              // pitch
    m_rotation.y = glm::atan(direction.x, direction.z);  // yaw
    m_rotation.z = 0.0f;                                 // roll (not extracted from lookAt)
}

glm::vec3 PerspectiveCamera::getForward() const {
    f32 pitch = m_rotation.x;
    f32 yaw = m_rotation.y;

    glm::vec3 forward;
    forward.x = glm::cos(pitch) * glm::sin(yaw);
    forward.y = -glm::sin(pitch);
    forward.z = glm::cos(pitch) * glm::cos(yaw);
    return glm::normalize(forward);
}

glm::vec3 PerspectiveCamera::getRight() const {
    return glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 PerspectiveCamera::getUp() const {
    return glm::normalize(glm::cross(getRight(), getForward()));
}

void PerspectiveCamera::recalculateViewMatrix() {
    // Build rotation matrix from Euler angles (pitch, yaw, roll)
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));  // roll
    rotation = glm::rotate(rotation, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));  // yaw
    rotation = glm::rotate(rotation, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));  // pitch

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_position);

    glm::mat4 transform = translation * rotation;
    m_viewMatrix = glm::inverse(transform);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

}  // namespace limbo
