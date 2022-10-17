/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Device.h"
#include "vulkan/SwapChain.h"


namespace vkp
{
    Device::Device(const Requirements& requirements,
        const std::vector<VkPhysicalDevice>& physicalDevices)
    {
        VKP_REGISTER_FUNCTION();

        SelectPhysicalDevice(physicalDevices, requirements);
        UpdatePhysicalDevice(requirements);

        CreateLogicalDevice();
        RetrieveQueueHandles();
    }

    Device::~Device()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Device::SelectPhysicalDevice(
        const std::vector<VkPhysicalDevice>& physicalDevices,
        const Requirements& requirements)
    {
        VKP_REGISTER_FUNCTION();

        std::multimap<int, PhysicalDevice> candidates =
            GetSuitablePhysicalDevices(physicalDevices, requirements);

        const auto kBestCandidate = candidates.rbegin();
        const bool kBestCandidateIsSuitable = !candidates.empty() &&
                                              kBestCandidate->first > 0;
        VKP_ASSERT_MSG(kBestCandidateIsSuitable,
                       "Failed to find a suitable Vulkan Device");

        m_PhysicalDevice = kBestCandidate->second;
        VKP_ASSERT_MSG(m_PhysicalDevice != VK_NULL_HANDLE,
                       "Failed to find a suitable Vulkan Device");

        VKP_LOG_INFO("Selected physical device: {}", 
                     m_PhysicalDevice.GetProperties().deviceName);
    }

    void Device::UpdatePhysicalDevice(const Requirements& requirements)
    {
        VKP_REGISTER_FUNCTION();

        m_PhysicalDevice.SetEnabledFeatures(requirements.deviceFeatures);
        m_PhysicalDevice.AddEnabledExtensions(requirements.deviceExtensions);

        if (requirements.presentationSupport)
        {
            bool swapChainSupported = m_PhysicalDevice.HasExtensions(
                SwapChain::s_kRequiredDeviceExtensions);
            VKP_ASSERT_MSG(swapChainSupported,
                "Physical device does not support swap chain creation!"
            );

            m_PhysicalDevice.AddEnabledExtensions(
                SwapChain::s_kRequiredDeviceExtensions);
        }
    }

    void Device::CreateLogicalDevice()
    {
        VKP_REGISTER_FUNCTION();

        const std::vector<VkDeviceQueueCreateInfo> kQueueCreateInfos =
            InitRequiredQueueCreateInfos();

        const std::vector<const char*> deviceEnabledExtensions = 
            m_PhysicalDevice.GetEnabledExtensions();

        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 
                static_cast<uint32_t>(kQueueCreateInfos.size()),
            .pQueueCreateInfos = kQueueCreateInfos.data(),

        #ifdef VKP_DEBUG
            .enabledLayerCount =
                static_cast<uint32_t>(Instance::s_kValidationLayers.size()),
            .ppEnabledLayerNames =
                Instance::s_kValidationLayers.data(),
        #else
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
        #endif

            .enabledExtensionCount = 
                static_cast<uint32_t>(deviceEnabledExtensions.size()),
            .ppEnabledExtensionNames = deviceEnabledExtensions.data(),
            .pEnabledFeatures = &m_PhysicalDevice.GetEnabledFeatures()
        };

