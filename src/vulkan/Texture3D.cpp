/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Texture3D.h"

#include <stb/stb_image.h>

#include "vulkan/Buffer.h"


namespace vkp
{

    Texture3D::Texture3D(Device& device)
        : m_Device(device),
          m_Image(device),
          m_ImageView(device),
          m_Sampler(device),
          m_SamplerInfo(Sampler::InitSamplerInfo())
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
    }

    Texture3D::~Texture3D()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Texture3D::Create(std::string_view filename, //bool mipmapping,
        Buffer& stagingBuffer, VkCommandBuffer commandBuffer)
    {
        VKP_REGISTER_FUNCTION();

        m_Filename = filename;

        // Initialize the staging buffer with the read data
        {
            auto pixels = Texture3D::LoadImage(filename, m_Width, m_Height);
            m_MipLevels = static_cast<uint32_t>(
                std::floor(
                    std::log2( std::max(m_Width, m_Height) )
                )
            ) + 1;

            VkDeviceSize imageSize = m_Width * m_Height * s_kImageComponents;

            stagingBuffer.Create(imageSize);
            stagingBuffer.Fill(pixels, imageSize);

            Texture3D::FreeImageData(pixels);
        }

        // Create the texture image
        m_Image.Create(VkExtent3D{m_Width, m_Height, 1}, 
                        m_MipLevels,
                        s_kDefaultFormat,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_Image.TransitionLayout_UNDEFtoDST_OPTIMAL(commandBuffer);

        // Copy the staging buffer to the texture image
        CopyBufferToImage(commandBuffer, stagingBuffer);

        // Prepare the texture image for shader access
        //m_Image.TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //                         commandBuffer);

        GenerateMipmaps(commandBuffer);

        m_ImageView.Create(m_Image, m_Image.GetFormat(), 
                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_3D, 
                           m_MipLevels);

        m_SamplerInfo.maxLod = static_cast<float>(m_MipLevels);
        m_Sampler.Create(m_SamplerInfo);
    }

    void Texture3D::Create(uint32_t width, uint32_t height, uint32_t depth)
    {
        VKP_REGISTER_FUNCTION();
        m_Width = width;
        m_Height = height;
        m_Depth = depth;

        m_MipLevels = 1;
        const VkFormat kFormat = VK_FORMAT_R8_UNORM;

        // Format support check
        // 3D texture support in Vulkan is mandatory (in contrast to OpenGL) so
        // no need to check if it's supported, but limits and usage must be checked

        VkFormatProperties formatProperties = 
            m_Device.GetFormatProperties(kFormat);

        VKP_ASSERT_MSG(formatProperties.optimalTilingFeatures & 
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT, "Device does not support"
                       " TRANSFER_DST for selected texture format");

        // Check if GPU supports requested 3D texture dimensions
        const uint32_t kMaxImageDimension3D = 
            m_Device.GetPhysicalDevice().GetProperties().limits.maxImageDimension3D;
        VKP_ASSERT_MSG(!(width > kMaxImageDimension3D || 
                         height > kMaxImageDimension3D ||
                         depth > kMaxImageDimension3D), 
                         "Device does not support requested 3D texture dimensions");

        // Create the texture image in device local memory
        //  for destination transfer
        m_Image.Create(VkExtent3D{m_Width, m_Height, m_Depth}, 
                        m_MipLevels,
                        kFormat,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_ImageView.Create(m_Image, m_Image.GetFormat(), 
                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_3D, 
                           m_MipLevels);

        m_Sampler.Create(m_SamplerInfo);
    }

    void Texture3D::CopyBufferToImage(VkCommandBuffer commandBuffer,
                                      VkBuffer buffer)
    {
        VKP_ASSERT(m_Image.GetLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Buffer data at offset 0, tightly-packed to whole image
        VkBufferImageCopy region = {
            .bufferOffset = 0,
            // Tightly packed data
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = m_Image.GetArrayLayerCount(),
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = { m_Width, m_Height, m_Depth }
        };
        
        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            m_Image,
            // Assumes the image has been transitioned to optimal
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );
    }

    void Texture3D::GenerateMipmaps(VkCommandBuffer cmdBuffer)
    {
        // Check if format supports linear blitting
        VkFormatProperties formatProperties = 
            m_Device.GetFormatProperties(m_Image.GetFormat());

        const bool suppportsLinearBlitting = 
            formatProperties.optimalTilingFeatures &
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        VKP_ASSERT_MSG(suppportsLinearBlitting, 
                       "Texture image format does not support linear blitting.");

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_Image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = m_Width;
        int32_t mipHeight = m_Height;

        for (uint32_t i = 1; i < m_MipLevels; ++i)
        {
            // Wait for the previous level to be filled
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            
            // TODO into a function
            VkImageBlit blit{};
            // Source Mip level
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            // Destination Mip level
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, 
                                   mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmdBuffer,
                m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);  // Filtering

            // Transition the src level for optimal shader use
            //  - wait for the blit to finish
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
            
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition the last level
        barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        m_Image.SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void Texture3D::Destroy()
    {
        //m_Sampler.Destroy();
        //m_ImageView.Destroy();
        //m_Image.Destroy();
    }

    unsigned char* Texture3D::LoadImage(std::string_view filename, 
        uint32_t& pwidth, uint32_t& pheight, int componentCount)
    {
        // Force load with alpha channel, even if it does not have one
        int texWidth, texHeight, texChannels;

        stbi_uc* pixels = stbi_load(filename.data(), 
                                    &texWidth, &texHeight, 
                                    &texChannels, componentCount);
        VKP_ASSERT_MSG(pixels != NULL, "Failed to load texture image \"{}\"",
                       filename.data());

        pwidth = static_cast<uint32_t>(texWidth);
        pheight = static_cast<uint32_t>(texHeight);
        return pixels;
    }

    void Texture3D::FreeImageData(unsigned char* data)
    {
        stbi_image_free(data);
    }

    void Texture3D::SetMagFilter(VkFilter f)
    { 
        m_SamplerInfo.magFilter = f;
        SetDirty();
    }
    void Texture3D::SetMinFilter(VkFilter f)
    {
        m_SamplerInfo.minFilter = f;
        SetDirty();
    }

    void Texture3D::SetAddressModeU(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeU = mode; 
        SetDirty();
    }
    void Texture3D::SetAddressModeV(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeV = mode; 
        SetDirty();
    }
    void Texture3D::SetAddressModeW(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeW = mode; 
        SetDirty();
    }
    void Texture3D::SetBorderColor(VkBorderColor c)
    {
        m_SamplerInfo.borderColor = c;
        SetDirty();
    }
    void Texture3D::SetAnisotropyEnabled(bool e)
    {
        if (e)
        {
            m_SamplerInfo.anisotropyEnable = VK_TRUE;
            m_SamplerInfo.maxAnisotropy = 
                m_Device.GetPhysicalDevice().GetMaxAnisotropy();
        }
        else
        {
            m_SamplerInfo.anisotropyEnable = VK_FALSE;
            m_SamplerInfo.maxAnisotropy = 1.0f;
        }
        SetDirty();
    }
    void Texture3D::SetMaxAnisotropy(float f)
    {
        //if (enabled)
        m_SamplerInfo.maxAnisotropy = f;
        // TODO clamp <1.0f, maxAniso>
        SetDirty();
    }

    void Texture3D::SetMipmappingEnabled(bool e)
    {
        // TODO
    }

    void Texture3D::SetMipmapMode(VkSamplerMipmapMode mode)
    { 
        m_SamplerInfo.mipmapMode = mode; 
        SetDirty();
    }

    void Texture3D::SetMipLodBias(float f)
    {
        m_SamplerInfo.mipLodBias = f;
        SetDirty();
    }

    void Texture3D::SetMinLod(float f)
    {
        m_SamplerInfo.minLod = f;
        SetDirty();
    }

    void Texture3D::SetMaxLod(float f)
    {
        m_SamplerInfo.maxLod = f;
        SetDirty();
    }


    void Texture3D::UpdateOnDirty()
    {
        if (!IsDirty())
        {
            return;
        }

        m_OldSamplers.emplace_back(std::move(m_Sampler));
        m_Sampler.Create(m_SamplerInfo);
        ClearDirty();
    }

} // namespace vkp
