#include "pch.h"
#include "Gui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"


namespace vkp
{
    Gui::Gui(
        const Instance& instance,
        Device& device,
        const SwapChain& swapChain)
        : m_Instance(instance),
          m_Device(device),
          m_SwapChain(swapChain)
    {
        VKP_REGISTER_FUNCTION();
    }

    Gui::~Gui()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Gui::SetupContext(GLFWwindow* window, 
                           VkRenderPass renderPass)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable Gamepad Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);

        VkDescriptorPool descriptorPool;
        {
            const std::vector<VkDescriptorPoolSize> kPoolSizes = 
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            descriptorPool = m_Device.CreateDescriptorPool(
                1000 * kPoolSizes.size(),
                kPoolSizes,
                VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
        }

        {
            const PhysicalDevice& physicalDevice = m_Device.GetPhysicalDevice();

            ImGui_ImplVulkan_InitInfo initInfo{};
            initInfo.Instance = m_Instance;
            initInfo.PhysicalDevice = physicalDevice;
            initInfo.Device = m_Device;
            initInfo.QueueFamily = 
                physicalDevice.GetQueueFamilyIndices().Graphics();
            initInfo.Queue = m_Device.GetQueue(QFamily::Graphics);
            initInfo.PipelineCache = VK_NULL_HANDLE;
            initInfo.DescriptorPool = descriptorPool;
            initInfo.Allocator = allocator;
            initInfo.MinImageCount = m_SwapChain.GetMinImageCount();
            initInfo.ImageCount = m_SwapChain.GetImageCount();
            initInfo.CheckVkResultFn = [](VkResult err){ 
                VKP_ASSERT_RESULT(err);
            };

            // renderPass needed to create its own pipeline
            ImGui_ImplVulkan_Init(&initInfo, renderPass);
        }
    }

    void Gui::Recreate() const
    {
        ImGui_ImplVulkan_SetMinImageCount(m_SwapChain.GetMinImageCount());
    }

    void Gui::Destroy()
    {
        VKP_ASSERT_RESULT(vkDeviceWaitIdle(m_Device));
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Gui::UploadFonts(VkCommandBuffer& commandBuffer) const
    {
        //VkCommandBufferBeginInfo beginInfo{};
        //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        //VKP_ASSERT_RESULT(
        //    vkBeginCommandBuffer(commandBuffer, &beginInfo));

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        //VkSubmitInfo endInfo{};
        //endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        //endInfo.commandBufferCount = 1;
        //endInfo.pCommandBuffers = &commandBuffer;
        //VKP_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

        //m_Device.QueueSubmit(QFamily::Transfer, { endInfo });
        //m_Device.WaitIdle();
    }

    void Gui::DestroyFontUploadObjects() const
    {
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void Gui::NewFrame() const
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();        
    }
    
    void Gui::Render(CommandBuffer& commandBuffer) const
    {
        ImGui::Render();
        //auto &io         = ImGui::GetIO();
        //io.DisplaySize.x = static_cast<float>(width);
        //io.DisplaySize.y = static_cast<float>(height);

        ImDrawData* drawData = ImGui::GetDrawData();
        const bool is_minimized = (drawData->DisplaySize.x <= 0.0f ||
                                   drawData->DisplaySize.y <= 0.0f);
        if (is_minimized)
        {
            return;
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }

} // namespace vkp
