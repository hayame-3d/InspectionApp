#pragma once
#include "Types.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace InspectionApp {

    enum class OrthoView { Top, Bottom, Front, Back, Left, Right };

    class Camera {
    public:
        Camera(float left, float right, float bottom, float top, float zNear, float zFar);
        void SetView(OrthoView view);
        void SetZoom(float zoom);
        void SetOffset(float x, float y, float z);
        void AddOffset(float dx, float dy, float dz);
        glm::vec3 GetOffset() const { return m_offset; }

        void SetRotationZ(float degrees);
        float GetRotationZ() const { return m_rotZ; }

        // === NUEVO: Yaw / Pitch para orbita libre ===
        void SetYaw(float yaw);
        void SetPitch(float pitch);
        void AddYaw(float dyaw);
        void AddPitch(float dpitch);
        float GetYaw() const { return m_yaw; }
        float GetPitch() const { return m_pitch; }
        glm::vec3 GetEyeDirection() const;

        void SetCenter(float x, float y, float z);
        void SetAspectRatio(float aspect);
        void LockRotation(bool lock) { m_lockRotation = lock; }
        void LockOffsetX(bool lock) { m_lockOffsetX = lock; }
        void LockOffsetY(bool lock) { m_lockOffsetY = lock; }
        void LockOffsetZ(bool lock) { m_lockOffsetZ = lock; }
        bool IsRotationLocked() const { return m_lockRotation; }
        bool IsOffsetXLocked() const { return m_lockOffsetX; }
        bool IsOffsetYLocked() const { return m_lockOffsetY; }
        bool IsOffsetZLocked() const { return m_lockOffsetZ; }
        float GetFrustumHeight() const { return m_top - m_bottom; }
        float GetFrustumWidth() const { return m_right - m_left; }
        glm::vec3 GetRightVector() const;
        glm::vec3 GetUpVector() const;
        const glm::mat4& GetProjection() const { return m_projection; }
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetPV() const { return m_projection * GetViewMatrix(); }
        float GetZoom() const { return m_zoom; }
        OrthoView GetCurrentView() const { return m_currentView; }
    private:
        void RecalculateProjection();
        float m_left, m_right, m_bottom, m_top, m_zNear, m_zFar;
        float m_zoom = 1.0f, m_rotZ = 0.0f;
        float m_yaw = 0.0f, m_pitch = 90.0f;  // pitch=90 = Top view por defecto
        glm::vec3 m_offset = glm::vec3(0.0f);
        glm::vec3 m_center = glm::vec3(0.0f);
        glm::mat4 m_projection = glm::mat4(1.0f);
        OrthoView m_currentView = OrthoView::Top;
        bool m_lockRotation = false, m_lockOffsetX = false, m_lockOffsetY = false, m_lockOffsetZ = false;
    };

}