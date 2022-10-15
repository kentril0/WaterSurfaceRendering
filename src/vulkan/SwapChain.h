/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_SWAPCHAIN_H_
#define WATER_SURFACE_RENDERING_VULKAN_SWAPCHAIN_H_

#include <vulkan/vulkan.h>

#include "vulkan/PhysicalDevice.h"
#include "vulkan/Device.h"
#include "vulkan/Surface.h"
#include "vulkan/ImageView.h"
#include "vulkan/Image.h"


namespace vkp
{
    class SwapChain
    {
    public:
        /** 
         * @brief Details of swap chain that are supported by a physical device
         */
        struct SupportDetails
        {
            VkSurfaceCapabilitiesKHR        capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR>   presentModes;
        };

        struct Frame
        {
            VkFence       fence;
            VkImage       backbuffer;
            VkImageView   backbufferView;
            VkFramebuffer framebuffer;
        };

        // Each frame has its own set of semaphores
        //  should be cleaned up when all commands have finished
        struct FrameSemaphores
        {
            VkSemaphore imageAcquiredSemaphore;
            VkSemaphore renderCompleteSemaphore;
        };

        /** 
         * @brief Required extensions that must be supported by a physical
         *  device 
         */
        static inline const std::vector<const char*> s_kRequiredDeviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        /** 
         * @brief Whether a physical device supports any surface details, 
         *  thus swap chain can be created and used as intended
         * @return True if the device supports surface presentation, 
         *  and at least one surface format
         */
        static bool IsSurfaceSupported(VkPhysicalDevice device,
                                       VkSurfaceKHR surface);

        static SupportDetails QuerySwapChainSupport(VkPhysicalDevice device, 
                                                    VkSurfaceKHR surface);

        static VkFormat FindSupportedFormat(
            VkPhysicalDevice physicalDevice,
            const std::vector<VkFormat>& candidates, 
            VkImageTiling tiling,
            VkFormatFeatureFlags features);

    public:
        /**
         * @brief Sets up structures for upcomming swap chain creation.
         *  To be usable, must be created using the "Create" function.
         */
        SwapChain(const Device& device,
                  const Surface& surface);
        ~SwapChain();

        // ---------------------------------------------------------------------
        // Setup
        //  ... Destroy frame-related resources (*renderpass, pipeline, ...)
        //  1. Create the swap chain (or recreate)
        //  2. Create frame-related resources (*renderpass, ...)
        //  3. Create swap chain framebuffers using renderpass

        /** 
         * @brief Creates (Recreates) the swap chain with new dimensions
         * @param depthAttachment To create depth attachment for depth testing
         * @pre Destroyed: renderPass, pipeline, buffers, ... anything that
         *  depends on imageCount
         */
        void Create(uint32_t width,
                    uint32_t height,
                    bool depthAttachment = false);

        operator VkSwapchainKHR() const { return m_SwapChain; }

        /**
         * @brief MUST be called AFTER swap chain creation
         */
        void CreateFramebuffers(const VkRenderPass renderPass);

        bool NeedsRecreation() const { return m_SwapChainRecreate; }

        // ---------------------------------------------------------------------
        // Drawing

        /**
         * @param imageIndex Aquired image index if successfully acquired
         * @return Result of the 'vkAcquireNextImageKHR'
         */
        VkResult AcquireNextImage(uint32_t* imageIndex);

        /**
         * @param waitStages Destination stages to wait on
         * @param cmdBuffers Command buffers to execute
         */
        void DrawFrame(const std::vector<VkPipelineStageFlags>& waitStages,
                       const std::vector<VkCommandBuffer>& cmdBuffers);

        void SubmitFrame(std::vector<VkSemaphore> kWaitSemaphores,
                         const std::vector<VkPipelineStageFlags>& kWaitStages,
                         const std::vector<VkCommandBuffer>& commandBuffers,
                         std::vector<VkSemaphore> kSignalSemaphores);

        void PresentFrame();

        // ---------------------------------------------------------------------

