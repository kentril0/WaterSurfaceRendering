#ifndef WATER_SURFACE_RENDERING_GUI_H_
#define WATER_SURFACE_RENDERING_GUI_H_

#include "vulkan/Instance.h"
#include "vulkan/Device.h"
#include "vulkan/SwapChain.h"
#include "vulkan/CommandBuffer.h"

#include "imgui/imgui.h"


namespace vkp
{
    /**
     * @brief Abstraction above Dear ImGui for Vulkan using GLFW
     */
    class Gui
    {
    public:
        /* Must have
        - glfw init, window created, 
        - setupVulkan:
            instance
            selected physical device
            queue families, 
            logical device
            queues
            precise descriptor pool
        - created surface
        - SetupVulkanWindow
            SurfaceFormat
            PresentMode selected
            minImageCount
            CreateOrResizeWindow:
                CreateWindowSwapChain
                    - cleanupSwapchain
                    - createSwapChain
                    - createSyncObjects
                    - createRenderpass
                    - imageviews
                    - framebuffer with that renderpass
                CreateWindowCommandBuffers

        - create context
        - VulkanInit
         */
        Gui(const Instance& instance, 
            Device& device, 
            const SwapChain& swapChain);
        ~Gui();

        void SetupContext(GLFWwindow* window, 
                          VkRenderPass renderPass);

        void UploadFonts(VkCommandBuffer& commandBuffer) const;
        void DestroyFontUploadObjects() const;

        void NewFrame() const;

        void Render(CommandBuffer& commandBuffer) const;

        // @brief On Swap chain recreation
        void Recreate() const;

    private:
        void Destroy();

    private:
        const VkAllocationCallbacks* allocator{ nullptr };
        const Instance&  m_Instance;
        Device&    m_Device;
        const SwapChain& m_SwapChain;

    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_GUI_H_