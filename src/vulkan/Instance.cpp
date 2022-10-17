/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Instance.h"


namespace vkp
{
    Instance::Instance(const char* appName,
                       const std::vector<const char*>& extensions)
    {
        VKP_REGISTER_FUNCTION();

        CreateInstance(appName, extensions);
    #ifdef VKP_DEBUG
        SetupDebugMessenger();
    #endif
    }

    Instance::~Instance()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Instance::CreateInstance(const char* appName,
                                  const std::vector<const char*>& extensions)
    {
        // Optional - tell the driver about the app, to possibly optimize
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName,
            .applicationVersion = VULKAN_VERSION,
            .pEngineName = appName,
            .engineVersion = VULKAN_VERSION,
            .apiVersion = VULKAN_VERSION,
        };

        VKP_LOG_INFO("Reqesting Vulkan version: {}.{}", 
                      VK_API_VERSION_MAJOR(VULKAN_VERSION),
                      VK_API_VERSION_MINOR(VULKAN_VERSION));

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

    #ifdef VKP_DEBUG
        VKP_LOG_INFO("Trying to enable validation layers ...");
        VKP_ASSERT_MSG(ValidationLayersSupported(),
                       "Validation layers not available");

        // Set up debug callback
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo = InitDebugMessengerCreateInfo();

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo,
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(s_kValidationLayers.size());
        createInfo.ppEnabledLayerNames = s_kValidationLayers.data();
    #endif

        auto err = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        VKP_ASSERT_RESULT(err);
    }

    void Instance::Destroy()
    {
        #ifdef VKP_DEBUG
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        #endif

        vkDestroyInstance(m_Instance, nullptr);
    }

    // --------------------------------------------------------------------------
    // Utils

    std::vector<VkPhysicalDevice> Instance::GetPhysicalDevices() const
    {
        VKP_REGISTER_FUNCTION();

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        VKP_LOG_TRACE("Devices found: {}", deviceCount);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        return devices;
    }

    // --------------------------------------------------------------------------
    // Debug utils

    bool Instance::ValidationLayersSupported() const
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : s_kValidationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    void Instance::SetupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo =
            InitDebugMessengerCreateInfo();

        auto err = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr,
                                                &m_DebugMessenger);
        VKP_ASSERT_RESULT(err);
    }

    VkResult Instance::CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"
        );

        if (func != nullptr)
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkDebugUtilsMessengerCreateInfoEXT Instance::InitDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            // TODO debug level
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT  |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = Instance::DebugCallback;

        return createInfo;
    }

    void Instance::DestroyDebugUtilsMessengerEXT(
        VkInstance instance, 
        VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT"
        );

        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL Instance::DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            VKP_LOG_ERR("Validation layer: {}", pCallbackData->pMessage);
        }
        else if (messageSeverity &
                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            VKP_LOG_WARN("Validation layer: {}", pCallbackData->pMessage);
        }
        else if (messageSeverity &
                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            VKP_LOG_INFO("Validation layer: {}", pCallbackData->pMessage);
        }
        else if (messageSeverity &
                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            VKP_LOG_TRACE("Validation layer: {}", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

} // namespace vkp
