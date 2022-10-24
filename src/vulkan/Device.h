/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_DEVICE_H_ 
#define WATER_SURFACE_RENDERING_VULKAN_DEVICE_H_

#include <optional>
#include <vector>
#include <map>
#include "vulkan/Instance.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/QueueTypes.h"


namespace vkp
{
    /** 
     * @brief Logical device abstraction. 
     *  1. Pass device requirements, 
     *  2. Selects a suitable physical device, 
     *  3. creates a logical device to interface with it, along with queues of
     *     the required queue families. 
     */
    class Device 
    {
    public:
        struct Requirements
        {
            VkPhysicalDeviceType         deviceType;
            VkPhysicalDeviceFeatures     deviceFeatures;
            std::vector<VkQueueFlagBits> queueFamilies;
            std::vector<const char*>     deviceExtensions;
            bool                         presentationSupport;
        };

    public:
        /**
         * @brief Based on the supplied requirements, select a physical device
         *  and creates a logical device on top of it
         */
        Device(const Requirements& requirements,
               const std::vector<VkPhysicalDevice>& physicalDevices);
        ~Device();

        operator VkDevice() const { return m_Device; }

        const PhysicalDevice& GetPhysicalDevice() const 
        {
            return m_PhysicalDevice;
        };
        PhysicalDevice& GetPhysicalDevice() { return m_PhysicalDevice; };

        VkQueue GetQueue(QFamily f) const { return m_Queues[f]; }

        /**
         * @brief Submits command buffers in 'submitInfos' to a queue of the
         *  requested queue family.
         * @param qFamily Queue family of the submission queue, the first is used
         * @param submitInfos Vector of 'VkSubmitInfo' structs with buffers, etc.
         * @param fence Handle to a fence to be signaled on execution completion
         */
        VkResult QueueSubmit(const QFamily qFamily, 
                             const std::vector<VkSubmitInfo>& submitInfos,
                             VkFence fence = VK_NULL_HANDLE) const;
        /**
         * @brief Waits for the first queue of the queue family 'qFamily'
         *  to go idle
         */
        VkResult QueueWaitIdle(const QFamily qFamily) const;

        /**
         * @return Result of a call to queue an image for presentation
         */
        VkResult QueuePresent(
            const std::vector<VkPresentInfoKHR>& presentInfos) const;

        inline VkResult WaitIdle() const { return vkDeviceWaitIdle(m_Device); }

        void RetrievePresentQueueHandle();

        /**
         * @brief Creates a descriptor pool on the device side, will be destroyed
         *  at the end of device's lifetime
         * @param maxSets Maximum number of descriptor sets that can be
         *  allocated from the pool
         * @param kPoolSizes Vector of VkDescriptorPoolSize
         * @param flags Bitmask of VkDescriptorPoolCreateFlagBits
         * @return Successfully created descriptor pool
         */
        VkDescriptorPool CreateDescriptorPool(
            uint32_t maxSets, 
            const std::vector<VkDescriptorPoolSize>& kPoolSizes, 
            VkDescriptorPoolCreateFlags flags = 0);
        
        /**
         * @brief Sascha Willems. Dynamic uniform buffers. 2020.
         *  Available at: https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer.
         */
        size_t GetUniformBufferAlignment(size_t originalSize)
        {
            const size_t minUboAlignment = 
                m_PhysicalDevice.GetMinUniformBufferOffsetAlignment();
            size_t alignedSize = originalSize;
            if (minUboAlignment > 0)
            {
                alignedSize = (alignedSize + minUboAlignment - 1) &
                              ~(minUboAlignment - 1);
            }
            return alignedSize;
        }

        size_t GetNonCoherentAtomSizeAlignment(size_t originalSize)
        {
            const size_t kAtomSize = m_PhysicalDevice.GetNonCoherentAtomSize();
            size_t alignedSize = originalSize;
            if (kAtomSize > 0)
            {
                alignedSize = (alignedSize + kAtomSize - 1) & ~(kAtomSize - 1);
            }
            return alignedSize;
        }

        /**
         * @return Format properties of the selected physical device
         */
        VkFormatProperties GetFormatProperties(VkFormat format) const
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(
                m_PhysicalDevice, format, &formatProperties);
            return formatProperties;
        }

    private:
        /**
         * @brief Finds a physical device that satisfies the requirements and
         *  is the highest rated
         */
        void SelectPhysicalDevice(
            const std::vector<VkPhysicalDevice>& physicalDevices,
            const Requirements& requirements);

        static std::multimap<int, PhysicalDevice> GetSuitablePhysicalDevices(
            const std::vector<VkPhysicalDevice>& physicalDevices,
            const Requirements& requirements);

        /**
         * @brief Rates the physical device based on requirements 
         * Enforces the required device type, and size of textures
         * @return Score of the device's suitability, suitable when >0
        */
        static int RateDeviceSuitability(const PhysicalDevice& device,
                                         const Requirements& req);

        void UpdatePhysicalDevice(const Requirements& requirements);

        void CreateLogicalDevice();

        std::vector<VkDeviceQueueCreateInfo> InitRequiredQueueCreateInfos() const;

        void RetrieveQueueHandles();

        void Destroy();

    private:
        VkDevice        m_Device{ VK_NULL_HANDLE }; ///< Logical device handle
        PhysicalDevice  m_PhysicalDevice;           ///< Selected physical device

        Queues m_Queues;    ///< Queues created along with the device

        std::vector<VkDescriptorPool> m_DescriptorPools;
    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_DEVICE_H_