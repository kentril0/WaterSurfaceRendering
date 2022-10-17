/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/QueueTypes.h"


namespace vkp
{
    QFamilyIndex& QFamilyIndices::FlagBits2Index(VkQueueFlagBits bits)
    {
        switch(bits)
        {
            case VK_QUEUE_GRAPHICS_BIT:
                return (*this)[QFamily::Graphics];
            case VK_QUEUE_COMPUTE_BIT:
                return (*this)[QFamily::Compute];
            case VK_QUEUE_TRANSFER_BIT:
                return (*this)[QFamily::Transfer];
            default:
                VKP_ASSERT_MSG(false, "Unknown queue flag bits");
                return (*this)[QFamily::Graphics];
        }
    }

    const QFamilyIndex& QFamilyIndices::FlagBits2Index(VkQueueFlagBits bits) 
        const
    {
        switch(bits)
        {
            case VK_QUEUE_GRAPHICS_BIT:
                return (*this)[QFamily::Graphics];
            case VK_QUEUE_COMPUTE_BIT:
                return (*this)[QFamily::Compute];
            case VK_QUEUE_TRANSFER_BIT:
                return (*this)[QFamily::Transfer];
            default:
                VKP_ASSERT_MSG(false, "Unknown queue flag bits");
                return (*this)[QFamily::Graphics];
        }
    }

} // namespace vkp
