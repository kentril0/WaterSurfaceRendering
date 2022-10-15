
#include "pch.h"
#include "vulkan/Image.h"
#include "vulkan/Device.h"


namespace vkp
{
    Image::Image(const Device& device)
        : m_Device(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
    }

    Image::Image(Image&& other)
        : m_Device(other.m_Device),
          m_Image(other.m_Image),
          m_ImageMemory(other.m_ImageMemory),
          m_Type(other.m_Type),
          m_Extent(other.m_Extent),
          m_MipLevelCount(other.m_MipLevelCount),
          m_ArrayLayerCount(other.m_ArrayLayerCount),
          m_Format(other.m_Format),
          m_ImageLayout(other.m_ImageLayout)
    {
        VKP_REGISTER_FUNCTION();
        other.m_Image = VK_NULL_HANDLE;
        other.m_ImageMemory = VK_NULL_HANDLE;
    }

    Image::~Image()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Image::Destroy()
    {
        if (m_Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_Device, m_Image, nullptr);
            m_Image = VK_NULL_HANDLE;
        }
        if (m_ImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, m_ImageMemory, nullptr);
            m_ImageMemory = VK_NULL_HANDLE;
        }
    }

    VkImageType FindImageType(const VkExtent3D& extent)
    {
        uint32_t dimensionCount = 0;
        if (extent.width > 0)
        {
            ++dimensionCount;
        }
        if (extent.height > 0)
        {
            ++dimensionCount;
        }
        if (extent.depth > 1)
        {
            ++dimensionCount;
        }

        VkImageType type{};
        switch (dimensionCount)
        {
        case 1:
            type = VK_IMAGE_TYPE_1D;
            break;
        case 2:
            type = VK_IMAGE_TYPE_2D;
            break;
        case 3:
            type = VK_IMAGE_TYPE_3D;
            break;
        default:
            VKP_ASSERT_MSG(false, "Invalid image type");
            break;
        }

        return type;
    }

    void Image::Create(const VkExtent3D& extent, uint32_t mipLevels, 
                       VkFormat format, VkImageUsageFlags usage,
                       VkMemoryPropertyFlags memoryProps, VkImageTiling tiling,
                       VkSampleCountFlagBits samples, uint32_t arrayLayers,
                       VkImageCreateFlags flags, uint32_t queueFamilyCount,
                       const uint32_t* queueFamilies)
    {
        VKP_ASSERT(mipLevels > 0 && arrayLayers > 0 && queueFamilyCount >= 0);

        VKP_LOG_INFO("Creating image: {} x {} x {}", 
                     extent.width, extent.height, extent.depth);

        m_Type = FindImageType(extent);
        m_Extent = extent;
        m_MipLevelCount = mipLevels;
        m_ArrayLayerCount = arrayLayers;
        m_Format = format;

        // Create the image
        {
            //CheckFormatTilingSupport();

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.flags = flags;
            imageInfo.imageType = m_Type;
            imageInfo.format = m_Format;
            imageInfo.extent = extent;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = arrayLayers;
            imageInfo.samples = samples;
            imageInfo.tiling = tiling;
            imageInfo.usage = usage;

            if (queueFamilyCount == 0)
            {
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            else if (queueFamilyCount > 0)
            {
                imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
                imageInfo.queueFamilyIndexCount = queueFamilyCount; 
                imageInfo.pQueueFamilyIndices = queueFamilies;
            }

            imageInfo.initialLayout = m_ImageLayout;

            VKP_ASSERT_RESULT(
                vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image));
        }

        AllocateMemory(memoryProps);

        BindMemory();
    }

    void Image::AllocateMemory(VkMemoryPropertyFlags properties)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

        VKP_LOG_INFO("Allocating image with size: {}", memRequirements.size);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 
            m_Device.GetPhysicalDevice().FindMemoryType(
                memRequirements.memoryTypeBits, 
                properties);

        VKP_ASSERT_RESULT(
            vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_ImageMemory));
    }

    void Image::BindMemory(VkDeviceSize offset)
    {
        VKP_ASSERT(m_Image != VK_NULL_HANDLE && 
                   m_ImageMemory != VK_NULL_HANDLE);
        VKP_ASSERT_RESULT(
            vkBindImageMemory(m_Device, m_Image, m_ImageMemory, offset));
    }

    void Image::TransitionLayout_UNDEFtoDST_OPTIMAL(VkCommandBuffer cmdBuffer)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Image != VK_NULL_HANDLE && 
                   m_ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED);

        RecordImageBarrier(
            cmdBuffer,
            // srcStageMask,                  dstStageMask
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            // here, writes don't have to wait on anything, but must be completed
            //  before transfer stages can start
            // srcAccessMask, dstAccessMask
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            // All writes must be AVAILABLE before layout change
            // >> Layout transition
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    void Image::TransitionLayout_DST_OPTIMALtoSHADER_READ(
        VkCommandBuffer cmdBuffer, VkPipelineStageFlags dstStage)
    {
        VKP_REGISTER_FUNCTION();
        VKP_LOG_INFO("{} {}", dstStage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        VKP_ASSERT(m_Image != VK_NULL_HANDLE && 
                   m_ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        RecordImageBarrier(
            cmdBuffer,
            // srcStageMask,                  dstStageMask
            VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage,
            // wait for writes, before reads
            // srcAccessMask, dstAccessMask
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            // All writes must be AVAILABLE before layout change
            // >> Layout transition
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    void Image::TransitionLayout_SHADER_READtoDST_OPTIMAL(
        VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage)
    {
        VKP_ASSERT(m_Image != VK_NULL_HANDLE && 
                   m_ImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        RecordImageBarrier(
            cmdBuffer,
            // srcStageMask,                  dstStageMask
            srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
            // wait for writes, before reads
            // srcAccessMask, dstAccessMask
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            // All writes must be AVAILABLE before layout change
            // >> Layout transition
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    void Image::TransitionLayoutToDST_OPTIMAL(VkCommandBuffer cmdBuffer,
        VkPipelineStageFlags stage)
    {
        switch(m_ImageLayout)
        {
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                TransitionLayout_SHADER_READtoDST_OPTIMAL(cmdBuffer, stage);
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return;
            case VK_IMAGE_LAYOUT_UNDEFINED:
                TransitionLayout_UNDEFtoDST_OPTIMAL(cmdBuffer);
                break;
            default:
                TransitionLayout(cmdBuffer,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                //VKP_ASSERT_MSG(false, "Unsupported layout transition");
                break;
        }
    }
        
    void Image::RecordImageBarrier(VkCommandBuffer cmdBuffer,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout newLayout) const
    {
        VKP_ASSERT(cmdBuffer != VK_NULL_HANDLE);

        VkImageMemoryBarrier memoryBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = srcAccessMask,
            // All writes must be AVAILABLE before layout change
            .dstAccessMask = dstAccessMask,
            // >> Layout transition
            .oldLayout = m_ImageLayout,
            .newLayout = newLayout,
            // Not transferring queue family ownership
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_Image,
            .subresourceRange = {   // Transition the whole image at once
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = m_MipLevelCount,
                .baseArrayLayer = 0,
                .layerCount = m_ArrayLayerCount
            }
        };

        vkCmdPipelineBarrier(
            cmdBuffer,
            srcStageMask, dstStageMask,
            0,                          // dependencyFlags
            0, nullptr,                 // memory barriers
            0, nullptr,                 // buffer memory barriers
            1, &memoryBarrier           // image memory barriers
        );
    }

    VkImageCreateInfo Image::InitImageInfo()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = 1;
        imageInfo.extent.height = 1;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        return imageInfo;
    }

    void Image::TransitionLayout(VkCommandBuffer cmdBuffer,
        VkImageLayout newLayout, VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask)
    {
        VKP_REGISTER_FUNCTION();
        VkImageLayout oldLayout = m_ImageLayout;
        VkAccessFlags srcAccessMask = 0;

        // Source image layouts
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch(oldLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined, only valid as initial layout
            // No flags required
            srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Only valid as initial layout for linear images, preserves memory contents
            // Host writes must be finished
            srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Writes to the color buffer must be finished
            srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Any writes to the depth/stencil buffer must be finished
            srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Any reads from the image must be finished
            srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Any writes to the image must be finished
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Any shader reads from the image must be finished
            srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        default:
            break;
        }

        // Transition to new layout
        // Destination access mask controls the dependency for the new layout
        VkAccessFlags dstAccessMask = 0;
        
        switch(newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            if (srcAccessMask == 0)
            {
                srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        default:
            break;
        }

        RecordImageBarrier(cmdBuffer,
            srcStageMask, dstStageMask,
            srcAccessMask, dstAccessMask,
            newLayout
        );

        m_ImageLayout = newLayout;
    }

} // namespace vkp
