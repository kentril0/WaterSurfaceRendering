/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_COMMAND_POOL_H_
#define WATER_SURFACE_RENDERING_VULKAN_COMMAND_POOL_H_

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "vulkan/Device.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/CommandBuffer.h"


namespace vkp
{
    /** 
     * @brief VkCommandPool abstraction, allocates command buffers
     */
    class CommandPool
    {
    public:
        /**
         * @brief Creates a command pool owned by a logical device that submits
         *  command buffers on queues from a queue family 'qFamily'
         * @param device The logical device that creates the command pool
         * @param qFamily Queue family used for command buffer submission, 
         *  supported by the logical device
         * @param flags Bitmask of creation flags 'VkCommandPoolCreateFlagBits'
         */
        CommandPool(const Device& device, 
                    QFamily qFamily, 
                    VkCommandPoolCreateFlags flags = 0);

        ~CommandPool();

        operator VkCommandPool() const { return m_CommandPool; }

        const std::vector<CommandBuffer>& GetCommandBuffers() const {
            return m_CommandBuffers;
        }
        std::vector<CommandBuffer>& GetCommandBuffers() {
            return m_CommandBuffers;
        }

        CommandBuffer& operator[](std::size_t i) { return m_CommandBuffers[i]; }
        const CommandBuffer& operator[](std::size_t i) const {
            return m_CommandBuffers[i];
        }

        CommandBuffer& Front() { return m_CommandBuffers.front(); }
        const CommandBuffer& Front() const { return m_CommandBuffers.front(); }

        CommandBuffer& Back() { return m_CommandBuffers.back(); }
        const CommandBuffer& Back() const { return m_CommandBuffers.back(); }

        QFamily GetAssignedQueueFamily() const { return m_QFamily; }

        /**
         * @brief Allocates a number of command buffers from the pool,
         *  any subsequent call allocates more buffers
         * @param bufferCount Number of command buffers to allocate
         * @param level Level of the command buffers
         */
        void AllocateCommandBuffers(uint32_t bufferCount, 
                                    VkCommandBufferLevel level = 
                                        VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        /**
         * @brief Resets the command pool on valid use
         */
        void Reset(VkCommandPoolResetFlags flags = 0);

        /**
         * @brief Frees all the allocated command buffers held by the pool
         *  Any command buffers in recording or executable state become invalid.
         */
        void FreeCommandBuffers();

    private:
        void Create(VkCommandPoolCreateFlags flags);

        void Destroy();

        static VkCommandBuffer* ToVkBuffers(CommandBuffer* buf) { 
            return reinterpret_cast<VkCommandBuffer*>(buf);
        }

    private:
        VkDevice m_Device      { VK_NULL_HANDLE };
        QFamily  m_QFamily     { };
        uint32_t m_QFamilyIndex{ 0 };

        VkCommandPool m_CommandPool{ VK_NULL_HANDLE };;

        std::vector<CommandBuffer> m_CommandBuffers;
    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_COMMAND_POOL_H_