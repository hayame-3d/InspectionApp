#include "Camera.h"
#include <cmath>

namespace InspectionApp {

    Camera::Camera(float left, float right, float bottom, float top, float zNear, float zFar)
        : m_left(left), m_right(right), m_bottom(bottom), m_top(top), m_zNear(zNear), m_zFar(zFar) {
        RecalculateProjection();
    }

    void Camera::RecalculateProjection() {
        float zl = m_left / m_zoom, zr = m_right / m_zoom;
        float zb = m_bottom / m_zoom, zt = m_top / m_zoom;
        m_projection = glm::ortho(zl, zr, zb, zt, m_zNear, m_zFar);
    }

    void Camera::SetZoom(float zoom) {
        if (zoom <= 0.0f) zoom = 0.001f;
        m_zoom = zoom;
        RecalculateProjection();
    }

    void Camera::SetAspectRatio(float aspect) {
        float height = m_top - m_bottom;
        float width = height * aspect;
        m_left = -width * 0.5f;
        m_right = width * 0.5f;
        RecalculateProjection();
    }

    void Camera::SetOffset(float x, float y, float z) {
        if (!m_lockOffsetX) m_offset.x = x;
        if (!m_lockOffsetY) m_offset.y = y;
        if (!m_lockOffsetZ) m_offset.z = z;
    }

    void Camera::AddOffset(float dx, float dy, float dz) {
        if (!m_lockOffsetX) m_offset.x += dx;
        if (!m_lockOffsetY) m_offset.y += dy;
        if (!m_lockOffsetZ) m_offset.z += dz;
    }

    void Camera::SetRotationZ(float degrees) {
        if (!m_lockRotation) m_rotZ = degrees;
    }

    void Camera::SetYaw(float yaw) {
        m_yaw = yaw;
    }

    void Camera::SetPitch(float pitch) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        m_pitch = pitch;
    }

    void Camera::AddYaw(float dyaw) {
        m_yaw += dyaw;
    }

    void Camera::AddPitch(float dpitch) {
        m_pitch += dpitch;
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
    }

    glm::vec3 Camera::GetEyeDirection() const {
        float yawRad = glm::radians(m_yaw);
        float pitchRad = glm::radians(m_pitch);
        return glm::vec3(
            std::cos(pitchRad) * std::cos(yawRad),
            std::cos(pitchRad) * std::sin(yawRad),
            std::sin(pitchRad)
        );
    }

    void Camera::SetCenter(float x, float y, float z) { m_center = glm::vec3(x, y, z); }

    void Camera::SetView(OrthoView view) {
        m_currentView = view;
        switch (view) {
        case OrthoView::Top:    m_yaw = 0.0f;   m_pitch = 90.0f;  break;
        case OrthoView::Bottom: m_yaw = 0.0f;   m_pitch = -90.0f; break;
        case OrthoView::Front:  m_yaw = -90.0f; m_pitch = 0.0f;   break;
        case OrthoView::Back:   m_yaw = 90.0f;  m_pitch = 0.0f;   break;
        case OrthoView::Left:   m_yaw = 180.0f; m_pitch = 0.0f;   break;
        case OrthoView::Right:  m_yaw = 0.0f;   m_pitch = 0.0f;   break;
        }
    }

    glm::vec3 Camera::GetUpVector() const {
        glm::vec3 eye = GetEyeDirection();
        glm::vec3 tempUp = (std::abs(eye.z) < 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::vec3 forward = -glm::normalize(eye);
        glm::vec3 right = glm::normalize(glm::cross(tempUp, forward));
        return glm::normalize(glm::cross(forward, right));
    }

    glm::vec3 Camera::GetRightVector() const {
        glm::vec3 eye = GetEyeDirection();
        glm::vec3 tempUp = (std::abs(eye.z) < 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::vec3 forward = -glm::normalize(eye);
        return glm::normalize(glm::cross(tempUp, forward));
    }

    glm::mat4 Camera::GetViewMatrix() const {
        glm::vec3 eye = GetEyeDirection();
        glm::vec3 tempUp = (std::abs(eye.z) < 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::mat4 view = glm::lookAt(m_center + eye + m_offset, m_center + m_offset, tempUp);
        if (m_rotZ != 0.0f) view = glm::rotate(view, glm::radians(-m_rotZ), glm::vec3(0, 0, 1));
        return view;
    }

}