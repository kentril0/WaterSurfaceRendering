/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_TEXTURE3D_H_
#define WATER_SURFACE_RENDERING_VULKAN_TEXTURE3D_H_

#include <string_view>
#include <vulkan/vulkan.h>

#include "vulkan/Device.h"
#include "vulkan/Buffer.h"
#include "vulkan/Image.h"
#include "vulkan/ImageView.h"
#include "vulkan/Sampler.h"


namespace vkp
{
    class Texture3D
    {
    public:
        // Force RGBA image
        static const int      s_kImageComponents{ 4 }; //STBI_rgb_alpha
        static const VkFormat s_kDefaultFormat  { VK_FORMAT_R8G8B8A8_UNORM };

    public:
        /**
         * @brief Sets up the texture resources for subsequent creation with the
         *  'Create' call. 
         * @param device Created logical device
         */
        Texture3D(const Device& device);
        ~Texture3D(); 

        /**
         * @brief Creates a 3D RGBA texture with pixels loaded from a file, 
         *  Uses staging buffer to copy the image data from host to the
         *  device local memory, 
         * @param filename Path to the file with the texture image
         * @param cmdBuffer Command buffer for any commands to RECORD into
         */
        void Create(std::string_view filename, 
                    //bool mipmapping = true,
                    Buffer& stagingBuffer,
                    VkCommandBuffer cmdBuffer);
        
        void Create(uint32_t width,
                    uint32_t height,
                    uint32_t depth);
        
        VkImageView GetImageView() const { return m_ImageView; }
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

    private:
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

        /**
         * @brief Enques a copy command from a staging buffer to the image
         * @pre The image must be in LAYOUT_TRANSFER_DST_OPTIMAL
         * @param cmdBuffer Command buffer in a recording state
         * @param buffer Staging buffer filled with data
         */
        void CopyBufferToImage(VkCommandBuffer cmdBuffer,
                               VkBuffer buffer);

        void GenerateMipmaps(VkCommandBuffer cmdBuffer);

        void Destroy();

        void SetDirty() { m_IsDirty = true; }
        void ClearDirty() { m_IsDirty = false; }

    private:
        const Device& m_Device;
        std::string_view m_Filename;

        // ---------------------------------------------------------------------
        // Owned

        uint32_t m_Width { 0 };
        uint32_t m_Height{ 0 };
        uint32_t m_Depth { 0 };
        uint32_t m_MipLevels{ 1 };

        Image     m_Image;
        ImageView m_ImageView;
        Sampler   m_Sampler;
 
        // ---------------------------------------------------------------------
        // For setting up sampler parameters from the 'outside'

        VkSamplerCreateInfo m_SamplerInfo{};
        bool m_IsDirty{ false };     ///< Whether props have been changed

        std::vector<Sampler>      m_OldSamplers;
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_TEXTURE3D_H_