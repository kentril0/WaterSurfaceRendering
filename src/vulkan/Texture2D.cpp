/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "vulkan/Buffer.h"


namespace vkp
{

    Texture2D::Texture2D(const Device& device)
        : m_Device(device),
          m_Image(device),
          m_ImageView(device),
          m_Sampler(device),
          m_SamplerInfo(Sampler::InitSamplerInfo()),
          m_CustomImageView(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);
    }

    Texture2D::~Texture2D()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void Texture2D::CreateFromFile(std::string_view filename, //bool mipmapping,
        Buffer& stagingBuffer, VkCommandBuffer commandBuffer)
    {
        VKP_REGISTER_FUNCTION();

        m_Filename = filename;

        // Initialize the staging buffer with the read data
        {
            auto pixels = Texture2D::LoadImage(filename, m_Width, m_Height);
            m_MipLevels = static_cast<uint32_t>(
                std::floor(
                    std::log2( std::max(m_Width, m_Height) )
                )
            ) + 1;

            VkDeviceSize imageSize = m_Width * m_Height * s_kImageComponents;

            stagingBuffer.Create(imageSize);
            stagingBuffer.Fill(pixels, imageSize);

            Texture2D::FreeImageData(pixels);
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
                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 
                           m_MipLevels);

        m_SamplerInfo.maxLod = static_cast<float>(m_MipLevels);
        m_Sampler.Create(m_SamplerInfo);
    }

    void Texture2D::Create(VkCommandBuffer cmdBuffer,
        uint32_t width, uint32_t height, VkFormat format, bool mipmapping,
        VkImageUsageFlags usage)
    {
        VKP_REGISTER_FUNCTION();
        m_Width = width;
        m_Height = height;

        //VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        //                          VK_IMAGE_USAGE_SAMPLED_BIT;

        if (mipmapping)
        {
            m_MipLevels = static_cast<uint32_t>(
                std::floor(
                    std::log2( std::max(m_Width, m_Height) )
                )
            ) + 1;

            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        // TODO check format support

        m_Image.Create(VkExtent3D{m_Width, m_Height, 1}, 
                        m_MipLevels,
                        format,
                        usage,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        m_Image.TransitionLayout_UNDEFtoDST_OPTIMAL(cmdBuffer);

        m_ImageView.Create(m_Image, format, 
                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 
                           m_MipLevels);

        m_SamplerInfo.maxLod = static_cast<float>(m_MipLevels);
        m_Sampler.Create(m_SamplerInfo);
    }

    void Texture2D::Create(VkCommandBuffer cmdBuffer, uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling, VkFormatFeatureFlags requiredTilingFeatures,
        VkImageUsageFlags usage, VkImageLayout dstLayout,
        const std::vector<uint32_t>& queueFamilyIndices,
        VkSamplerAddressMode addressModeU,
        VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW)
    {
        VKP_REGISTER_FUNCTION();
        m_Width = width;
        m_Height = height;
        m_MipLevels = 1;

        // TODO do it only once, ** should be one Image level **
        // Check if supports store compute shader calculations
        VKP_ASSERT_MSG(
            DeviceSupportsFormatTilingFeatures(format, requiredTilingFeatures, tiling),
            "Texture image format does not support tiling features: {}",
            requiredTilingFeatures); // TODO macro from vulkan enum to name?

        m_Image.Create(VkExtent3D{m_Width, m_Height, 1}, 
                        m_MipLevels,
                        format,
                        usage,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        tiling,
                        VK_SAMPLE_COUNT_1_BIT,
                        1, 0, //VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
                        //VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT,
                        queueFamilyIndices.size(),
                        queueFamilyIndices.data() );
        
        m_Image.TransitionLayout(cmdBuffer, dstLayout,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        );

        m_ImageView.Create(m_Image, format, 
                           VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 
                           m_MipLevels);

        m_SamplerInfo.maxLod = static_cast<float>(m_MipLevels);
        m_SamplerInfo.addressModeU = addressModeU;
        m_SamplerInfo.addressModeV = addressModeV;
        m_SamplerInfo.addressModeW = addressModeW;
        m_Sampler.Create(m_SamplerInfo);
    }

    void Texture2D::CopyFromBuffer(VkCommandBuffer cmdBuffer,
                                   VkBuffer buffer, bool genMips,
                                   VkPipelineStageFlags dstStage,
                                   uint32_t bufferOffset)
    {
        m_Image.TransitionLayoutToDST_OPTIMAL(cmdBuffer, dstStage);

        CopyBufferToImage(cmdBuffer, buffer, bufferOffset);

        if (genMips && m_MipLevels > 1)
        {
            GenerateMipmaps(cmdBuffer, dstStage);
        }
        else
        {
            m_Image.TransitionLayout_DST_OPTIMALtoSHADER_READ(cmdBuffer,
                                                              dstStage);
        }
    }

    void Texture2D::CopyBufferToImage(VkCommandBuffer cmdBuffer,
        VkBuffer buffer, uint32_t offset)
    {
        VKP_ASSERT(m_Image.GetLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Buffer data at offset 0, tightly-packed to whole image
        VkBufferImageCopy region = {
            .bufferOffset = offset,
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
            .imageExtent = { m_Width, m_Height, 1 }
        };

        vkCmdCopyBufferToImage(
            cmdBuffer,
            buffer,
            m_Image,
            // Assumes the image has been transitioned to optimal
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );
    }

    void Texture2D::GenerateMipmaps(VkCommandBuffer cmdBuffer,
        VkPipelineStageFlags dstStage)
    {
        VKP_REGISTER_FUNCTION();

        if (m_Image.GetLayout() != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            // TODO handle better
            m_Image.RecordImageBarrier(cmdBuffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dstStage,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            m_Image.SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        }
        
        VKP_ASSERT(m_Image.GetLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                   || m_Image.GetLayout() == VK_IMAGE_LAYOUT_GENERAL); // TODO


        // TODO do it only once
        // Check if format supports linear blitting
        VKP_ASSERT_MSG(
            DeviceSupportsFormatTilingFeatures(m_Image.GetFormat(),
                        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT),
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
                VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage,
                0,
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
            VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        m_Image.SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }


    bool Texture2D::DeviceSupportsFormatTilingFeatures(
        VkFormat format,
        VkFormatFeatureFlags requestedFeatures,
        VkImageTiling tiling) const
    {
        VkFormatProperties formatProperties =
            m_Device.GetFormatProperties(format);

        switch(tiling)
        {
            case VK_IMAGE_TILING_LINEAR:
                return formatProperties.linearTilingFeatures & requestedFeatures;
            case VK_IMAGE_TILING_OPTIMAL:
                return formatProperties.optimalTilingFeatures & requestedFeatures;
            default:
                VKP_ASSERT(false);
                return false;
        }
    };

    void Texture2D::Destroy()
    {
        //m_Sampler.Destroy();
        //m_ImageView.Destroy();
        //m_Image.Destroy();
    }

    unsigned char* Texture2D::LoadImage(std::string_view filename, 
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

    void Texture2D::FreeImageData(unsigned char* data)
    {
        stbi_image_free(data);
    }

    void Texture2D::SetMagFilter(VkFilter f)
    { 
        m_SamplerInfo.magFilter = f;
        SetDirty();
    }
    void Texture2D::SetMinFilter(VkFilter f)
    {
        m_SamplerInfo.minFilter = f;
        SetDirty();
    }

    void Texture2D::SetAddressModeU(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeU = mode; 
        SetDirty();
    }
    void Texture2D::SetAddressModeV(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeV = mode; 
        SetDirty();
    }
    void Texture2D::SetAddressModeW(VkSamplerAddressMode mode)
    { 
        m_SamplerInfo.addressModeW = mode; 
        SetDirty();
    }
    void Texture2D::SetBorderColor(VkBorderColor c)
    {
        m_SamplerInfo.borderColor = c;
        SetDirty();
    }
    void Texture2D::SetAnisotropyEnabled(bool e)
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
    void Texture2D::SetMaxAnisotropy(float f)
    {
        //if (enabled)
        m_SamplerInfo.maxAnisotropy = f;
        // TODO clamp <1.0f, maxAniso>
        SetDirty();
    }

    void Texture2D::SetMipmappingEnabled(bool e)
    {
        // TODO
    }

    void Texture2D::SetMipmapMode(VkSamplerMipmapMode mode)
    { 
        m_SamplerInfo.mipmapMode = mode; 
        SetDirty();
    }

    void Texture2D::SetMipLodBias(float f)
    {
        m_SamplerInfo.mipLodBias = f;
        SetDirty();
    }

    void Texture2D::SetMinLod(float f)
    {
        m_SamplerInfo.minLod = f;
        SetDirty();
    }

    void Texture2D::SetMaxLod(float f)
    {
        m_SamplerInfo.maxLod = f;
        SetDirty();
    }


    void Texture2D::UpdateOnDirty()
    {
        if (!IsDirty())
        {
            return;
        }

        m_OldSamplers.emplace_back(std::move(m_Sampler));
        m_Sampler.Create(m_SamplerInfo);
        ClearDirty();
    }

    uint32_t Texture2D::FormatToBytes(VkFormat format)
    {
        // Some of the widely supported formats for 'sampled image'
        //   according to: "https://vulkan.gpuinfo.org/listoptimaltilingformats.php"
        switch(format)
        {
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_UNORM:
                return 1;
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_UINT:
                return 2;
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_UINT:
                return 4;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_UINT:
                return 4 * 2;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_UINT:
                return 4 * 4;
            default:
                VKP_ASSERT_MSG(false, "Unsupported format to bytes");
                return 0;
        }
    }

    void Texture2D::CreateCustomImageView(
        VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT_MSG(m_Width > 0 && m_Height > 0,
                       "Image must be created first!");

        // TODO check format support

        m_CustomImageView.Destroy();

        m_CustomImageView.Create(
            m_Image,
            format,
            aspectFlags,
            VK_IMAGE_VIEW_TYPE_2D,
            m_MipLevels
        );
    }

} // namespace vkp
