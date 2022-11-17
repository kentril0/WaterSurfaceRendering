/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/CommandPool.h"


namespace vkp
{
    CommandPool::CommandPool(const Device& device, 
                             QFamily qFamily, VkCommandPoolCreateFlags flags)
        : m_Device(device), 
          m_QFamily(qFamily)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);

        m_QFamilyIndex = 
            device.GetPhysicalDevice()
                .GetQueueFamilyIndices()[qFamily].value();

        Create(flags);
    }

    CommandPool::~CommandPool()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void CommandPool::Create(VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_QFamilyIndex;
        poolInfo.flags = flags;

        auto err =
            vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
        VKP_ASSERT_RESULT(err);
    }

    void CommandPool::AllocateCommandBuffers(uint32_t bufferCount, 
                                             VkCommandBufferLevel level)
    {
        const std::size_t kOldSize = m_CommandBuffers.size();

        // Invalidation should not be a problem for pointers to command buffers
        m_CommandBuffers.resize(kOldSize + bufferCount);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = bufferCount;

        auto err =
            vkAllocateCommandBuffers(m_Device, &allocInfo, 
                                     ToVkBuffers(&m_CommandBuffers[kOldSize])
        ); 
        VKP_ASSERT_RESULT(err);
    }

    void CommandPool::Reset(VkCommandPoolResetFlags flags)
    {
        auto err = vkResetCommandPool(m_Device, m_CommandPool, flags);
        VKP_ASSERT_RESULT(err);
    }

    void CommandPool::FreeCommandBuffers()
    {
        VKP_REGISTER_FUNCTION();
        vkFreeCommandBuffers(m_Device, m_CommandPool, 
                             static_cast<uint32_t>(m_CommandBuffers.size()),
                             ToVkBuffers(m_CommandBuffers.data()));

        m_CommandBuffers.clear();
    }

    void CommandPool::Destroy()
    {
        // Also frees all the command buffers
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

        m_CommandBuffers.clear();
    }

} // namespace vkp
