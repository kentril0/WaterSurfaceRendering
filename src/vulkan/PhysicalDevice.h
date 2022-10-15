
#ifndef WATER_SURFACE_RENDERING_VULKAN_PHYSICAL_DEVICE_H_ 
#define WATER_SURFACE_RENDERING_VULKAN_PHYSICAL_DEVICE_H_

#include <vector>
#include <set>
#include <optional>

#include "vulkan/QueueTypes.h"


namespace vkp
{
    class PhysicalDevice 
    {
    public:
        static void PrintLogTraceMemoryRequirements(
            const VkMemoryRequirements& requirements);

        void PrintLogTraceProperties() const;

    public:
        PhysicalDevice(VkPhysicalDevice device = VK_NULL_HANDLE);
        ~PhysicalDevice();

        operator VkPhysicalDevice() const { return m_Device; }

        /**
         * @brief Initializes the members with the properties of the
         *  physical device 'device' for any subseqent use
         */
        void Init(VkPhysicalDevice device);

        /**
         * @brief Finds the required type of device memory with required memory
         *  properties, or exits
         * @param memoryTypeBitsRequirement Bitmask of the required memory type,
         *  as returned in 'VkMemoryRequirements' struct
         * @param requiredProperties Bitmask of property flags that must be found 
         * @return Index of the memory type that satisfies the requirements
         */
        uint32_t FindMemoryType(uint32_t memoryTypeBitsRequirement,
                                VkMemoryPropertyFlags requiredProperties) const;
 
        bool HasFeatures(VkPhysicalDeviceFeatures reqFeatures) const;
        bool HasExtensions(
            const std::vector<const char*>& reqDeviceExtensions) const;
        bool HasQueueFamilies(
            const std::vector<VkQueueFlagBits>& reqQFamilies) const;

        void SetEnabledFeatures(VkPhysicalDeviceFeatures features);
        void AddEnabledExtensions(const std::vector<const char*>& extensions);

        /**
         * @brief Requests the support of required swap chain extensions,
         *  searches for queue family that supports surface presentation
         * @return True if supports the required swap chain extension,
         *  and has queue family that supports presentation
         */
        bool RequestSwapChainSupport(const VkSurfaceKHR surface);

        // @brief Call "FindPresentationQueueIndex" function first
        bool SupportsPresentation() const
        {
            return m_QFamilyIndices[QFamily::Graphics].has_value() &&
                   m_QFamilyIndices[QFamily::Present].has_value();
        }

        const VkPhysicalDeviceProperties& GetProperties() const { 
            return m_Properties;
        }
        const VkPhysicalDeviceFeatures& GetFeatures() const {
            return m_Features;
        }
        const VkPhysicalDeviceFeatures& GetEnabledFeatures() const {
            return m_EnabledFeatures;
        }
        const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const {
            return m_MemProperties;
        };

        std::vector<const char*> GetEnabledExtensions() const;

        const QFamilyIndices& GetQueueFamilyIndices() const {
            return m_QFamilyIndices;
        }

        inline float GetMaxAnisotropy() const { 
            return m_Properties.limits.maxSamplerAnisotropy;
        }
        inline float GetMaxLodBias() const { 
            return m_Properties.limits.maxSamplerLodBias;
        }

        inline size_t GetMinUniformBufferOffsetAlignment() const
        {
            return m_Properties.limits.minUniformBufferOffsetAlignment;
        }

    private:

        void FindSupportedExtensions();
        void FindQueueFamilyProperties();

        void FindQueueFamilyIndices();
        QFamilyIndex FindQueueFamilyIndex(VkQueueFlagBits queueFlags) const;

        /** 
         * @brief Looks for a queue family that has the capability of 
         * presenting to the window surface, while prioritizing when 
         * both graphics and presentation queue families are supported by
         * one queue. Sets the internal structures when found.
         */
        void FindPresentationQueueIndex(const VkSurfaceKHR surface);

    private:
        VkPhysicalDevice m_Device{ VK_NULL_HANDLE };

        // TODO keep only necessary, like enabled...
        VkPhysicalDeviceProperties              m_Properties{};
        VkPhysicalDeviceMemoryProperties        m_MemProperties{};
        VkPhysicalDeviceFeatures                m_Features{};
        std::vector<VkExtensionProperties>      m_Extensions;

        VkPhysicalDeviceFeatures                m_EnabledFeatures{};
        std::set<std::string>                   m_EnabledExtensions;

        std::vector<VkQueueFamilyProperties>    m_QFamilyProps;
        QFamilyIndices                          m_QFamilyIndices{};

        static constexpr VkQueueFlagBits QUEUE_FAMILY_BITS[]
        {
            VK_QUEUE_GRAPHICS_BIT,
            VK_QUEUE_COMPUTE_BIT,
            VK_QUEUE_TRANSFER_BIT
        };

    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_PHYSICAL_DEVICE_H_
