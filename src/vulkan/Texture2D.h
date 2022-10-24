/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

// TODO create a Texture2D class, later abstract it to general Texture class
//   to create more specific derived classes like: Texture2D, Texture3D,
//      TextureArray2D, TextureCubeMaps. ...

#ifndef WATER_SURFACE_RENDERING_VULKAN_TEXTURE2D_H_
#define WATER_SURFACE_RENDERING_VULKAN_TEXTURE2D_H_

#include <string_view>
#include <vulkan/vulkan.h>
#include "vulkan/Device.h"
#include "vulkan/Buffer.h"
#include "vulkan/Image.h"
#include "vulkan/ImageView.h"
#include "vulkan/Sampler.h"


namespace vkp
{
    class Texture2D
    {
    public:
        // Force RGBA image
        static const int      s_kImageComponents{ 4 }; //STBI_rgb_alpha
        static const VkFormat s_kDefaultFormat  { VK_FORMAT_R8G8B8A8_UNORM };

        static uint32_t FormatToBytes(VkFormat format);

        /**
         * @brief Loads image data using STBI library
         * @param filename Path to the image file
         * @param pwidth Width of the loaded image to be assigned
         * @param pheight Height of the loaded image to be assigned
         * @param componentCount Required number of components to be read
         * @return Read 'componentCount' pixel data, should be freed after use
         */
        static unsigned char* LoadImage(std::string_view filename, 
                                        uint32_t& pwidth, 
                                        uint32_t& pheight,
                                        int componentCount = 4);

        // @brief Frees data loaded using STBI library
        static void FreeImageData(unsigned char* data);

    public:
        /**
         * @brief Sets up the texture resources for subsequent creation with the
         *  'Create' call. 
         * @param device Created logical device
         */
        Texture2D(const Device& device);
        ~Texture2D(); 

        /**
         * @brief Creates a device local texture with dimensions and format,
         *  in a state that is ready for transfer.
         * @param cmdBuffer Command buffer in recording state, for image
         *  layout transition to DST_OPTIMAL
         * @param width
         * @param height 
         * @param format
         */
        void Create(VkCommandBuffer cmdBuffer,
                    uint32_t width,
                    uint32_t height,
                    VkFormat format = s_kDefaultFormat,
                    bool mipmapping = false,
                    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                              VK_IMAGE_USAGE_SAMPLED_BIT);
        
        /**
         * @brief More customized "Create" function, especially for the use case
         *  of compute shaders
         * @param queueFamilyIndices Determines sharing mode, whethe to share the
         *  underlying image across queues
         */
        void Create(VkCommandBuffer cmdBuffer,
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkFormatFeatureFlags requiredTilingFeatures = 
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout dstLayout = VK_IMAGE_LAYOUT_GENERAL,
            const std::vector<uint32_t>& queueFamilyIndices = {},
            VkSamplerAddressMode addressModeU = 
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            VkSamplerAddressMode addressModeV =
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            VkSamplerAddressMode addressModeW =
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
        );

        /**
         * @brief Copies contents of the buffer (should be the same size as the
         *   image in bytes) to the Image, while ALSO TRANSITIONING the layout
         *  from DST_OPTIMAL to SHADER_READ
         * @param cmdBuffer Command buffer in recording state
         * @param buffer buffer with data, having the same size as the image
         * @param genMips If true, will generate mipmaps if created as mipmapped
         */
        void CopyFromBuffer(VkCommandBuffer cmdBuffer,
                            VkBuffer buffer,
                            bool genMips = true,
                            VkPipelineStageFlags dstStage = 
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            uint32_t bufferOffset = 0);

        /**
         * @brief Creates a 2D RGBA texture with pixels loaded from a file, 
         *  Uses staging buffer to copy the image data from host to the
         *  device local memory, Mipmapped by default
         * @param filename Path to the file with the texture image
         * @param cmdBuffer Command buffer for any commands to RECORD into
         */
        void CreateFromFile(std::string_view filename, 
                            //bool mipmapping = true,
                            Buffer& stagingBuffer,
                            VkCommandBuffer cmdBuffer);

