/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Sampler.h"
#include "vulkan/Device.h"

namespace vkp
{
    Sampler::Sampler(Device& d)
        : m_Device(d),
          m_Sampler(VK_NULL_HANDLE)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Device != VK_NULL_HANDLE);
    }

    Sampler::Sampler(Device& d, const VkSamplerCreateInfo& info)
        : Sampler(d)
    {
        Create(info);
    }

    Sampler::Sampler(Sampler&& other)
        : m_Device(other.m_Device),
          m_Sampler(other.m_Sampler)
    {
        VKP_REGISTER_FUNCTION();
        other.m_Sampler = VK_NULL_HANDLE;
    }

    Sampler::~Sampler()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Sampler::Create(const VkSamplerCreateInfo& info)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT_RESULT(
            vkCreateSampler(m_Device, &info, nullptr, &m_Sampler));
    }

    void Sampler::Destroy()
    {
        if (m_Sampler != VK_NULL_HANDLE)
	    {
            vkDestroySampler(m_Device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }
    }

    VkSamplerCreateInfo Sampler::InitSamplerInfo()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // Filtering modes
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        // Addressing modes beyond texture boundaries
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        // Anisotropy settings - hardware-based feature, _must be enabled!_
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        // Shadow maps
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        // Mipmapping
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        return samplerInfo;
    }
}
