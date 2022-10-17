/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_SURFACE_H_
#define WATER_SURFACE_RENDERING_VULKAN_SURFACE_H_


namespace vkp
{
    class Surface
    {
    public:
        Surface(const VkInstance instance,
                VkSurfaceKHR surface);
        /** @pre Instance must exist, must be destroyed before instance */
        ~Surface();

        operator VkSurfaceKHR() const { return m_Surface; }

    private:
        void CreateSurface();
        void Destroy();

    private:
        // Needed instance for RAII destruction
        const VkInstance m_Instance{ VK_NULL_HANDLE };

        VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_SURFACE_H_
