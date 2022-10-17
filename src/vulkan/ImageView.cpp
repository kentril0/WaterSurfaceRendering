/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/ImageView.h"
#include "vulkan/Device.h"


namespace vkp
{
    ImageView::ImageView(const Device& device)
        : m_Device(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
    }

    ImageView::ImageView(ImageView&& other)
        : m_Device(other.m_Device),
          m_ImageView(other.m_ImageView)
    {
        VKP_REGISTER_FUNCTION();
        other.m_ImageView = VK_NULL_HANDLE;
    }

    ImageView::~ImageView()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void ImageView::Create(VkImage image, VkFormat format, 
                           VkImageAspectFlags aspectFlags, 
                           VkImageViewType viewType, uint32_t mipLevelCount,
                           uint32_t mipLevel, uint32_t arrayLayerCount, 
                           uint32_t arrayLayer)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.components = { VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        //viewInfo.subresourceRange = m_SubresourceRange;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = mipLevel;
        viewInfo.subresourceRange.baseArrayLayer = arrayLayer;
        viewInfo.subresourceRange.levelCount = mipLevelCount;
        viewInfo.subresourceRange.layerCount = arrayLayerCount;

        VKP_ASSERT_RESULT(
            vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView));
    }

    void ImageView::Destroy()
    {
        if (m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }
    }


} // namespace vkp
