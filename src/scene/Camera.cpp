/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/Camera.h"


namespace vkp
{
    Camera::Camera(float aspectRatio, 
                   const glm::vec3& pos, const glm::vec3& front, 
                   const glm::vec3& up)
      : m_Position(pos), 
        m_Front(front),
        m_Up(up),
        m_AspectRatio(aspectRatio)
    {
        VKP_REGISTER_FUNCTION();

        UpdateVectors();
        UpdateViewMat();
        UpdateProjMat();
    }

    void Camera::UpdateVectors()
    {
        // Calculate the new front vector
        // Assumes yaw, pitch in radians!
        m_Front.x = cosf(m_Yaw) * cosf(m_Pitch);
        m_Front.y = sinf(m_Pitch);
        m_Front.z = sinf(m_Yaw) * cosf(m_Pitch);
        m_Front = glm::normalize(m_Front);

        // Re-calculate the right and up vector
        //  must normalize because their length gets closer to 0 the more you 
        //  look up or down, results in slower movement
        m_Right = glm::normalize(glm::cross(m_Front, VEC_WORLD_UP));
        m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
    }

    void Camera::Update(float dt)
    {
        const float velocity = MOVE_SPEED * dt;

        // Moving forwards
        const float speedUpAddition = static_cast<float>(m_IsActiveSpeedup) *
                                      velocity * SPEEDUP_MULTIPLIER;
        m_Position += static_cast<float>(m_IsMovingForward) * 
                      (m_Front * (velocity + speedUpAddition));

        // Moving backwards
        m_Position -= static_cast<float>(m_IsMovingBackward) * m_Front *
                      velocity;
        // Moving right
        m_Position += static_cast<float>(m_IsMovingRight) * m_Right * velocity;
        // Moving Left
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
        float dx = static_cast<float>(x - m_LastX) * MOUSE_SENSITIVITY;
        float dy = static_cast<float>(m_LastY - y) * MOUSE_SENSITIVITY;

        m_LastX = static_cast<int>(x);
        m_LastY = static_cast<int>(y);

        // TODO convert to radians??
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
        if (action == GLFW_PRESS)
        {
            SetActiveState(key, true);
        }
        else if (action == GLFW_RELEASE)
        {
            SetActiveState(key, false);
        }
    }

    void Camera::SetPitch(float deg)
    { 
        VKP_REGISTER_FUNCTION();
        _SetPitch(glm::radians(deg));
        UpdateVectors();
        UpdateViewMat();
    }

    void Camera::SetYaw(float deg)
    {
        VKP_REGISTER_FUNCTION();
        _SetYaw(glm::radians(deg));
        UpdateVectors();
        UpdateViewMat();
    }

    void Camera::SetPerspective(float aspectRatio, float fovDeg, 
                                float near, float far)
    {
        VKP_REGISTER_FUNCTION();
        m_AspectRatio = aspectRatio;
        m_Fov         = glm::radians(fovDeg);
        m_Near        = near;
        m_Far         = far;
        UpdateProjMat();
    }

    void Camera::SetAspectRatio(float aspect)
    {
        VKP_REGISTER_FUNCTION();
        m_AspectRatio = aspect;
        UpdateProjMat();
    }

    void Camera::SetFov(float fovRad)
    {
        VKP_REGISTER_FUNCTION();
        m_Fov = fovRad;
        UpdateProjMat();
    }

    void Camera::SetNear(float dist)
    {
        VKP_REGISTER_FUNCTION();
        m_Near = dist;
        UpdateProjMat();
    }

    void Camera::SetFar(float dist)
    {
        VKP_REGISTER_FUNCTION();
        m_Far = dist;
        UpdateProjMat();
    }

} // namespace vkp
