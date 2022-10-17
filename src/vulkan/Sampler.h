/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_SAMPLER_H_
#define WATER_SURFACE_RENDERING_VULKAN_SAMPLER_H_

#include <vulkan/vulkan.h>


namespace vkp
{
    class Device;

    /** 
     * @brief VkSampler abstraction
     */
    class Sampler
    {
    public:
        /** 
         * @brief Initializes the sampler info structure to commonly used
         *  predefined values
         */
        static VkSamplerCreateInfo InitSamplerInfo();

    public:
        Sampler(Device& device);
        Sampler(Device& device, 
                const VkSamplerCreateInfo& info);
        Sampler(Sampler&& other);

        ~Sampler();

        operator VkSampler() const { return m_Sampler; }

        void Create(const VkSamplerCreateInfo& info);

        void Destroy();

        // One one sampler can keep VkSampler pointer at one time
        //  prevents multiple deletions
        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;
        Sampler& operator=(Sampler&&) = delete;

    private:
        Device& m_Device;

        // ---------------------------------------------------------------------
        VkSampler m_Sampler{ VK_NULL_HANDLE };
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_SAMPLER_H_