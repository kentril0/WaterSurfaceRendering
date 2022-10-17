/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "core/Application.h"


namespace vkp
{
    Application::Application(const std::string& name, AppCmdLineArgs args)
        : m_Name(name), 
          m_Args(args),
          m_StartTimer()
    {
        VKP_REGISTER_FUNCTION();

        srand((unsigned int)time(NULL));
    }

    Application::~Application()
    {
        VKP_REGISTER_FUNCTION();


        // Wait before issuing members' destructors
        if (m_Device != nullptr)
            m_Device->WaitIdle();

        // before swap chain destroy: pipeline, renderpass
    }

    void Application::Init()
    {
        VKP_REGISTER_FUNCTION();
        Timer initTimer;       // TODO to profile

        SetupWindow();
        SetupVulkan();

        this->Start();

        VKP_LOG_INFO("Initialization took: \"{}\" ms", initTimer.ElapsedMillis());
    }

    void Application::Loop()
    {
        VKP_REGISTER_FUNCTION();

        while ( m_Window->IsOpen() )
        {
			// Poll and handle events (inputs, window resize, etc.)
			// Read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell
            //  if dear imgui wants to use inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input
            //  data to main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard
            //  input data to main application.
			// Generally, always pass all inputs to dear imgui, and hide them
            //  from application based on those two flags.
            glfwPollEvents();

            if (m_FramebufferResized || m_SwapChain->NeedsRecreation())
            {
                RecreateSwapChain();
            }

            const float curTime = static_cast<float>(glfwGetTime());
            Timestep dt(curTime - m_LastFrameTime);
            m_LastFrameTime = curTime;

            this->Update(dt);

            uint32_t frameIndex;
            if (m_SwapChain->AcquireNextImage(&frameIndex) != VK_SUCCESS)
                continue;
        /*
            m_Gui->NewFrame();
            {
                if (!ImGui::Begin("Application Configuration"))
                {
                    ImGui::End();
                    return;
                }

                ImGui::ColorEdit3("clear color", &(m_ClearValues[0].color.float32[0]));
                ImGui::End();
            }

        */
            //this->OnImGuiRender();

            std::vector<VkPipelineStageFlags> waitStages;
            std::vector<VkCommandBuffer> cmdBuffers;

            this->Render(
                frameIndex,
                dt,
                waitStages,
                cmdBuffers
            );

            // TODO GUI renderpass
            //m_Gui->Render(cmdBuffers.back());
            // TODO

            m_SwapChain->DrawFrame(
                waitStages,
                cmdBuffers
            );

            m_SwapChain->PresentFrame();
        }

        // Finish operations before exiting and destroying the window
        //  else might destroy objects while still drawing
        m_Device->WaitIdle();
    }

    // =============================================================================
    // =============================================================================
    // Setup functions
    // =============================================================================

    void Application::SetupWindow()
    {
        m_Window = std::make_unique<Window>(m_Name.c_str());

        // Register window callbacks
        m_Window->SetUserPointer(this);
        m_Window->SetFramebufferSizeCallback(Application::FramebufferResizeCallback);
        m_Window->SetCursorPosCallback(Application::OnMouseMoveCallback);
        m_Window->SetMouseButtonCallback(Application::OnMousePressedCallback);
        m_Window->SetKeyCallback(Application::OnKeyPressedCallback);
        m_Window->SetCursorEnterCallback(Application::OnCursorEnterCallback);
    }

    void Application::SetupVulkan()
    {
        CreateInstance();
        CreateDevice();
        CreateSurface();
        SetupSwapChain();

        CreateTransferCommandPool();
    }

    void Application::CreateInstance()
    {
        auto extensions = m_Window->GetRequiredExtensions();

        #ifdef VKP_DEBUG
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        m_Instance = std::make_unique<Instance>(m_Name.c_str(), extensions);
    }

    void Application::CreateDevice()
    {
        std::vector<VkPhysicalDevice> physicalDevices =
            m_Instance->GetPhysicalDevices();
        VKP_ASSERT_MSG(physicalDevices.size() > 0, "No physical devices found");

        m_Device = std::make_unique<Device>(m_Requirements, physicalDevices);
    }

    void Application::CreateSurface()
    {
        VkSurfaceKHR surface = m_Window->CreateSurface(*m_Instance);
        m_Surface = std::make_unique<Surface>(*m_Instance, surface);
    }

