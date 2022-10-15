
#ifndef WATER_SURFACE_RENDERING_VULKAN_IMAGEVIEW_H_
#define WATER_SURFACE_RENDERING_VULKAN_IMAGEVIEW_H_

#include <vulkan/vulkan.h>


namespace vkp
{
    class Device;

    /** 
     * @brief VkImageView abstraction
     */
    class ImageView 
    {
    public:
        /** 
         * @brief Initializes the image view info structure to commonly used
         *  predefined values
         */
        static VkImageViewCreateInfo InitImageViewInfo();

    public:
        ImageView(const Device& device);
        ImageView(ImageView&& other);

        ~ImageView();

        operator VkImageView() const { return m_ImageView; }
        VkImageView GetImageView() const { return m_ImageView; }

        /**
         * @brief Creates the image view on a certain image object
         * @param image Image on which the view will be created
         * @param viewType Type of the image view
         * @param format How to interpret texel block in the image
         * @param mipLevelCount Number of mipmap levels accessible to the view
         * @param arrayLayerCount Number of array layers accessible
         * @param mipLevel The first mipmap level accessible to the view
         * @param arrayLayer The first array layer accessible to the view
         */
        void Create(VkImage image, 
                    VkFormat format, 
                    VkImageAspectFlags aspectFlags,
                    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
                    uint32_t mipLevelCount = 1,
                    uint32_t mipLevel = 0,
                    uint32_t arrayLayerCount = 1,
                    uint32_t arrayLayer = 0);

        void Destroy();

        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;
        ImageView& operator=(ImageView&&) = delete;

    private:
        const Device& m_Device;

        VkImageView  m_ImageView{ VK_NULL_HANDLE };
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_IMAGEVIEW_H_