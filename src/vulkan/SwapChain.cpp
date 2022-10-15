/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/SwapChain.h"


namespace vkp
{
    SwapChain::SwapChain(const Device& device, const Surface& surface)
        : m_Device(device),
          m_Surface(surface),
          m_DepthImage(device),
          m_DepthImageView(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
        VKP_ASSERT(surface != VK_NULL_HANDLE);

        const PhysicalDevice& physicalDevice = m_Device.GetPhysicalDevice();
        VKP_ASSERT(physicalDevice.SupportsPresentation());

        const auto& kIndices = physicalDevice.GetQueueFamilyIndices();
        m_GraphicsQFamily = kIndices[QFamily::Graphics].value();
        m_PresentQFamily = kIndices[QFamily::Present].value();

        m_Details = QuerySwapChainSupport(physicalDevice, m_Surface);
    }

    SwapChain::~SwapChain()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    bool SwapChain::IsSurfaceSupported(VkPhysicalDevice device,
                                       VkSurfaceKHR surface)
    {
        VKP_REGISTER_FUNCTION();
        SupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
        return !swapChainSupport.formats.empty() && 
               !swapChainSupport.presentModes.empty();
    }

    SwapChain::SupportDetails SwapChain::QuerySwapChainSupport(
        VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VKP_REGISTER_FUNCTION();
        SupportDetails details;

        // Query the basic surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, 
                                                  &details.capabilities);