        /**
         * @brief Generate mipmaps for the texture
         * @pre The texture must be in layout DST_OPTIMAL
         * @param cmdBuffer Command buffer in recording state
         */
        void GenerateMipmaps(VkCommandBuffer cmdBuffer,
                             VkPipelineStageFlags dstStage =
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        VkImageView GetImageView() const { return m_ImageView; }
        VkImage GetImage() const { return m_Image; }
        Image& GetImage() { return m_Image; }
        VkSampler GetSampler() const { return m_Sampler; }
        uint32_t GetMipLevels() const { return m_MipLevels; }

        VkDescriptorImageInfo GetDescriptor() const {
            return VkDescriptorImageInfo {
                .sampler = m_Sampler,
                .imageView = m_ImageView,
                .imageLayout = m_Image.GetLayout()
            };
        }

        // ---------------------------------------------------------------------
        // Setting sampler properties 
        //  To see the changes, requires sampler recreation and tis descriptor 
        //  update, @see functions IsDirty, UpdateOnDirty, ClearOld

        void SetMagFilter(VkFilter f);
        void SetMinFilter(VkFilter f);

        void SetAddressModeU(VkSamplerAddressMode mode);
        void SetAddressModeV(VkSamplerAddressMode mode);
        void SetAddressModeW(VkSamplerAddressMode mode);
        void SetBorderColor(VkBorderColor c);

        void SetAnisotropyEnabled(bool e);
        void SetMaxAnisotropy(float f);

        void SetMipmappingEnabled(bool e);
        void SetMipmapMode(VkSamplerMipmapMode mode);
        void SetMipLodBias(float f);
        void SetMinLod(float f);
        void SetMaxLod(float f);

        /** 
         * @brief Whether sampler properties have been changed (then probably 
         *  its descriptors should be updated)
         */
        bool IsDirty() const { return m_IsDirty; }

        /** 
         * @brief Recreates a sampler if any sampler properties have been 
         *  changed saves the previous VkSampler handle for deletion later
         */
        void UpdateOnDirty();

        /**
         * @brief Destroys any previously created samplers. Assumes that the 
         * samplers are no longer in use!
         */
        void ClearOld() { m_OldSamplers.clear(); }

        void CreateCustomImageView(
            VkFormat format,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
        
        VkImageView GetCustomImageView() const { return m_CustomImageView; }

    private:

        /**
         * @brief Enques a copy command from a staging buffer to the image
         * @pre The image must be in LAYOUT_TRANSFER_DST_OPTIMAL
         * @param cmdBuffer Command buffer in a recording state
         * @param buffer Staging buffer filled with data
         */
        void CopyBufferToImage(VkCommandBuffer cmdBuffer,
                               VkBuffer buffer,
                               uint32_t bufferOffset = 0);

        bool DeviceSupportsFormatTilingFeatures(
            VkFormat format,
            VkFormatFeatureFlags requiredTilingFeatures,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) const;

        void Destroy();

        void SetDirty() { m_IsDirty = true; }
        void ClearDirty() { m_IsDirty = false; }

    private:
        const Device& m_Device;
        std::string_view m_Filename;

        // ---------------------------------------------------------------------
        // Owned

        uint32_t m_Width    { 0 };
        uint32_t m_Height   { 0 };
        uint32_t m_MipLevels{ 1 };

        Image     m_Image;
        ImageView m_ImageView;
        Sampler   m_Sampler;
 
        // ---------------------------------------------------------------------
        // For setting up sampler parameters from the 'outside'

        VkSamplerCreateInfo m_SamplerInfo{};
        bool m_IsDirty{ false };     ///< Whether properties have been changed

        std::vector<Sampler>      m_OldSamplers;

        // Custom image views TODO vector
        ImageView m_CustomImageView;
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_TEXTURE2D_H_