/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Surface.h"


namespace vkp
{
    Surface::Surface(const VkInstance instance,
                     VkSurfaceKHR surface)
        : m_Instance(instance),
          m_Surface(surface)
    {
        VKP_REGISTER_FUNCTION();

        VKP_ASSERT(instance != VK_NULL_HANDLE);
        VKP_ASSERT(surface != VK_NULL_HANDLE);
    }
    
    Surface::~Surface()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Surface::Destroy()
    {
        if (m_Surface == VK_NULL_HANDLE)
            return;

        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }

} // namespace vkp

