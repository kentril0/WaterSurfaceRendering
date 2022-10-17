/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_APPLICATION_H_
#define WATER_SURFACE_RENDERING_APPLICATION_H_

#include <string>
#include <memory>

#include <vulkan/vulkan.h>

#include "core/Assert.h"
#include "core/Timestep.h"
#include "core/Timer.h"
#include "core/Window.h"
#include "vulkan/Instance.h"
#include "vulkan/Device.h"
#include "vulkan/SwapChain.h"
#include "vulkan/CommandPool.h"


namespace vkp
{
    class Application
    {
    public:
        struct AppCmdLineArgs
        {
            int argc = 0;
            char** argv = nullptr;

            const char* operator[](int i) const
            {
                VKP_ASSERT(i < argc);
                return argv[i];
            }
        };

    public:
        Application(const std::string& name,
                    AppCmdLineArgs args);
        ~Application();

        void Run()
        {
            Init();
            Loop();
        }

        void SetFrameBufferResized() { m_FramebufferResized = true; }

    protected:
        /** @brief Called once in "Run" function, before the main loop */
        virtual void Start() = 0;
        
        /** @brief Called each frame, before rendering */
        virtual void Update(Timestep deltaTime) = 0;

        /** @brief Render call, called each frame */
        virtual void Render(
            uint32_t frameIndex,
            Timestep dt,
            std::vector<VkPipelineStageFlags>& stagesToWait,
            std::vector<VkCommandBuffer>& buffersToSubmit) = 0;

        // Callbacks - conforming to GLFW interface
        // TODO need virtual?
        virtual void OnMouseMove(double xpos, double ypos) {}
        virtual void OnMousePressed(int button, int action, int mods) {}
        virtual void OnKeyPressed(int key, int action, int mods) {}
        virtual void OnCursorEntered(int entered) {}
        virtual void OnFrameBufferResize(int width, int height) {}

    protected:
        // -------------------------------------------------------------------------
        // Utility functions 

        /**
        * @return Allocated command buffer from the transfer command pool, that
        *  can be individually reset for multiple submissions */
        CommandBuffer& CreateTransferCmdBuffer();

        /**
        * @brief Ends the recording, and submits the command buffer to 
        *  the transfer queue
        */
        void SubmitTransferCmdBuffer(CommandBuffer& cmdBuffer) const;

        /** @brief Host waits until the transfer queue is idle */
        void WaitTransferComplete() const;

        /**
        * @brief Allocates a command buffer from the transfer command pool, and
        *  begins the recording as a "one time submit buffer"
        * @return Command buffer in a recording state
        */
        CommandBuffer& BeginOneTimeCommands();

        /**
        * @brief Ends the recording, submits the buffer to the transfer queue,
        *  host waits for the queue to go idle, then frees the command buffer
        * @param cmdBuffer Command buffer in recording state
        */
        void EndOneTimeCommands(CommandBuffer& cmdBuffer);

    protected:
        std::string m_Name;
        AppCmdLineArgs m_Args{};

        Timer m_StartTimer;    ///< Time elapsed since the 'Run()' issued

        // Set for Custom device requirements
        Device::Requirements m_Requirements{
            .deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
            .deviceFeatures = {},
            .queueFamilies = { VK_QUEUE_GRAPHICS_BIT },
            .deviceExtensions = {},
            .presentationSupport = true
        };
        
        // =====================================================================
        // Order of declaration is important!
    
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Instance>  m_Instance;
        std::unique_ptr<Surface>   m_Surface;
        std::unique_ptr<Device>    m_Device;

        std::unique_ptr<SwapChain> m_SwapChain;

        // -------------------------------------------------------------------------
        // Command pools and buffers 
        //  *using UNIFIED QUEUE for both GRAPHICS and TRANSFER commands*
        static const auto s_kTransferCmdPoolQueueFamily = QFamily::Graphics;

        // Used for issuing transfer commands
        std::unique_ptr<CommandPool> m_TransferCmdPool{ nullptr };
        
        float m_LastFrameTime      { 0.0f };
        bool  m_FramebufferResized { false };
        bool m_DepthTestingEnabled{ false };

    private:
        void Init();
        void Loop();

        void SetupVulkan();
        void SetupWindow();

        void CreateInstance();
        void CreateDevice();
        void CreateSurface();
        void SetupSwapChain();

        void SetPhysicalDevicePresentationSupport();

        void RecreateSwapChain();
        void CreateTransferCommandPool();

        // ----------------------------------------------------------------------

        static void FramebufferResizeCallback(GLFWwindow* window,
                                            int width, int height);
        static void OnMouseMoveCallback(GLFWwindow* window,
                                        double xpos, double ypos);
        static void OnMousePressedCallback(GLFWwindow* window,
                                        int button, int action, int mods);
        static void OnKeyPressedCallback(GLFWwindow* window,
                                        int key, int scancode, int action,
                                        int mods);
        static void OnCursorEnterCallback(GLFWwindow* window, int entered);
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_APPLICATION_H_
