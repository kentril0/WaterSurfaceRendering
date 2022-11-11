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
     * @brief First person, controls: 
     * GLFW mouse control interface
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

        /**
         * @brief TODO
         * @param aspectRatio Camera aspect ratio, should be same as screen's
         * @param pos Position of the camera in the scene
         * @param front Direction where the camera looks
         * @param up Direction up of the camera
         */
        Camera(float aspectRatio,
               const glm::vec3& pos   = glm::vec3(0.0f, 0.0f,  0.0f), 
               const glm::vec3& front = glm::vec3(0.0f, 0.0f, -1.0f),
               const glm::vec3& up    = glm::vec3(0.0f, 1.0f,  0.0f));

        void Update(float deltaTime);

        inline const glm::vec3& GetPosition() const { return m_Position; }
        inline const glm::vec3& GetFront() const { return m_Front; }
        inline const glm::vec3& GetUp() const { return m_Up; }
        inline const glm::vec3& GetRight() const { return m_Right; }

        // @return pitch angle in degrees
        inline float GetPitch() const { return glm::degrees(m_Pitch); }
        // @return yaw angle in degrees
        inline float GetYaw() const { return glm::degrees(m_Yaw); }
        // @return field of view angle in radians
        inline float GetFov() const { return m_Fov; }
        inline float GetFovDeg() const { return glm::degrees(m_Fov); }
        inline float GetNear() const { return m_Near; }
        inline float GetFar() const { return m_Far; }

        // TODO make it proj view mat!
        // TODO make class member
        inline const glm::mat4& GetViewMat() const { return m_ViewMat; }
        inline const glm::mat4& GetProjMat() const { return m_ProjMat; }

        inline const glm::mat3 GetView() const
        {
            return glm::mat3(m_Right, m_Up, m_Front);
        }
    
        void SetPosition(const glm::vec3& p) { m_Position = p; }
        void SetFront(const glm::vec3& f)
        {
            m_Front = f;
        }

        // @brief Set pitch angle in degrees
        void SetPitch(float deg);

        // @brief Set yaw angle in degrees
        void SetYaw(float deg);

        /**
         * @brief Sets the properties of perspection
         * @param aspectRation Screen aspect ratio
         * @param fovDeg Field of view, in degrees
         * @param near Distance to near
         * @param far Distance to far
         */
        void SetPerspective(float aspectRatio, 
                            float fovDeg, 
                            float near, 
                            float far);

        void SetAspectRatio(float aspect);
        // @brief Set field of view in radians
        void SetFov(float fovRad);
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
        // @brief Updates the camera vectors based on yaw and pitch values
        void UpdateVectors();

        inline void UpdateViewMat()
        {
            m_ViewMat = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
        }

        inline void UpdateProjMat()
        {
            m_ProjMat = glm::perspective(m_Fov, m_AspectRatio, m_Near, m_Far);
            //m_ProjMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);
            //m_ProjMat = CalcOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 2.0f);
        }

        // @brief Keeps the camera from flipping along Y-axis
        inline void _SetPitch(float rad)
        { 
            m_Pitch = glm::clamp(rad, MIN_PITCH_RAD, MAX_PITCH_RAD);
        } 

        // @brief Keeps yaw in range <0,2*PI rad>
        inline void _SetYaw(float rad)
        {
            m_Yaw = glm::mod(rad, MAX_YAW_RAD);
        }

        void ResetStates()
        { 
            m_IsFirstCursor = true; 
            m_States.fill(false);
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

    private:
        glm::vec3 m_Position{}; ///< Position of the camera
        glm::vec3 m_Front   {}; ///< Direction the camera is looking into
        glm::vec3 m_Up      {}; ///< Direction up from the camera
        glm::vec3 m_Right   {}; ///< Direction to the right of the camera

        // Properties of projection matrix
        float m_AspectRatio{ 0.0f };       ///< Camera aspect ratio
        float m_Fov { DEFAULT_FOV_RAD   }; ///< Field of view
        float m_Near{ DEFAULT_NEAR_DIST }; ///< Distance to the near plane
        float m_Far { DEFAULT_FAR_DIST  }; ///< Distance to the far plane

        glm::mat4 m_ViewMat{ 1.0f };
        glm::mat4 m_ProjMat{ 1.0f };

        // ---------------------------------------------------------------------
        // Controls

        // Looking in which direction in xz plane 
        //  0 degrees   - looking in -z direction
        //  90 degrees  - looking in -x direction
        //  180 degrees - looking in +z direction
        //  270 degrees - looking in +x direction
        float m_Yaw{ DEFAULT_YAW_RAD };

        // positive ... looking from above the xz plane
        // negative ... looking from below the xz plane
        float m_Pitch{ DEFAULT_PITCH_RAD };

        // Last position of the mouse cursor on the screen
        int m_LastX{ 0 };
        int m_LastY{ 0 };

        bool m_IsFirstCursor{ true }; ///< Signs the first cursor registered

        // Active movement states of the camera
        union
        {
            struct
            {
                bool m_IsMovingForward ;
                bool m_IsMovingLeft    ;
                bool m_IsMovingBackward;
                bool m_IsMovingRight   ;
                bool m_IsActiveSpeedup ;
            };
            std::array<bool, 5> m_States{ false };
        };

        const std::unordered_map<int, std::function<void(bool)>> m_KeyToState{
            { KeyForward , [&](bool b){ m_States[0] = b; } },
            { KeyLeft    , [&](bool b){ m_States[1] = b; } },
            { KeyBackward, [&](bool b){ m_States[2] = b; } },
            { KeyRight   , [&](bool b){ m_States[3] = b; } },
            { KeySpeedup , [&](bool b){ m_States[4] = b; } },
            { KeyReset   , [&](bool b){ ResetStates(); } }
        };

        void SetActiveState(int key, bool pressed)
        {
            auto state = m_KeyToState.find(key);
            if (state != m_KeyToState.end())
            {
                (state->second)(pressed);
            }
        }
        
        // ---------------------------------------------------------------------

#ifndef GLM_FORCE_AVX
        static constexpr glm::vec3 VEC_WORLD_UP{ 0.0f, 1.0f, 0.0f };

        static constexpr float DEFAULT_FOV_RAD{ glm::radians(60.0f) },
                               DEFAULT_NEAR_DIST  { 0.1f    },
                               DEFAULT_FAR_DIST   { 1000.0f },
                               
        // TODO ?? facing the right dir? shouldnt be computed from up, front?
                               DEFAULT_YAW_RAD{ glm::radians(0.0f) },
                               MAX_YAW_RAD    { glm::radians(360.0f) },

                               MIN_PITCH_RAD    { glm::radians(-89.0f) },
                               DEFAULT_PITCH_RAD{ glm::radians( 0.0f) },
                               MAX_PITCH_RAD    { glm::radians( 89.0f) },

                               MOVE_SPEED        { 25.0f },
                               SPEEDUP_MULTIPLIER{ 3.0f  },
                               MOUSE_SENSITIVITY { 0.1f  };
    };
#else
        static inline const glm::vec3 VEC_WORLD_UP{ 0.0f, 1.0f, 0.0f };

        static inline const float DEFAULT_FOV_RAD{ glm::radians(60.0f) },
                    DEFAULT_NEAR_DIST  { 0.1f    },
                    DEFAULT_FAR_DIST   { 1000.0f },
                               
        // TODO ?? facing the right dir? shouldnt be computed from up, front?
                    DEFAULT_YAW_RAD{ glm::radians(0.0f) },
                    MAX_YAW_RAD    { glm::radians(360.0f) },

                    MIN_PITCH_RAD    { glm::radians(-89.0f) },
                    DEFAULT_PITCH_RAD{ glm::radians( 0.0f) },
                    MAX_PITCH_RAD    { glm::radians( 89.0f) },

                    MOVE_SPEED        { 25.0f },
                    SPEEDUP_MULTIPLIER{ 3.0f  },
                    MOUSE_SENSITIVITY { 0.1f  };
    };

#endif

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_SCENE_CAMERA_H_