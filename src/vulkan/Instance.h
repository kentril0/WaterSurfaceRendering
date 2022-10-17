/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_INSTANCE_H_ 
#define WATER_SURFACE_RENDERING_VULKAN_INSTANCE_H_

#include <array>
#include <vector>


namespace vkp
{
    class Instance
    {
    public:

    #ifdef VKP_DEBUG
        static constexpr std::array s_kValidationLayers {
            "VK_LAYER_KHRONOS_validation"
        };
    #endif

    public:
        Instance(const char* appName,
                 const std::vector<const char*>& extensions);
        ~Instance();

        operator VkInstance() const { return m_Instance; }

        // ---------------------------------------------------------------------
        // Utils

        std::vector<VkPhysicalDevice> GetPhysicalDevices() const;


    private:
        void CreateInstance(const char* appName,
                            const std::vector<const char*>& extensions);
        void Destroy();

    private:
        VkInstance   m_Instance{ VK_NULL_HANDLE };

        // ---------------------------------------------------------------------
        // Debug utils

    #ifdef VKP_DEBUG
        bool ValidationLayersSupported() const;
        void SetupDebugMessenger();

        static VkDebugUtilsMessengerCreateInfoEXT InitDebugMessengerCreateInfo();

        static VkResult CreateDebugUtilsMessengerEXT(
            VkInstance                                instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks*              pAllocator,
            VkDebugUtilsMessengerEXT*                 pDebugMessenger);

        static void DestroyDebugUtilsMessengerEXT(
            VkInstance                   instance, 
            VkDebugUtilsMessengerEXT     debugMessenger, 
            const VkAllocationCallbacks* pAllocator);

        // @return True - the Vulkan call is aborted:
        // VK_ERROR_VALIDATION_FAILED_EXT
        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT             messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void*                                       pUserData);


    private:
        VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };

    #endif

    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_INSTANCE_H_
