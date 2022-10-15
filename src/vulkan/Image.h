
#ifndef WATER_SURFACE_RENDERING_VULKAN_IMAGE_H_
#define WATER_SURFACE_RENDERING_VULKAN_IMAGE_H_

#include <vulkan/vulkan.h>


namespace vkp
{
    class Device;

    /** 
     * @brief VkImage abstraction
     * WORKS ONLY FOR UNIFIED GRAPHICS AND TRANSFER QUEUES
     */
    class Image 
    {
    public:
        /** 
         * @brief Initializes the image info structure to commonly used
         *  predefined values
         */
        static VkImageCreateInfo InitImageInfo();

    public:
        Image(const Device& device);
        Image(Image&& other);

        ~Image();

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image& operator=(Image&&) = delete;

        operator VkImage() const { return m_Image; }

        //void Create(const VkImageCreateInfo& info);

        /**
         * @brief 1) Creates VkImage handle, 2) allocates memory according to
         *  image memory requirements on the device, 3) binds the memory to the
         *  image handle
         *  Created in layout: VK_IMAGE_LAYOUT_UNDEFINED
         */
        void Create(const VkExtent3D& extent, 
                    uint32_t mipLevels,
                    VkFormat format,
                    VkImageUsageFlags usage,
                    VkMemoryPropertyFlags memoryProps,
                    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                    uint32_t arrayLayers = 1,
                    VkImageCreateFlags flags = 0,
                    uint32_t queueFamilyCount = 0,
                    const uint32_t* queueFamilies = nullptr);
        
        /**
         * @brief Destroys the image handle, frees the bound image memory,
         *  if not already destroyed
         */
        void Destroy();

        // ---------------------------------------------------------------------
        // Layout Transitions
        // ---------------------------------------------------------------------

        /**
         * @brief *PRECOPY* Prepares the image layout for transfer
         *  - transition from LAYOUT_UNDEFINED to LAYOUT_TRANSFER_DST_OPTIMAL
         * Bg: Submission guarantees the host write being complete
         *     (e.g. to staging buffer):
         *      - no need for memory barrier before the transfer
         *      - but needed for the image layout changes
         * @param cmdBuffer Command Buffer in recording state
         */
        void TransitionLayout_UNDEFtoDST_OPTIMAL(VkCommandBuffer cmdBuffer);

        /**
         * WORKS ONLY FOR UNIFIED GRAPHICS AND TRANSFER QUEUES
         * @brief *POSTCOPY* Transitions the image layout for read in shader
         *  from LAYOUT_TRANSFER_DST_OPTIMAL to LAYOUT_SHADER_READ_ONLY_OPTIMAL
         * @param cmdBuffer Command Buffer in recording state
         */
        void TransitionLayout_DST_OPTIMALtoSHADER_READ(
            VkCommandBuffer cmdBuffer,
            VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        /**
         * @brief *PRECOPY* Prepares the image layout for transfer
         *  - from SHADER_READ_ONLY_OPTIMAL to LAYOUT_TRANSFER_DST_OPTIMAL
         * @param cmdBuffer Command Buffer in recording state
         */
        void TransitionLayout_SHADER_READtoDST_OPTIMAL(
            VkCommandBuffer cmdBuffer, 
            VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        /**
         * @brief Transitions the image to LAYOUT_TRANSFER_DST_OPTIMAL from
         *  whatever layout it is in
         * @param cmdBuffer Command Buffer in recording state
         */
        void TransitionLayoutToDST_OPTIMAL(
            VkCommandBuffer cmdBuffer,
            VkPipelineStageFlags stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        // @brief Custom transition
        void TransitionLayout(VkCommandBuffer cmdBuffer,
                              VkImageLayout newLayout,
                              VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask);

        // ---------------------------------------------------------------------
        //  Getters, Setters
        // ---------------------------------------------------------------------

        VkImage GetHandle() const { return m_Image; }
        VkFormat GetFormat() const { return m_Format; }
        VkImageLayout GetLayout() const { return m_ImageLayout; }
        uint32_t GetArrayLayerCount() const { return m_ArrayLayerCount; }
        void SetLayout(VkImageLayout layout) { m_ImageLayout = layout; }

        /**
         * @brief Records an image memory barrier to a command buffer
         * @param cmbBuffer Command buffer in a recording state
         * @param srcStageMask Source STAGES bitmask
         * @param dstStageMask Destination STAGES bitmask
         * @param srcAccessMask Source ACCESS bitmask
         * @param dstAccessMask Destination ACCESS bitmask
         * @param newLayout New layout to transition to
         */
        void RecordImageBarrier(VkCommandBuffer cmdBuffer,
                                VkPipelineStageFlags srcStageMask, 
                                VkPipelineStageFlags dstStageMask,
                                VkAccessFlags srcAccessMask, 
                                VkAccessFlags dstAccessMask,
                                VkImageLayout newLayout) const;

    private:
        void CreateImage();

        void AllocateMemory(VkMemoryPropertyFlags properties);

        /** 
         * @brief Associate the memory with the image at offset 'offset'
         */
        void BindMemory(VkDeviceSize offset = 0);

    private:
        const Device& m_Device;

        VkImage        m_Image      { VK_NULL_HANDLE };
        VkDeviceMemory m_ImageMemory{ VK_NULL_HANDLE };

        VkImageType    m_Type           {};
        VkExtent3D     m_Extent         {};
        uint32_t       m_MipLevelCount  { 0 };
        uint32_t       m_ArrayLayerCount{ 0 };
        VkFormat       m_Format         { VK_FORMAT_UNDEFINED };

        VkImageLayout  m_ImageLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_IMAGE_H_