        // Query supported surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, 
                                             nullptr);
        VKP_ASSERT_MSG(formatCount > 0, "No surface format is supported");

        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, 
                                             details.formats.data());

        // Query the supported presentation modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, 
                                                  &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, 
                                                      &presentModeCount, 
                                                      details.presentModes.data());
        }

        return details;
    }

    VkFormat SwapChain::FindSupportedFormat(
        VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, 
        VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        VKP_REGISTER_FUNCTION();
        // Support of a format depends on the tiling mode and usage
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && 
                (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                     (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        VKP_ASSERT_MSG(false, "Failed to find supported format.");
        return VK_FORMAT_UNDEFINED;
    }

    void SwapChain::Create(uint32_t width, uint32_t height,
                           bool depthAttachment)
    {
        VKP_REGISTER_FUNCTION();

        const VkSurfaceFormatKHR kSurfaceFormat = 
            SelectSwapSurfaceFormat(m_Details.formats);

        const VkPresentModeKHR kPresentMode =
            SelectSwapPresentMode(m_Details.presentModes);

        auto err = m_Device.WaitIdle();
        VKP_ASSERT_RESULT(err);

        VKP_ASSERT(m_Frames.size() == m_FrameSemaphores.size());
        for (uint32_t i = 0; i < m_Frames.size(); ++i)
        {
            DestroyFrame(m_Frames[i]);
            DestroyFrameSemaphores(m_FrameSemaphores[i]);
        }

        if (m_HasDepthAttachment)
            DestroyDepthResources();

        // Min image count was not specified
        if (m_MinImageCount == 0)
            m_MinImageCount = GetMinImageCountFromPresentMode(kPresentMode);

        VkSwapchainKHR oldSwapChain = m_SwapChain;

        CreateSwapChain(kSurfaceFormat, kPresentMode, width, height);

        RetrieveAllocateImageHandles();

        if (oldSwapChain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(m_Device, oldSwapChain, nullptr);

        CreateImageViews(kSurfaceFormat);
        CreateSyncObjects();

        if (depthAttachment)
            CreateDepthResources();

        m_HasDepthAttachment = depthAttachment;

        m_FrameIndex = 0;
        m_SwapChainRecreate = false;
    }

    // =========================================================================
    // =========================================================================
    // Frame Rendering
    // =========================================================================

    VkResult SwapChain::AcquireNextImage(uint32_t* imageIndex)
    {
        VKP_ASSERT(imageIndex != nullptr);

        auto& imageAcquiredSemaphore =
            m_FrameSemaphores[m_SemaphoreIndex].imageAcquiredSemaphore;

        VkResult err = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX,
                                             imageAcquiredSemaphore,
                                             VK_NULL_HANDLE, &m_FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        {
            m_SwapChainRecreate = true;
            return err;
        }
        VKP_ASSERT_RESULT_MSG(err, "Failed to acquire swap chain image");

        *imageIndex = m_FrameIndex;

        m_CurrentFrame = (m_CurrentFrame + 1) % GetImageCount();

        // Wait for any previous frame that is using the image
        //  there is a fence to wait on

        auto& fence = m_Frames[m_FrameIndex].fence;

        auto err1 = vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);
        VKP_ASSERT_RESULT(err1);

        //  Restore the fence to the unsignaled state
        err1 = vkResetFences(m_Device, 1, &fence);
        VKP_ASSERT_RESULT(err1);

        return err;
    }

    void SwapChain::DrawFrame(
        const std::vector<VkPipelineStageFlags>& waitStages,
        const std::vector<VkCommandBuffer>& commandBuffers)
    {
        if (m_SwapChainRecreate)
            return; 

        // Execute the command buffers

        const std::array<VkSemaphore, 1> kWaitSemaphores { 
            m_FrameSemaphores[m_SemaphoreIndex].imageAcquiredSemaphore
        };
        // Which semaphores to signal once the cmd buffer(s) have finished exec.
        const std::array<VkSemaphore, 1> kSignalSemaphores { 
            m_FrameSemaphores[m_SemaphoreIndex].renderCompleteSemaphore
        };

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr, 
            .waitSemaphoreCount = static_cast<uint32_t>(kWaitSemaphores.size()),
            .pWaitSemaphores = kWaitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 
                static_cast<uint32_t>(commandBuffers.size()),
            .pCommandBuffers = commandBuffers.data(),
            .signalSemaphoreCount = 
                static_cast<uint32_t>(kSignalSemaphores.size()),
            .pSignalSemaphores = kSignalSemaphores.data()
        };

        //  Add fences to sync CPU-GPU - should be signaled when the command buffer
        //  finishes executing -> signal that a frame has finished

        auto err = m_Device.QueueSubmit(QFamily::Graphics, { submitInfo },
                                        m_Frames[m_FrameIndex].fence);
        VKP_ASSERT_RESULT_MSG(err, "Failed to submit draw command buffer");
    }

    void SwapChain::SubmitFrame(
        std::vector<VkSemaphore> waitSemaphores,
        const std::vector<VkPipelineStageFlags>& kWaitStages,
        const std::vector<VkCommandBuffer>& commandBuffers,
        std::vector<VkSemaphore> signalSemaphores)
    {
        if (m_SwapChainRecreate)
            return; 

        // Submit the command buffers

        waitSemaphores.push_back(
            m_FrameSemaphores[m_SemaphoreIndex].imageAcquiredSemaphore
        );

        signalSemaphores.push_back(
            m_FrameSemaphores[m_SemaphoreIndex].renderCompleteSemaphore
        );

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr, 
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = kWaitStages.data(),
            .commandBufferCount = 
                static_cast<uint32_t>(commandBuffers.size()),
            .pCommandBuffers = commandBuffers.data(),
            .signalSemaphoreCount = 
                static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data()
        };

        //  Add fences to sync CPU-GPU - should be signaled when the command buffer
        //  finishes executing -> signal that a frame has finished

        auto err = m_Device.QueueSubmit(QFamily::Graphics, { submitInfo },
                                        m_Frames[m_FrameIndex].fence);
        VKP_ASSERT_RESULT_MSG(err, "Failed to submit draw command buffer");
    }

    void SwapChain::PresentFrame()
    {
        if (m_SwapChainRecreate)
            return; 

        VKP_ASSERT(m_FrameSemaphores.size() > m_SemaphoreIndex);

        // Return the image to the swap chain for presentation

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores =
            &m_FrameSemaphores[m_SemaphoreIndex].renderCompleteSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_SwapChain;
        presentInfo.pImageIndices = &m_FrameIndex;

        // Submits the request to present an image to the swap chain

        VkResult err = m_Device.QueuePresent({ presentInfo });
        if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        {
            m_SwapChainRecreate = true;
            return;
        } 
        VKP_ASSERT_RESULT_MSG(err, "Failed to present swap chain image");

        // Set the following frame for processing
        m_SemaphoreIndex = (m_SemaphoreIndex + 1) % GetImageCount();
    }

    // =========================================================================
    // =========================================================================
    // Create functions
    // =========================================================================

    void SwapChain::CreateSwapChain(
        const VkSurfaceFormatKHR kSurfaceFormat,
        const VkPresentModeKHR kPresentMode,
        uint32_t width, uint32_t height
    )
    {
        VKP_REGISTER_FUNCTION();

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.imageFormat = kSurfaceFormat.format;
        m_ImageFormat = createInfo.imageFormat;

        createInfo.imageColorSpace = kSurfaceFormat.colorSpace;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        SelectImageSharingMode(createInfo);
        //createInfo.imageSharingMode = ...
        //createInfo.queueFamilyIndexCount = ...
        //createInfo.pQueueFamilyIndices = ...

        createInfo.preTransform = SelectSurfaceTransform();
        createInfo.compositeAlpha = SelectCompositeAlpha();
        createInfo.presentMode = kPresentMode;
        createInfo.clipped = VK_TRUE;


        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), 
                                                  m_Surface, 
                                                  &m_Details.capabilities);

        createInfo.minImageCount = SelectMinImageCount(m_Details.capabilities,
                                                       m_MinImageCount);

        createInfo.imageExtent = SelectImageExtent(m_Details.capabilities,
                                                   width, height);
        m_Extent = createInfo.imageExtent;

        // Ease up on the swap chain recreation using the old one 
        createInfo.oldSwapchain = m_SwapChain;
        m_SwapChain = VK_NULL_HANDLE;

        auto err = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr,
                                        &m_SwapChain);
        VKP_ASSERT_RESULT(err);
    }

    void SwapChain::RetrieveAllocateImageHandles()
    {
        VKP_REGISTER_FUNCTION();
        uint32_t imageCount;
        auto err = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount,
                                           nullptr);
        VKP_ASSERT_RESULT(err);

        const uint32_t kBackbufferSize = 16;
        VkImage backbuffers[kBackbufferSize] = {};
        VKP_ASSERT(imageCount >= m_MinImageCount);
        VKP_ASSERT(kBackbufferSize > imageCount);

        err = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount,
                                      backbuffers);
        VKP_ASSERT_RESULT(err);

        //VKP_ASSERT(m_Frames.empty());

        m_Frames.resize(imageCount, Frame{});
        m_FrameSemaphores.resize(imageCount, FrameSemaphores{});

        for (uint32_t i = 0; i < imageCount; ++i)
            m_Frames[i].backbuffer = backbuffers[i];
    }

    void SwapChain::CreateImageViews(
        const VkSurfaceFormatKHR& kSurfaceFormat
    )
    {
        VKP_REGISTER_FUNCTION();

        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = kSurfaceFormat.format;
        info.components.r = VK_COMPONENT_SWIZZLE_R;
        info.components.g = VK_COMPONENT_SWIZZLE_G;
        info.components.b = VK_COMPONENT_SWIZZLE_B;
        info.components.a = VK_COMPONENT_SWIZZLE_A;
        const VkImageSubresourceRange imageRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,      // baseMipLevel
            1,      // levelCount
            0,      // baseArrayLayer
            1       // layerCount
        };
        info.subresourceRange = imageRange;

        for (auto& frame : m_Frames)
        {
            info.image = frame.backbuffer;
            auto err = vkCreateImageView(m_Device, &info, nullptr,
                                         &frame.backbufferView);
            VKP_ASSERT_RESULT(err);
        }
    }

    void SwapChain::CreateFramebuffers(const VkRenderPass renderPass)
    {
        VKP_REGISTER_FUNCTION();

        std::array<VkImageView, 2> attachments;
        uint32_t attachmentCount = static_cast<uint32_t>(attachments.size()) -1;

        if (m_HasDepthAttachment)
        {
            attachments[1] = m_DepthImageView;
            ++attachmentCount;
        }

        // For each swap chain image view create a framebuffer

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // With which render pass the framebuffer needs to be compatible with
        //  the same number and type of attachments
        framebufferInfo.renderPass = renderPass;
        // VkImageView objects that should be bound to the respective 
        //  attachment descriptions
        framebufferInfo.attachmentCount = attachmentCount;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_Extent.width;
        framebufferInfo.height = m_Extent.height;
        framebufferInfo.layers = 1;

        for (auto& frame : m_Frames)
        {
            attachments[0] = frame.backbufferView;

            auto err = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr,
                                           &frame.framebuffer);
            VKP_ASSERT_RESULT(err);
        }
    }

    void SwapChain::CreateSyncObjects()
    {
        VKP_ASSERT(m_Frames.size() == m_FrameSemaphores.size());

        CreateFrameFences();
        CreateFrameSemaphores();
    }

    void SwapChain::CreateFrameFences()
    {
        VkResult err;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // Set the fences to start in a signaled state
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto& frame : m_Frames)
        {
            err = vkCreateFence(m_Device, &fenceInfo, nullptr, 
                                &frame.fence);
            VKP_ASSERT_RESULT(err);
        }
    }

    void SwapChain::CreateFrameSemaphores()
    {
        VkResult err;
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (auto& frameSem : m_FrameSemaphores)
        {
            err = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                                    &frameSem.imageAcquiredSemaphore);
            VKP_ASSERT_RESULT(err);

            err = vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                                    &frameSem.renderCompleteSemaphore);
            VKP_ASSERT_RESULT(err);
        }
    }

    void SwapChain::CreateDepthResources()
    {
        VKP_REGISTER_FUNCTION();

        VkFormat depthFormat = SwapChain::FindSupportedFormat(
            m_Device.GetPhysicalDevice(),
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
              VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        m_DepthImage.Create(VkExtent3D{m_Extent.width, m_Extent.height, 1},
                            1, depthFormat,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_DepthImageView.Create(m_DepthImage, depthFormat,
                                VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    // =========================================================================
    // =========================================================================
    // Query functions
    // =========================================================================

    VkSurfaceFormatKHR SwapChain::SelectSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats) const
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(availableFormats.size() > 0);

        // If the only one format available is VK_FORMAT_UNDEFINED, that implies
        //  that any format is available
        if (availableFormats.size() == 1)
        {
            if (availableFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                VkSurfaceFormatKHR format{};
                format.format = s_kRequestedSurfaceImageFormats[0];
                format.colorSpace = s_kRequestedSurfaceColorSpace;
                return format;
            }
            else
                return availableFormats[0];
        }
        else
        {
            // Select image format (the color channels and types) and colorspace
            for (const auto& reqFormat : s_kRequestedSurfaceImageFormats)
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.format == reqFormat && 
                        availableFormat.colorSpace == s_kRequestedSurfaceColorSpace)
                    {
                        return availableFormat;
                    }
                }
            }
        }

        // If none of the requested is available, settle with the first
        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::SelectSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes) const
    {
        VKP_REGISTER_FUNCTION();

        // Check for VK_PRESENT_MODE_MAILBOX_KHR for lowest latency
        static const VkPresentModeKHR kReqPresentModes[] = {
            VK_PRESENT_MODE_MAILBOX_KHR,
            VK_PRESENT_MODE_IMMEDIATE_KHR,
            VK_PRESENT_MODE_FIFO_KHR
        };

        for (const auto& reqPresentMode : kReqPresentModes)
        {
            for (const auto& availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == reqPresentMode)
                {
                    VKP_LOG_INFO("Selected Present mode: {}", availablePresentMode);
                    return availablePresentMode;
                }
            }
        }

        // FIFO present mode must always be supported as per spec
        VKP_LOG_INFO("Presentation mode: FIFO");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    int SwapChain::GetMinImageCountFromPresentMode(
        const VkPresentModeKHR presentMode)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return 3;
        if (presentMode == VK_PRESENT_MODE_FIFO_KHR ||
            presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            return 2;
        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return 1;

        VKP_ASSERT_MSG(false, "Unknown present mode");
        return 1;
    }

    void SwapChain::SelectImageSharingMode(VkSwapchainCreateInfoKHR& createInfo) const
    {
        // How to handle swap chain images that will be used across 
        //  multiple queue families

        const std::array<uint32_t, 2> kQueueFamilyIndices {
            m_GraphicsQFamily, m_PresentQFamily
        };

        // Graphics and Presentation are not the same queue families
        //  -> concurrent sharing between them
        if (m_GraphicsQFamily != m_PresentQFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            // Number of queue families having access to swap chain images
            createInfo.queueFamilyIndexCount = kQueueFamilyIndices.size();
            createInfo.pQueueFamilyIndices = kQueueFamilyIndices.data();
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
    }

    VkSurfaceTransformFlagBitsKHR SwapChain::SelectSurfaceTransform() const
    {
        // Specify transform to be applied to images in the swap chain
        //  prefer non-rotated transform, if supporrted

        if (m_Details.capabilities.supportedTransforms &
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            return m_Details.capabilities.currentTransform;
        }
    }

    VkCompositeAlphaFlagBitsKHR SwapChain::SelectCompositeAlpha()
    {
        // If the alpha channel should be used for blending with other windows 
        //  find the first supported composite alpha format

        static const std::array<VkCompositeAlphaFlagBitsKHR, 4> kCompositeAlphaFlags {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };

        for (const auto& compositeAlphaFlag : kCompositeAlphaFlags)
        {
            if (m_Details.capabilities.supportedCompositeAlpha & 
                compositeAlphaFlag)
            {
                return compositeAlphaFlag;
            }
        }

        return kCompositeAlphaFlags[0];
    }

    uint32_t SwapChain::SelectMinImageCount(
        const VkSurfaceCapabilitiesKHR& cap,
        uint32_t minImageCount) const
    {
        if (minImageCount < cap.minImageCount)
        {
            return cap.minImageCount;
        }
        // Do not exceed the maximum number of images
        // '0' is a special value that means that there is no maximum
        else if (cap.maxImageCount > 0 &&
                 minImageCount > cap.maxImageCount)
        {
            return cap.maxImageCount;
        }

        return minImageCount;
    }

    VkExtent2D SwapChain::SelectImageExtent(
        const VkSurfaceCapabilitiesKHR& capabilities, 
        uint32_t width, uint32_t height) const
    {
        VKP_REGISTER_FUNCTION();

        // If the surface size is defined, then swap chain size must match
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            // Pick the resolution that best matches the window within the 
            //  minImageExtent and maxImageExtent bounds
            VkExtent2D actualExtent = { width, height };

            actualExtent.width = std::clamp(actualExtent.width, 
                                            capabilities.minImageExtent.width, 
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, 
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }

    // =========================================================================
    // =========================================================================
    // Destroy functions
    // =========================================================================

    void SwapChain::Destroy()
    {
        VKP_REGISTER_FUNCTION();

        if (m_SwapChain == VK_NULL_HANDLE)
            return;

        // TODO wait on queue
        m_Device.WaitIdle();

        VKP_ASSERT(m_Frames.size() == m_FrameSemaphores.size());
        for (uint32_t i = 0; i < m_Frames.size(); ++i)
        {
            DestroyFrame(m_Frames[i]);
            DestroyFrameSemaphores(m_FrameSemaphores[i]);
        }

        if (m_HasDepthAttachment)
            DestroyDepthResources();

        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    void SwapChain::DestroyFrame(Frame& frame) const
    {
        vkDestroyFence(m_Device, frame.fence, nullptr);
        frame.fence = VK_NULL_HANDLE;

        vkDestroyImageView(m_Device, frame.backbufferView, nullptr);
        vkDestroyFramebuffer(m_Device, frame.framebuffer, nullptr);        
    }

    void SwapChain::DestroyFrameSemaphores(FrameSemaphores& fr) const
    {
        vkDestroySemaphore(m_Device, fr.imageAcquiredSemaphore, nullptr);
        vkDestroySemaphore(m_Device, fr.renderCompleteSemaphore, nullptr);

        fr.imageAcquiredSemaphore = VK_NULL_HANDLE;
        fr.renderCompleteSemaphore = VK_NULL_HANDLE;
    }

    void SwapChain::DestroyDepthResources()
    {
        m_DepthImageView.Destroy();
        m_DepthImage.Destroy();
    }

} // namespace vkp