        VkFramebuffer GetFramebuffer(uint32_t index) const
        {
            VKP_ASSERT(index < m_Frames.size());
            return m_Frames[index].framebuffer;
        }

        VkSurfaceKHR GetSurface() const { return m_Surface; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }

        uint32_t GetMinImageCount() const { return m_MinImageCount; }
        uint32_t GetImageCount() const { 
            return static_cast<uint32_t>(m_Frames.size());
        }

        bool HasDepthAttachment() const { return m_HasDepthAttachment; }

        VkFormat GetDepthAttachmentFormat() const {
            return m_DepthImage.GetFormat();
        }

    private:
        void CreateSwapChain(
            const VkSurfaceFormatKHR kSurfaceFormat,
            const VkPresentModeKHR kPresentMode,
            uint32_t width, uint32_t height);

        void RetrieveAllocateImageHandles();
        void CreateImageViews(const VkSurfaceFormatKHR& kSurfaceFormat);
        void CreateSyncObjects();
        void CreateFrameFences();
        void CreateFrameSemaphores();
        void CreateDepthResources();

        static constexpr VkFormat s_kRequestedSurfaceImageFormats[] = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8_UNORM,
            VK_FORMAT_R8G8B8_UNORM
        };
	    static constexpr VkColorSpaceKHR s_kRequestedSurfaceColorSpace {
            VK_COLORSPACE_SRGB_NONLINEAR_KHR
        };

        VkSurfaceFormatKHR SelectSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats) const;

        VkPresentModeKHR SelectSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes) const;

        static int GetMinImageCountFromPresentMode(
            const VkPresentModeKHR presentMode);

        void SelectImageSharingMode(VkSwapchainCreateInfoKHR& createInfo) const;

        VkSurfaceTransformFlagBitsKHR SelectSurfaceTransform() const;

        VkCompositeAlphaFlagBitsKHR SelectCompositeAlpha();

        uint32_t SelectMinImageCount(
            const VkSurfaceCapabilitiesKHR& cap,
            uint32_t minImageCount) const;

        VkExtent2D SelectImageExtent(
            const VkSurfaceCapabilitiesKHR& capabilities, 
            uint32_t width, uint32_t height) const;

        void Destroy();
        void DestroyFrame(Frame& frame) const;
        void DestroyFrameSemaphores(FrameSemaphores& fr) const;
        void DestroyDepthResources();

    private:
        const Device& m_Device;
        const Surface& m_Surface;

        // ---------------------------------------------------------------------
        uint32_t   m_GraphicsQFamily{ 0 };
        uint32_t   m_PresentQFamily { 0 };
        SupportDetails m_Details{};

        VkSwapchainKHR m_SwapChain   { VK_NULL_HANDLE };
        VkSwapchainKHR m_OldSwapChain{ VK_NULL_HANDLE };

        bool m_SwapChainRecreate{ true };

        // ---------------------------------------------------------------------
        // Swap chain images

        VkFormat   m_ImageFormat{ VK_FORMAT_UNDEFINED };
        VkExtent2D m_Extent{};      ///< Resolution of the images

        // Size of frames is imageCount
        std::vector<Frame> m_Frames;
        std::vector<FrameSemaphores> m_FrameSemaphores;

        // ---------------------------------------------------------------------
        // Rendering

        // Min Number of frames to be proceseed concurrently
        static const int MIN_FRAMES_IN_FLIGHT = 2;

        uint32_t m_MinImageCount{ MIN_FRAMES_IN_FLIGHT };  // TODO needed?
        uint32_t m_FrameIndex    { 0 };
        uint32_t m_SemaphoreIndex{ 0 };

        uint32_t m_CurrentFrame { 0 };   ///< Goes conseq. from 0, to imageCount

        // ---------------------------------------------------------------------
        // Depth attachment

        bool m_HasDepthAttachment{ false };

        Image     m_DepthImage;
        ImageView m_DepthImageView;

    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_SWAPCHAIN_H_