    bool Application::MakeDeviceSupportPresentation()
    {
        // Selected physical device must support swap chain creation
        //  and presentation

        auto& physDevice = m_Device->GetPhysicalDevice();

        bool swapChainSupported = physDevice.HasEnabledExtensions(
            SwapChain::s_kRequiredDeviceExtensions);
        VKP_ASSERT_MSG(swapChainSupported,
                       "Physical device does not support swap chain creation!");

        swapChainSupported = physDevice.RequestSwapChainSupport(*m_Surface);
        VKP_ASSERT_MSG(swapChainSupported,
                       "Physical device does not support presentation!");

        m_Device->RetrievePresentQueueHandle();

        return SwapChain::IsSurfaceSupported(physDevice, *m_Surface);
    }

    void Application::SetupSwapChain()
    {
        /* TODO Application has only one device, for now
        if (!m_Requirements.presentationSupport)
        */

        const bool IsSwapChainSupported = MakeDeviceSupportPresentation();
        VKP_ASSERT_MSG(IsSwapChainSupported,
                       "No WSI support on physical device:");

        m_SwapChain = std::make_unique<SwapChain>(*m_Device, *m_Surface);

        {
            int width = 0, height = 0;
            m_Window->GetFramebufferSize(&width, &height);
            m_SwapChain->Create(width, height, m_DepthTestingEnabled);
        }
    }

    // =============================================================================
    // =============================================================================
    // Create functions 
    // =============================================================================

    void Application::RecreateSwapChain()
    {
        VKP_REGISTER_FUNCTION();

        int width = 0, height = 0;
        m_Window->GetFramebufferSize(&width, &height);

        bool isMinimized = width == 0 || height == 0;
        while (isMinimized)
        {
            glfwWaitEvents();

            m_Window->GetFramebufferSize(&width, &height);
            isMinimized = width == 0 || height == 0;
        }

        VKP_ASSERT_RESULT(m_Device->WaitIdle());

        //m_RenderPass->Destroy();
        //DestroyDrawCommandPools();
        m_SwapChain->Create(width, height, m_DepthTestingEnabled);

    /*
        CreateRenderPass();
        m_SwapChain->CreateFramebuffers(*m_RenderPass);
        CreateDrawCommandPools();
        CreateDrawCommandBuffers();

        m_Gui->Recreate();
    */
        // TODO recreate anything based on number of framebuffers
        this->OnFrameBufferResize(width, height);

        m_FramebufferResized = false;
    }

    void Application::CreateTransferCommandPool()
    {
        VKP_REGISTER_FUNCTION();

        m_TransferCmdPool = 
            std::make_unique<CommandPool>(
                *m_Device, QFamily::Graphics,
                VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |  // short-lived
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT // individually reset
        );
    }

    // ============================================================================
    // Utils
    // ============================================================================

    CommandBuffer& Application::CreateTransferCmdBuffer()
    {
        VKP_REGISTER_FUNCTION();

        m_TransferCmdPool->AllocateCommandBuffers(1);
        return m_TransferCmdPool->Back();
    }

    void Application::SubmitTransferCmdBuffer(CommandBuffer& cmdBuffer) const
    {
        VKP_REGISTER_FUNCTION();

        cmdBuffer.End();

        // Execute the command buffer to complete the transfer
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer.buffer;

            m_Device->QueueSubmit(m_TransferCmdPool->GetAssignedQueueFamily(),
                                  { submitInfo });
        }
    }

    void Application::WaitTransferComplete() const
    {
        // Wait for the transfer to complete
        // TODO bad for unified - NEVER WAIT
        m_Device->QueueWaitIdle(m_TransferCmdPool->GetAssignedQueueFamily());
    }

    CommandBuffer& Application::BeginOneTimeCommands()
    {
        VKP_REGISTER_FUNCTION();

        CommandBuffer& cmdBuffer = CreateTransferCmdBuffer();
        cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        return cmdBuffer;
    }

    void Application::EndOneTimeCommands(CommandBuffer& cmdBuffer)
    {
        VKP_REGISTER_FUNCTION();

        SubmitTransferCmdBuffer(cmdBuffer);
        WaitTransferComplete();

        // TODO only free the cmdBuffer
        m_TransferCmdPool->FreeCommandBuffers();
    }

    // =============================================================================
    // External Controls
    // =============================================================================

    void Application::FramebufferResizeCallback(
        GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->SetFrameBufferResized();
    }

    void Application::OnMouseMoveCallback(
        GLFWwindow* window, double xpos, double ypos)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->OnMouseMove(xpos, ypos);
    }

    void Application::OnMousePressedCallback(
        GLFWwindow* window, int button, int action, int mods)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->OnMousePressed(button, action, mods);
    }
    void Application::OnKeyPressedCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->OnKeyPressed(key, action, mods);
    }

    void Application::OnCursorEnterCallback(GLFWwindow* window, int entered)
    {
        auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->OnCursorEntered(entered);
    }


} // namespace vkp
