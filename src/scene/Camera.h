/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_CAMERA_H_
#define WATER_SURFACE_RENDERING_SCENE_CAMERA_H_

#include <unordered_map>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>


namespace vkp
{
    /**
     * @brief FPS camera, uses perspective projection by default
     *  By default, all angles are in radians, unless 'deg'/'Deg' suffix specified
     */
    class Camera
    {
    public:
        enum Controls
        {
            KeyForward  = GLFW_KEY_W,
            KeyLeft     = GLFW_KEY_A,
            KeyBackward = GLFW_KEY_S,
            KeyRight    = GLFW_KEY_D,
            KeySpeedup  = GLFW_KEY_LEFT_SHIFT,
            KeyReset    = GLFW_KEY_ESCAPE
        };

        // Can't be constexpr due to AVX instructions enabled by GLM_FORCE_AVX

        static inline const glm::vec3 s_kVecWorldUp{ 0.0, 1.0, 0.0 };

        static inline const float s_kDefaultPitch{ glm::radians(0.0f) };
        static inline const float s_kDefaultYaw  { glm::radians(0.0f) }; 
        static inline const float s_kDefaultFov  { glm::radians(60.0f) };

        static inline const float s_kDefaultNear { 0.1 };
        static inline const float s_kDefaultFar  { 1000.0 };

        static constexpr float s_kMinPitchDeg{ -89.0 };
        static constexpr float s_kMaxPitchDeg{  89.0 };
        static constexpr float s_kMaxYawDeg  { 360.0 };
        static inline const float s_kMinPitch{ glm::radians(s_kMinPitchDeg) };
        static inline const float s_kMaxPitch{ glm::radians(s_kMaxPitchDeg) };
        static inline const float s_kMaxYaw  { glm::radians(s_kMaxYawDeg) };
                                                          
        static inline const float s_kMoveSpeed        { 25.0 };
        static inline const float s_kSpeedupMultiplier{ 3.0  };
        static inline const float s_kMouseSensitivity { 0.1  };

    public:
        Camera(float screenAspectRatio,
               const glm::vec3& pos = glm::vec3(0.0, 0.0, 0.0), 
               const float pitch    = Camera::s_kDefaultPitch,
               const float yaw      = Camera::s_kDefaultYaw,
               const glm::vec3& up  = Camera::s_kVecWorldUp);

        void Update(float deltaTime);

        // @brief Updates the camera vectors based on set yaw and pitch values
        void UpdateVectors();

        inline const glm::vec3& GetPosition() const { return m_Position; }
        inline const glm::vec3& GetFront() const { return m_Front; }
        inline const glm::vec3& GetUp() const { return m_Up; }
        inline const glm::vec3& GetRight() const { return m_Right; }

        inline float GetPitch() const { return m_Pitch; }
        inline float GetYaw() const { return m_Yaw; }
        inline float GetPitchDeg() const { return glm::degrees(m_Pitch); }
        inline float GetYawDeg() const { return glm::degrees(m_Yaw); }

        inline float GetFov() const { return m_Fov; }
        inline float GetFovDeg() const { return glm::degrees(m_Fov); }
        inline float GetNear() const { return m_Near; }
        inline float GetFar() const { return m_Far; }

        inline const glm::mat4& GetViewMat() const { return m_ViewMat; }
        inline const glm::mat4& GetProjMat() const { return m_ProjMat; }

        /** @return View matrix without the translation part */
        inline const glm::mat3 GetView() const
        {
            return glm::mat3(m_Right, m_Up, m_Front);
        }

        // =====================================================================
    
        void SetPosition(const glm::vec3& p) { m_Position = p; }
        void SetFront(const glm::vec3& f) { m_Front = f; }

        void SetPitch(float rad) { _SetPitch(rad); }
        void SetPitchDeg(float deg) { _SetPitch(glm::radians(deg)); }

        void SetYaw(float rad) { _SetYaw(rad); }
        void SetYawDeg(float deg) { _SetYaw(glm::radians(deg)); }

        // ---------------------------------------------------------------------
        // Functions below *AUTOMATICALLY* UPDATE THE PROJECTION MATRIX  

        void SetPerspective(float screenAspectRatio, 
                            float fov, 
                            float near, 
                            float far);

        void SetPerspective(float fov, 
                            float near, 
                            float far)
        {
            SetPerspective(m_AspectRatio, fov, near, far);
        }

        void SetAspectRatio(float aspect);
        void SetFov(float rad);
        void SetFovDeg(float deg) { SetFov( glm::radians(deg) ); }
        void SetNear(float dist);
        void SetFar(float dist);

        // ------------------------------------------------------------------------
        // Input handlers - control the camera
        // ------------------------------------------------------------------------

