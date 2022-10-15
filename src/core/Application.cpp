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
        m_Device->WaitIdle();

        // before swap chain destroy: pipeline, renderpass
    }

    void Application::Init()
    {
        VKP_REGISTER_FUNCTION();
        Timer initTimer;       // TODO to profile

        SetupWindow();
        SetupVulkan();

        SetupGUI();

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

            this->OnImGuiRender();

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

    void Application::Render(
        uint32_t frameIndex,
        Timestep dt,
        std::vector<VkPipelineStageFlags>& stagesToWait,
        std::vector<VkCommandBuffer>& buffersToSubmit)
    {
        auto& drawCmdPool = m_DrawCmdPools[frameIndex];
        drawCmdPool->Reset();
    
        vkp::CommandBuffer& commandBuffer = drawCmdPool->Front();
        commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            BeginRenderPass(
                commandBuffer,
                m_SwapChain->GetFramebuffer(frameIndex)
            );

            m_Gui->Render(commandBuffer);

            vkCmdEndRenderPass(commandBuffer);
        }
        commandBuffer.End();

        stagesToWait.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        buffersToSubmit.push_back(commandBuffer);
    }

    void Application::BeginRenderPass(
        VkCommandBuffer commandBuffer,
        VkFramebuffer framebuffer)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = *m_RenderPass;
        renderPassInfo.framebuffer = framebuffer;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_SwapChain->GetExtent();

        renderPassInfo.clearValueCount = 
            static_cast<uint32_t>(m_ClearValues.size());
        renderPassInfo.pClearValues = m_ClearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, 
                             VK_SUBPASS_CONTENTS_INLINE);
    }

    // =============================================================================
    // =============================================================================
    // Setup functions
    // =============================================================================

    void Application::SetupVulkan()
    {
        CreateInstance();
        CreateDevice();
        CreateSurface();

        bool presentationSupportRequired = true;
        if (presentationSupportRequired)
            SetPhysicalDevicePresentationSupport();

        SetupSwapChain();

        CreateRenderPass();
        m_SwapChain->CreateFramebuffers(*m_RenderPass);

        CreateDrawCommandPools();
        CreateTransferCommandPool();

        CreateDrawCommandBuffers();
    }

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
        Device::Requirements requirements{};
        requirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        requirements.deviceFeatures.fillModeNonSolid = VK_TRUE;
        requirements.deviceFeatures.samplerAnisotropy = VK_TRUE;
        requirements.queueFamilies = { VK_QUEUE_GRAPHICS_BIT };
        requirements.presentationSupport = true;

        std::vector<VkPhysicalDevice> physicalDevices =
            m_Instance->GetPhysicalDevices();
        VKP_ASSERT_MSG(physicalDevices.size() > 0, "No physical devices found");

        m_Device = std::make_unique<Device>(requirements, physicalDevices);
    }

    void Application::CreateSurface()
    {
        VkSurfaceKHR surface = m_Window->CreateSurface(*m_Instance);
        m_Surface = std::make_unique<Surface>(*m_Instance, surface);
    }

    void Application::SetPhysicalDevicePresentationSupport()
    {
        // Selected physical device must support swap chain creation
        //  and presentation

        auto& physDevice = m_Device->GetPhysicalDevice();

        bool supportsPresent = physDevice.RequestSwapChainSupport(*m_Surface);
        VKP_ASSERT_MSG(supportsPresent,
                       "Physical device does not support presentation!");

        m_Device->RetrievePresentQueueHandle();
    }

    void Application::SetupSwapChain()
    {
        const auto& physDevice = m_Device->GetPhysicalDevice();

        const bool IsSwapChainSupported =
            SwapChain::IsSurfaceSupported(physDevice, *m_Surface);

        VKP_ASSERT_MSG(IsSwapChainSupported,
                       "No WSI support on physical device:");
        physDevice.PrintLogTraceProperties();

        m_SwapChain = std::make_unique<SwapChain>(*m_Device, *m_Surface);

        {
            int width = 0, height = 0;
            m_Window->GetFramebufferSize(&width, &height);
            m_SwapChain->Create(width, height, m_DepthTestingEnabled);
        }
    }

    void Application::SetupGUI()
    {
        VKP_REGISTER_FUNCTION();

        m_Gui = std::make_unique<Gui>(*m_Instance, *m_Device, *m_SwapChain);

        CreateImGuiContext();
    }

    void Application::CreateImGuiContext()
    {
        VKP_REGISTER_FUNCTION();

        m_Gui->SetupContext(*m_Window, *m_RenderPass);

        CommandBuffer& cmdBuffer = BeginOneTimeCommands();

            m_Gui->UploadFonts(cmdBuffer.buffer);

        EndOneTimeCommands(cmdBuffer);

        m_Gui->DestroyFontUploadObjects();
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

        m_RenderPass->Destroy();
        DestroyDrawCommandPools();
        m_SwapChain->Create(width, height, m_DepthTestingEnabled);

        CreateRenderPass();
        m_SwapChain->CreateFramebuffers(*m_RenderPass);
        CreateDrawCommandPools();
        CreateDrawCommandBuffers();

        m_Gui->Recreate();

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

    void Application::CreateRenderPass()
    {
        VKP_REGISTER_FUNCTION();

        m_RenderPass.reset(
            new RenderPass(
                *m_Device,
                m_SwapChain->GetImageFormat(),
                m_SwapChain->GetDepthAttachmentFormat())
        );

        m_RenderPass->Create(m_SwapChain->HasDepthAttachment());
    }

    void Application::CreateDrawCommandPools()
    {
        VKP_REGISTER_FUNCTION();
        // A command pool for each frame that is going to reset its buffers 
        //  every time
        // TODO maybe on recreate
        const uint32_t kImageCount = m_SwapChain->GetImageCount();

        m_DrawCmdPools.reserve(kImageCount);

        for (uint32_t i = 0; i < kImageCount; ++i)
        {
            m_DrawCmdPools.emplace_back(
                std::make_unique<vkp::CommandPool>(
                    *m_Device,
                    vkp::QFamily::Graphics,
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT    // short-lived
                )
            );
        }
    }

    void Application::CreateDrawCommandBuffers()
    {
        VKP_REGISTER_FUNCTION();
        const uint32_t kBuffersPerFrame = 1;

        for (auto& commandPool : m_DrawCmdPools)
        {
            commandPool->AllocateCommandBuffers(kBuffersPerFrame);
        }
    }

    void Application::DestroyDrawCommandPools()
    {
        m_DrawCmdPools.clear();
    }

} // namespace vkp
