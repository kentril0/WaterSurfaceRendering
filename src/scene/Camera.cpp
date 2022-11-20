/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/Camera.h"


namespace vkp
{

    Camera::Camera(float aspectRatio, 
                   const glm::vec3& pos,
                   const float pitch,
                   const float yaw,
                   const glm::vec3& up)
      : m_Position(pos), 
        m_Yaw(yaw),
        m_Pitch(pitch),
        m_AspectRatio(aspectRatio)
    {
        VKP_REGISTER_FUNCTION();

        UpdateVectors();
        UpdateViewMat();
        UpdateProjMat();
    }

    void Camera::UpdateVectors()
    {
        m_Front = glm::normalize(
            glm::vec3(
                glm::cos(m_Yaw) * glm::cos(m_Pitch),
                glm::sin(m_Pitch),
                glm::sin(m_Yaw) * glm::cos(m_Pitch)
            )
        );

        // Re-calculate the right and up vector
        //  must be normalized because their length gets closer to 0 the more you 
        //  look up or down, results in slower movement

        m_Right = glm::normalize(glm::cross(m_Front, s_kVecWorldUp));
        m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
    }

    void Camera::Update(float dt)
    {
        float velocity = s_kMoveSpeed * dt;
        const float kSpeedUpAddition = static_cast<float>(m_IsActiveSpeedup) *
                                       velocity * s_kSpeedupMultiplier;
        velocity += kSpeedUpAddition;

        m_Position += static_cast<float>(m_IsMovingForward) * m_Front * velocity;
        m_Position -= static_cast<float>(m_IsMovingBackward) * m_Front * velocity;
        m_Position += static_cast<float>(m_IsMovingRight) * m_Right * velocity;
        m_Position -= static_cast<float>(m_IsMovingLeft) * m_Right * velocity;

        UpdateViewMat();
    }

    void Camera::OnMouseMove(double x, double y)
    {
        // First time the cursor entered the screen, prevents sudden jumps
        if (m_IsFirstCursor)
        {
            m_LastX = static_cast<int>(x);
            m_LastY = static_cast<int>(y);
            m_IsFirstCursor = false;
        }

        // Calculate the offset movement between the last and current frame
        float dx = static_cast<float>(x - m_LastX) * s_kMouseSensitivity;
        float dy = static_cast<float>(m_LastY - y) * s_kMouseSensitivity;

        m_LastX = static_cast<int>(x);
        m_LastY = static_cast<int>(y);

        _SetPitch(m_Pitch + glm::radians(dy));
        _SetYaw(m_Yaw + glm::radians(dx));

        UpdateVectors();
        UpdateViewMat();
    }

    void Camera::OnMouseButton(int button, int action, int mods)
    {

    }

    void Camera::OnKeyPressed(int key, int action)
    {
        // ASSERT: GLFW_RELEASE == 0, GLFW_PRESS == 1, ...
        SetActiveState( key, bool(action) );
    }

    void Camera::SetPerspective(float aspectRatio, float fov,
                                float near, float far)
    {
        m_AspectRatio = aspectRatio;
        m_Fov         = fov;
        m_Near        = near;
        m_Far         = far;
        UpdateProjMat();
    }

    void Camera::SetAspectRatio(float aspect)
    {
        m_AspectRatio = aspect;
        UpdateProjMat();
    }

    void Camera::SetFov(float fov)
    {
        m_Fov = fov;
        UpdateProjMat();
    }

    void Camera::SetNear(float dist)
    {
        m_Near = dist;
        UpdateProjMat();
    }

    void Camera::SetFar(float dist)
    {
        m_Far = dist;
        UpdateProjMat();
    }

} // namespace vkp