        void OnMouseMove(double x, double y);
        void OnMouseButton(int button, int action, int mods);
        /**
         * @brief On key press GLFW interface
         * @param key GLFW key 
         * @param action Either pressed (<> 1) or released (==0)
         */
        void OnKeyPressed(int key, int action);
        void OnCursorEntered(int entered) { m_IsFirstCursor = entered; }

    private:
        inline void UpdateViewMat()
        {
            m_ViewMat = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
        }

        inline void UpdateProjMat()
        {
            m_ProjMat = glm::perspective(m_Fov, m_AspectRatio, m_Near, m_Far);
        }

        // @brief Keeps the camera from flipping along Y-axis
        inline void _SetPitch(float rad)
        { 
            m_Pitch = glm::clamp(rad, Camera::s_kMinPitch, Camera::s_kMaxPitch);
        } 

        // @brief Keeps yaw in range <0,2*PI rad>
        inline void _SetYaw(float rad)
        {
            m_Yaw = glm::mod(rad, Camera::s_kMaxYaw);
        }

        // Vulkan Cookbook
        // ISBN: 9781786468154
        // © Packt Publishing Limited
        // Author:   Pawel Lapinski
        // LinkedIn: https://www.linkedin.com/in/pawel-lapinski-84522329
        // Chapter: 10 Helper Recipes
        // Recipe:  05 Preparing an orthographic projection matrix
#ifndef GLM_FORCE_AVX
        constexpr glm::mat4 CalcOrtho(float left, float right,
                                      float bottom, float top,
                                      float near, float far)
#else
        glm::mat4 CalcOrtho(float left, float right,
                            float bottom, float top,
                            float near, float far)
#endif
        {
            return glm::mat4{
                2.0f / (right - left), 0.0f, 0.0f, 0.0f,    // 1th
                0.0f, 2.0f / (bottom - top), 0.0f, 0.0f,    // 2th
                0.0f, 0.0f, 1.0f / (near - far), 0.0f,      // 3th
                -(right + left) / (right - left),           // 4th
                -(bottom + top) / (bottom - top),
                near / (near - far), 1.0f
            };
        }

        // ---------------------------------------------------------------------
        // Controls

        void SetActiveState(int key, bool pressed)
        {
            auto state = m_KeyToState.find(key);
            if (state != m_KeyToState.end())
            {
                (state->second)(pressed);
            }
        }

        void ResetStates()
        { 
            m_IsFirstCursor = true; 
            m_States.fill(false);
        }

    private:
        glm::vec3 m_Position{ 0.0 }; ///< Position of the camera
        glm::vec3 m_Front   { 0.0 }; ///< Direction the camera is looking into
        glm::vec3 m_Up      { 0.0 }; ///< Direction up from the camera
        glm::vec3 m_Right   { 0.0 }; ///< Direction to the right of the camera

        // E.g. looking into xz plane, then:
        //  0 degrees   - looking in -z direction
        //  90 degrees  - looking in -x direction
        //  180 degrees - looking in +z direction
        //  270 degrees - looking in +x direction
        float m_Yaw{ Camera::s_kDefaultYaw };

        // positive ... looking from above the xz plane
        // negative ... looking from below the xz plane
        float m_Pitch{ Camera::s_kDefaultPitch };

        // Perspective projection
        float m_AspectRatio{ 1.0 };             ///< Camera aspect ratio
        float m_Fov { Camera::s_kDefaultFov };  ///< Field of view

        float m_Near{ Camera::s_kDefaultNear }; ///< Distance to the near plane
        float m_Far { Camera::s_kDefaultFar };  ///< Distance to the far plane

        glm::mat4 m_ViewMat{ 1.0 };
        glm::mat4 m_ProjMat{ 1.0 };

        // ---------------------------------------------------------------------
        // Controls

        // Last position of the mouse cursor on the screen
        int m_LastX{ 0 };
        int m_LastY{ 0 };

        bool m_IsFirstCursor{ true }; ///< The first time cursor is registered

        // Active movement states of the camera
        union
        {
            struct
            {
                bool m_IsMovingForward;
                bool m_IsMovingLeft;
                bool m_IsMovingBackward;
                bool m_IsMovingRight;
                bool m_IsActiveSpeedup;
            };
            std::array<bool, 5> m_States{ false };
        };

        const std::unordered_map<int, std::function<void(bool)> > m_KeyToState{
            { KeyForward , [&](bool b){ m_States[0] = b; } },
            { KeyLeft    , [&](bool b){ m_States[1] = b; } },
            { KeyBackward, [&](bool b){ m_States[2] = b; } },
            { KeyRight   , [&](bool b){ m_States[3] = b; } },
            { KeySpeedup , [&](bool b){ m_States[4] = b; } },
            { KeyReset   , [&](bool b){ ResetStates(); } }
        };
        
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_SCENE_CAMERA_H_