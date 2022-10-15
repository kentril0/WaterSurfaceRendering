/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/SwapChain.h"


namespace vkp
{
    PhysicalDevice::PhysicalDevice(VkPhysicalDevice device)
    {
        VKP_REGISTER_FUNCTION();

        if (device != VK_NULL_HANDLE)
            Init(device);
    }

    PhysicalDevice::~PhysicalDevice()
    {
        VKP_REGISTER_FUNCTION();
    }

    void PhysicalDevice::Init(VkPhysicalDevice device)
    {
        m_Device = device;
        VKP_ASSERT(m_Device != VK_NULL_HANDLE);

        vkGetPhysicalDeviceProperties(m_Device, &m_Properties);
        vkGetPhysicalDeviceMemoryProperties(m_Device, &m_MemProperties);
        vkGetPhysicalDeviceFeatures(m_Device, &m_Features);

        FindSupportedExtensions();
        FindQueueFamilyProperties();
        FindQueueFamilyIndices();
    }

    void PhysicalDevice::FindSupportedExtensions()
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, 
                                             nullptr);
        m_Extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, 
                                             m_Extensions.data());
    }

    void PhysicalDevice::FindQueueFamilyProperties()
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_Device, &queueFamilyCount,
                                                 nullptr);

        m_QFamilyProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_Device, &queueFamilyCount,
                                                 m_QFamilyProps.data());
    }

    void PhysicalDevice::FindQueueFamilyIndices()
    {
        m_QFamilyIndices = {};

        for (const auto& queueFamily : QUEUE_FAMILY_BITS)
        {
            m_QFamilyIndices.FlagBits2Index(queueFamily) =
                FindQueueFamilyIndex(queueFamily);
        }
    }

    bool PhysicalDevice::RequestSwapChainSupport(const VkSurfaceKHR surface)
    {
        // Check for Swap chain required extension support
        bool extensionsSupported =
            HasExtensions(SwapChain::s_kRequiredDeviceExtensions);

        FindPresentationQueueIndex(surface);

        return extensionsSupported && SupportsPresentation();
    }

    uint32_t PhysicalDevice::FindMemoryType(
        uint32_t memoryTypeBitsRequirement,
        VkMemoryPropertyFlags requiredProperties) const
    {
        const uint32_t kMemoryCount = m_MemProperties.memoryTypeCount;

        for (uint32_t memoryIdx = 0; memoryIdx < kMemoryCount; ++memoryIdx)
        {
            const uint32_t kMemoryTypeBits = (1 << memoryIdx);
            const bool kIsRequiredMemoryType = memoryTypeBitsRequirement &
                                               kMemoryTypeBits;
            if (kIsRequiredMemoryType)
            {
                const VkMemoryPropertyFlags kProperties =
                    m_MemProperties.memoryTypes[memoryIdx].propertyFlags &
                    requiredProperties;

                if (kProperties == requiredProperties)
                {
                    return memoryIdx;
                }
            }
        }

        VKP_ASSERT_MSG(false, "Failed to find required memory type");
        return UINT32_MAX;
    }

    std::vector<const char*> PhysicalDevice::GetEnabledExtensions() const
    {
        std::vector<const char*> deviceEnabledExtensions;

        for (const std::string& ext : m_EnabledExtensions)
            deviceEnabledExtensions.push_back(ext.c_str());

        return deviceEnabledExtensions;
    }

    bool PhysicalDevice::HasFeatures(VkPhysicalDeviceFeatures reqFeatures) const
    {
        VKP_REGISTER_FUNCTION();
        // Compare struct members using pointers 

        const VkBool32* pReqFeature = (VkBool32*)(&reqFeatures);
        const VkBool32* pDevFeature = (VkBool32*)(&m_Features);
        const size_t kMemberCount = sizeof(VkPhysicalDeviceFeatures) /
                                    sizeof(VkBool32);

        for (size_t i = 0; i < kMemberCount; ++i)
        {
            VkBool32 featureIsNotSupported = *pReqFeature == true &&
                                             *pDevFeature == false;
            if (featureIsNotSupported)
                return false;

            ++pReqFeature;
            ++pDevFeature;
        }

        return true;
    }

    bool PhysicalDevice::HasExtensions(
        const std::vector<const char*>& reqDeviceExtensions) const
    {
        VKP_REGISTER_FUNCTION();
        std::set<std::string> requiredExtensions(reqDeviceExtensions.begin(),
                                                 reqDeviceExtensions.end());

        // All the required extensions must be present in device extensions
        for (const auto& extension : m_Extensions)
            requiredExtensions.erase(extension.extensionName);

        return requiredExtensions.empty();
    }

    bool PhysicalDevice::HasQueueFamilies(
        const std::vector<VkQueueFlagBits>& reqQFamilies) const
    {
        for (const auto& reqQFamilyBits : reqQFamilies)
        {
            if ( !m_QFamilyIndices.FlagBits2Index(reqQFamilyBits).has_value() )
                return false;
        }

        return true;
    }

    void PhysicalDevice::SetEnabledFeatures(VkPhysicalDeviceFeatures features)
    {
        m_EnabledFeatures = features;
    }

    void PhysicalDevice::AddEnabledExtensions(
        const std::vector<const char*>& extensions)
    {
        std::copy(extensions.begin(), extensions.end(),
                  std::inserter(
                    m_EnabledExtensions,
                    m_EnabledExtensions.begin()
                  )
        );
    }

    QFamilyIndex PhysicalDevice::FindQueueFamilyIndex(
        VkQueueFlagBits queueFlags) const
    {
        // Find dedicated queues first

        // Compute queue
        if (queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            uint32_t i = 0;
            for (const auto& queueFamily : m_QFamilyProps)
            {
                if ( (queueFamily.queueFlags & queueFlags) &&
                     (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 )
                {
                    return i;
                }
                ++i;
            }
        }

        // Transfer queue
        if (queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            uint32_t i = 0;
            for (const auto& queueFamily : m_QFamilyProps)
            {
                if ( (queueFamily.queueFlags & queueFlags) &&
                     (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
                     (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0 )
                {
                    return i;
                }
                ++i;
            }
        }

        // Find the first queue that supports the flags
        uint32_t i = 0;
        for (const auto& queueFamily : m_QFamilyProps)
        {
            if (queueFamily.queueFlags & queueFlags)
            {
                return i;
            }
            ++i;
        }

        return {};
    }

    void PhysicalDevice::FindPresentationQueueIndex(const VkSurfaceKHR surface)
    {
        // Find the queues that support presentation
        std::vector<VkBool32> presentSupport(m_QFamilyProps.size());
        for (uint32_t i = 0; i < m_QFamilyProps.size(); ++i) 
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(m_Device, i, surface, 
                                                 &presentSupport[i]);
        }

        // Presentation requires both the graphics and presentation 
        //  queue families to be supported
        QFamilyIndex graphicsIndex, presentIndex;

        // Prioritze a queue that supports both graphics and presentation
        //  queue families together over two separated queue families
        for (uint32_t i = 0; i < m_QFamilyProps.size(); ++i)
        {
            if (m_QFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (!graphicsIndex.has_value())
                {
                    graphicsIndex = i;
                }
                if (presentSupport[i] == VK_TRUE)
                {
                    graphicsIndex = i;
                    presentIndex = i;
                    break;
                }
            }
        }

        // Single "joined" queue was not found
        if (!presentIndex.has_value())
        {
            // Find the first queue that supports presentation
            for (uint32_t i = 0; i < m_QFamilyProps.size(); ++i) 
            {
                if (presentSupport[i] == VK_TRUE)
                {
                    presentIndex = i;
                    break;
                }
            }
        }

        if (presentIndex.has_value() && graphicsIndex.has_value())
        {
            // Update to the joined queue
            if (presentIndex.value() == graphicsIndex.value())
            {
                m_QFamilyIndices[QFamily::Graphics] = graphicsIndex;
                m_QFamilyIndices[QFamily::Present] = presentIndex;
            }
            else
            {
                m_QFamilyIndices[QFamily::Present] = presentIndex;
            }
        }
    }

    // =========================================================================
    // Logging
    // =========================================================================

    void PhysicalDevice::PrintLogTraceProperties() const
    {
        VKP_LOG_TRACE("Device:\n\tAPI version: {}\n\tDriver version {}\n"
                           "\tVendor ID: {}\n\tDevice ID: {}\n\tDevice name: {}",
                           m_Properties.apiVersion, m_Properties.driverVersion, 
                           m_Properties.vendorID, m_Properties.deviceID, 
                           m_Properties.deviceName);
    }

    void PhysicalDevice::PrintLogTraceMemoryRequirements(
        const VkMemoryRequirements& requirements)
    {
        VKP_LOG_TRACE("VkMemoryRequirements:\nSize: {}, Alignment: {}, "
                      "memoryTypeBits: {}", 
                      requirements.size, requirements.alignment, 
                      requirements.memoryTypeBits); // TODO print bits in hexa
    }

} // namespace vkp
