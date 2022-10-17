/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_GUI_H_
#define WATER_SURFACE_RENDERING_GUI_H_

#include <vulkan/vulkan.h>


namespace gui
{
    std::vector<VkDescriptorPoolSize> GetImGuiDescriptorPoolSizes();

    /**
     * @param descriptorPool A valid descriptor pool with sizes from
     *  "GetImGuiDescriptorPoolSizes()" call
     */
    void InitImGui(
        VkInstance instance,
        VkPhysicalDevice physDevice,
        VkDevice device,
        uint32_t queueFamily,
        VkQueue queue,
        VkDescriptorPool descriptorPool,
        uint32_t swapChainMinImageCount,
        uint32_t swapChainImageCount,
        void (*checkVkResultFn)(VkResult err),
        GLFWwindow* windowHandle,
        VkRenderPass renderPass,
        VkPipelineCache pipelineCache = nullptr,
        uint32_t subpass = 0,
        VkSampleCountFlagBits MSAASamples  = VK_SAMPLE_COUNT_1_BIT,
        const VkAllocationCallbacks* allocator = nullptr 
    );

    /**
     * @pre Device is idle
     */
    void DestroyImGui();

    /**
     * @param commandBuffer Command buffer in recording state.
     *  After Fonts have been uploaded (the queue is idle), must call
     *  "DestroyFontUploadObjects()"
     */
    void UploadFonts(VkCommandBuffer commandBuffer);

    /**
     * @brief Call after "UploadFonts()" and device idling
    */
    void DestroyFontUploadObjects();

    /**
     * @brief Call before any ImGui draw function
     */
    void NewFrame();

    /**
     * @brief Call after all ImGui draw functions to record drawing commands
     * @pre Inside of vkBeginRenderPass() call
     * @param commandBuffer Command buffer in recording state.
     */
    void Render(VkCommandBuffer commandBuffer);

    /**
     * @brief Call on frame resized
     */
    void OnFramebufferResized(uint32_t minImageCount);
}

#endif // WATER_SURFACE_RENDERING_GUI_H_