        auto err = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
        VKP_ASSERT_RESULT(err);
    }

    void Device::RetrieveQueueHandles()
    {
        const auto& kQueueIndices =
            m_PhysicalDevice.GetQueueFamilyIndices().indices;
        const uint32_t kQueueIndex = 0;

        for (uint32_t i = 0; i < TotalQueues(); ++i)
        {
            if (kQueueIndices[i].has_value())
            {
                vkGetDeviceQueue(m_Device, 
                                 kQueueIndices[i].value(), 
                                 kQueueIndex, 
                                 &m_Queues.arr[i]);
            }
        }
    }
    void Device::RetrievePresentQueueHandle()

    {
        const auto& kQueueIndices = m_PhysicalDevice.GetQueueFamilyIndices();
        const uint32_t kQueueIndex = 0;

        VKP_ASSERT_MSG(kQueueIndices[QFamily::Present].has_value(),
                       "Physical device does not support presentation");
        vkGetDeviceQueue(m_Device,
                         kQueueIndices[QFamily::Present].value(), 
                         kQueueIndex, 
                         &m_Queues.arr[QFamilyToSize(QFamily::Present)]
        );
    }

    void Device::Destroy()
    {
        // Descriptor sets allocated from the pool are implicitly freed
        for (auto descriptorPool : m_DescriptorPools)
        {
            vkDestroyDescriptorPool(m_Device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }

        vkDestroyDevice(m_Device, nullptr);
    }

    std::multimap<int, PhysicalDevice> Device::GetSuitablePhysicalDevices(
        const std::vector<VkPhysicalDevice>& physicalDevices,
        const Requirements& requirements)
    {
        VKP_REGISTER_FUNCTION();

        std::multimap<int, PhysicalDevice> candidates;

        for (const auto& device : physicalDevices)
        {
            PhysicalDevice physDevice(device);
            physDevice.PrintLogTraceProperties();

            const int kScore = RateDeviceSuitability(physDevice, requirements);

            const bool kIsSuitable = kScore > 0;
            if (kIsSuitable)
                candidates.insert(
                    std::make_pair(kScore, std::move(physDevice) )
                );
        }

        return candidates;
    }

    int Device::RateDeviceSuitability(const PhysicalDevice& device, 
        const Requirements& deviceReq)
    {
        VKP_REGISTER_FUNCTION();

        int score = 0;
        {
            const VkPhysicalDeviceProperties& deviceProps = device.GetProperties();
            const bool kDeviceTypeRequired = deviceReq.deviceType > 0;

            // 1. Enforce the required device type
            if (kDeviceTypeRequired)
            {
                score += static_cast<int>(
                    deviceProps.deviceType == deviceReq.deviceType
                ) * 1000;
            }

            // 2. Maximum possible size of textures 
            score += deviceProps.limits.maxImageDimension2D;
        }

        if ( !device.HasFeatures(deviceReq.deviceFeatures) || 
             !device.HasExtensions(deviceReq.deviceExtensions) ||
             !device.HasQueueFamilies(deviceReq.queueFamilies) )
        {
            return -1;
        }

        return score;
    }

    std::vector<VkDeviceQueueCreateInfo> Device::InitRequiredQueueCreateInfos()
        const
    {
        const auto& kQueueFamilyIndices = m_PhysicalDevice.GetQueueFamilyIndices();
        std::set<uint32_t> uniqueQueueFamilyIndices;

        // Keep only unique queue family indices
        for (const auto index : kQueueFamilyIndices.indices)
        {
            if (index.has_value())
                uniqueQueueFamilyIndices.insert(index.value());
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilyIndices.size());

        const uint32_t kQueueCount = 1;     ///< Only one queue per queue family
        const float kQueuePriority = 1.0f;

        for (const uint32_t index : uniqueQueueFamilyIndices)
        {
            queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = index,
                .queueCount = kQueueCount,
                .pQueuePriorities = &kQueuePriority
            });
        }

        return queueCreateInfos;
    }

    // -------------------------------------------------------------------------

    VkResult Device::QueueSubmit(const QFamily qFamily, 
                                 const std::vector<VkSubmitInfo>& submitInfos,
                                 VkFence fence) const
    {
        return vkQueueSubmit(m_Queues[qFamily], 
                             static_cast<uint32_t>(submitInfos.size()), 
                             submitInfos.data(), 
                             fence);
    }

    VkResult Device::QueueWaitIdle(const QFamily qFamily) const
    {
        return vkQueueWaitIdle(m_Queues[qFamily]);
    }

    VkResult Device::QueuePresent(
        const std::vector<VkPresentInfoKHR>& presentInfos) const
    {
        return vkQueuePresentKHR(m_Queues[QFamily::Present], 
                                 presentInfos.data());
    }

    VkDescriptorPool Device::CreateDescriptorPool(
        uint32_t maxSets,
        const std::vector<VkDescriptorPoolSize>& kPoolSizes, 
        VkDescriptorPoolCreateFlags flags)
    {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = flags;
        poolInfo.maxSets = maxSets;
        poolInfo.poolSizeCount = static_cast<uint32_t>(kPoolSizes.size());
        poolInfo.pPoolSizes = kPoolSizes.data();

        VkDescriptorPool descriptorPool;

        VKP_ASSERT_RESULT(
            vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, 
                                   &descriptorPool));

        // Save for deallocation
        m_DescriptorPools.push_back(descriptorPool);

        return descriptorPool;
    }


} // namespace vkp
