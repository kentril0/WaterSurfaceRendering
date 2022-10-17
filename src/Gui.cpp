/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "Gui.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "vulkan/QueueTypes.h"


namespace gui
{
    // Function prototypes

    static void SetupImGuiContext();

    static void InitImGuiVulkan(GLFWwindow* windowHandle,
                                ImGui_ImplVulkan_InitInfo& initInfo,
                                VkRenderPass renderPass);

    // =========================================================================
    // =========================================================================

    void InitImGui(
        VkInstance instance,
        VkPhysicalDevice physDevice,
        VkDevice device,
        uint32_t queueFamily,
        VkQueue queue,
        VkDescriptorPool descriptorPool,
        uint32_t minImageCount,
        uint32_t imageCount,
        void (*checkVkResultFn)(VkResult err),
        GLFWwindow* windowHandle,
        VkRenderPass renderPass,
        VkPipelineCache pipelineCache,
        uint32_t subpass,
        VkSampleCountFlagBits MSAASamples,
        const VkAllocationCallbacks* allocator
    )
    {
        SetupImGuiContext();

        ImGui_ImplVulkan_InitInfo initInfo{
            instance,
            physDevice,
            device,
            queueFamily,
            queue,
            pipelineCache,
            descriptorPool,
            subpass,
            minImageCount,
            imageCount,
            MSAASamples,
            allocator,
            checkVkResultFn
        };

        InitImGuiVulkan(windowHandle,
                        initInfo,
                        renderPass);
    }

    void DestroyImGui()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void UploadFonts(VkCommandBuffer commandBuffer)
    {
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    }

    void DestroyFontUploadObjects()
    {
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void NewFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Render(VkCommandBuffer commandBuffer)
    {
        ImGui::Render();
        //auto &io         = ImGui::GetIO();
        //io.DisplaySize.x = static_cast<float>(width);
        //io.DisplaySize.y = static_cast<float>(height);

        ImDrawData* drawData = ImGui::GetDrawData();
        const bool is_minimized = (drawData->DisplaySize.x <= 0.0f ||
                                   drawData->DisplaySize.y <= 0.0f);
        if (is_minimized)
            return;

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }

    void OnFramebufferResized(uint32_t minImageCount)
    {
        ImGui_ImplVulkan_SetMinImageCount(minImageCount);
    }

    // =========================================================================
    // =========================================================================

    static void SetupImGuiContext()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();
    }

    static void InitImGuiVulkan(GLFWwindow* windowHandle,
                         ImGui_ImplVulkan_InitInfo& initInfo,
                         VkRenderPass renderPass)
    {
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(windowHandle, true);

        // RenderPass needed to create its own pipeline
        ImGui_ImplVulkan_Init(&initInfo, renderPass);
    }

    std::vector<VkDescriptorPoolSize> GetImGuiDescriptorPoolSizes()
    {
        return std::vector<VkDescriptorPoolSize> {
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
    }

} // namespace gui
