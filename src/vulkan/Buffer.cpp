/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Buffer.h"


namespace vkp
{
    Buffer::Buffer(const Device& device)
        : m_Device(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
    }
    
    Buffer::~Buffer()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Buffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, 
                        VkMemoryPropertyFlags properties, 
                        VkSharingMode sharingMode)
    {
        VKP_REGISTER_FUNCTION();
        CreateBuffer(size, usage, sharingMode);
        AllocateMemory(properties);
        Bind();
    }

    void Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                              VkSharingMode sharingMode)
    {
        VKP_REGISTER_FUNCTION();
        // Create the buffer 
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = sharingMode;
        bufferInfo.flags = 0;

        VKP_ASSERT_RESULT(
            vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer));
    }

    void Buffer::AllocateMemory(VkMemoryPropertyFlags properties)
    {
        VKP_ASSERT(m_Buffer != VK_NULL_HANDLE);
        VKP_REGISTER_FUNCTION();

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memRequirements);

        VKP_LOG_INFO("Allocating buffer, size: {} B", memRequirements.size);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 
            m_Device.GetPhysicalDevice().FindMemoryType(
                memRequirements.memoryTypeBits, 
                properties);

        VKP_ASSERT_RESULT(
            vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_BufferMemory));
    }

    void Buffer::Bind(VkDeviceSize offset) const
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Buffer != VK_NULL_HANDLE &&
                        m_BufferMemory != VK_NULL_HANDLE);

        VKP_ASSERT_RESULT(
            vkBindBufferMemory(m_Device, m_Buffer, m_BufferMemory, offset));
    }

    VkResult Buffer::Fill(const void* data, VkDeviceSize size)
    {
        VkResult r = Map(&m_MapAddr);
        if (r != VK_SUCCESS)
        {
            return r;
        }

            CopyToMapped(data, size);

        Unmap();

        return r;
    }

    VkResult Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
    {
        VKP_ASSERT(m_BufferMemory != VK_NULL_HANDLE);

        VkResult r = vkMapMemory(m_Device, m_BufferMemory, offset, size, 0, 
                                 &m_MapAddr);
        return r;
    }

    VkResult Buffer::Map(void** ppData, VkDeviceSize size,
                         VkDeviceSize offset)
    {
        VKP_ASSERT(m_BufferMemory != VK_NULL_HANDLE);

        VkResult r = vkMapMemory(m_Device, m_BufferMemory, offset, size, 0, 
                                 &m_MapAddr);
        if (r == VK_SUCCESS)
        {
            *ppData = m_MapAddr;
        }

        return r;
    }

    void Buffer::Unmap() 
    {
        if (m_MapAddr != nullptr)
        {
            vkUnmapMemory(m_Device, m_BufferMemory);
            m_MapAddr = nullptr;
        }
    }
    
    void Buffer::CopyToMapped(const void* data, VkDeviceSize size, 
                              void* destAddr)
    {
        void* dest = destAddr == nullptr ? m_MapAddr : destAddr;
        memcpy(dest, data, static_cast<size_t>(size));
    }

    void Buffer::FlushMappedRange(VkDeviceSize size, VkDeviceSize offset) const
    {
        // TODO: align according to the brief:
        // Align to the VkPhysicalDeviceProperties::nonCoherentAtomSize
        //uint32_t alignedSize = (imageDataSize-1) - ((imageDataSize-1) % nonCoherentAtomSize);

        VkMappedMemoryRange range = {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 
            .pNext = nullptr,
            .memory = m_BufferMemory,
            .offset = 0,        // TODO align
            .size = size
        };

        vkFlushMappedMemoryRanges(m_Device, 1, &range);
    }

    void Buffer::StageCopy(VkBuffer srcBuffer, const VkBufferCopy *pRegion,
                          VkCommandBuffer commandBuffer)
    {
        vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer, 1, pRegion);
    }

    void Buffer::Destroy()
    {
        if (m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
        }
        if (m_BufferMemory != VK_NULL_HANDLE)
        {
            Unmap();
            vkFreeMemory(m_Device, m_BufferMemory, nullptr);
        }

        DestoryBufferView();
    }

    void Buffer::CreateBufferView(VkFormat format,
        VkFormatFeatureFlags reqFeatures,
        VkDeviceSize offset, VkDeviceSize range)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Buffer != VK_NULL_HANDLE);

        const VkFormatProperties fprops = m_Device.GetFormatProperties(format);
        VKP_ASSERT_MSG(fprops.bufferFeatures & reqFeatures,
                        "BufferView format '{}', does not support the required"
                        " features", format);

        VkBufferViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        info.buffer = m_Buffer;
        info.format = format;
        info.offset = offset;
        info.range = range;

        VKP_ASSERT_RESULT(
            vkCreateBufferView(m_Device, &info, nullptr, &m_BufferView)
        );
    }

    void Buffer::DestoryBufferView()
    {
        VKP_REGISTER_FUNCTION();
        if (m_BufferView != VK_NULL_HANDLE)
        {
            vkDestroyBufferView(m_Device, m_BufferView, nullptr);
            m_BufferView = VK_NULL_HANDLE;
        }
    }

} // namespace vkp
